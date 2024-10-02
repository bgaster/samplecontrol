/* This file is part of {{ samplecontrol }}.
 * 
 * 2024 Benedict R. Gaster (cuberoo_)
 * 
 * Licensed under either of
 * Apache License, Version 2.0 (LICENSE-APACHE or http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license (LICENSE-MIT or http://opensource.org/licenses/MIT)
 * at your option.
 */

#include <util.h>
#include <console.h>
#include <screen.h>

//-----------------------------------------------------------------------------------------------
// limits
//-----------------------------------------------------------------------------------------------

// max 32K 32-bit instructions
#define MAX_INSRUCTIONS 1024 * 32 

// max 8K 32-bit literals
#define MAX_LITERALS 1024 * 4

#define MAX_MEMORY (MAX_LITERALS + 1024 * 8)

#define DEFAULT_STACK_SIZE 1024 * 4

//-----------------------------------------------------------------------------------------------
// Types
//-----------------------------------------------------------------------------------------------

typedef struct {
    sc_uint magic_;
    sc_uint capabilities_;
    sc_ushort entry_point_;
    sc_uint literals_start_;
    sc_ushort literals_length_;
    sc_uint code_start_;
    sc_ushort code_length_;
} header ;

// registers
// General purpose 0   - 127 (labeled R0 ... R127)
// Streams         128 - 159 (labeled S0 ... S31)
// Generators      160 - 191 (labeled G0 ... G31)
// Consumers       192 - 223 (labeled C0 ... C31)
// Sample rate     224       (labeled SR)     
enum {
    REG_0=0, REG_1, REG_2, REG_3, REG_4, REG_5, REG_6, REG_7, REG_8, 
    REG_9, REG_10, REG_11, REG_12, REG_13, REG_14, REG_15, REG_16, 
    REG_17, REG_18, REG_19, REG_20, REG_21, REG_22, REG_23, REG_24, 
    REG_25, REG_26, REG_27, REG_28, REG_29, REG_30, REG_31, REG_32, 
    REG_33, REG_34, REG_35, REG_36, REG_37, REG_38, REG_39, REG_40, 
    REG_41, REG_42, REG_43, REG_44, REG_45, REG_46, REG_47, REG_48, 
    REG_49, REG_50, REG_51, REG_52, REG_53, REG_54, REG_55, REG_56, 
    REG_57, REG_58, REG_59, REG_60, REG_61, REG_62, REG_63, REG_64, 
    REG_65, REG_66, REG_67, REG_68, REG_69, REG_70, REG_71, REG_72, 
    REG_73, REG_74, REG_75, REG_76, REG_77, REG_78, REG_79, REG_80, 
    REG_81, REG_82, REG_83, REG_84, REG_85, REG_86, REG_87, REG_88, 
    REG_89, REG_90, REG_91, REG_92, REG_93, REG_94, REG_95, REG_96, 
    REG_97, REG_98, REG_99, REG_100, REG_101, REG_102, REG_103, REG_104, 
    REG_105, REG_106, REG_107, REG_108, REG_109, REG_110, REG_111, REG_112, 
    REG_113, REG_114, REG_115, REG_116, REG_117, REG_118, REG_119, REG_120, 
    REG_121, REG_122, REG_123, REG_124, REG_125, REG_126, REG_127,

    REG_S0, REG_S1, REG_S2, REG_S3, REG_S4, REG_S5, REG_S6, REG_S7, 
    REG_S8, REG_S9, REG_S10, REG_S11, REG_S12, REG_S13, REG_S14, REG_S15, 
    REG_S16, REG_S17, REG_S18, REG_S19, REG_S20, REG_S21, REG_S22, REG_S23, 
    REG_S24, REG_S25, REG_S26, REG_S27, REG_S28, REG_S29, REG_S30, REG_S31,

    REG_G0, REG_G1, REG_G2, REG_G3, REG_G4, REG_G5, REG_G6, REG_G7, 
    REG_G8, REG_G9, REG_G10, REG_G11, REG_G12, REG_G13, REG_G14, REG_G15, 
    REG_G16, REG_G17, REG_G18, REG_G19, REG_G20, REG_G21, REG_G22, REG_G23, 
    REG_G24, REG_G25, REG_G26, REG_G27, REG_G28, REG_G29, REG_G30, REG_G31,

    REG_C0, REG_C1, REG_C2, REG_C3, REG_C4, REG_C5, REG_C6, REG_C7, 
    REG_C8, REG_C9, REG_C10, REG_C11, REG_C12, REG_C13, REG_C14, REG_C15, 
    REG_C16, REG_C17, REG_C18, REG_C19, REG_C20, REG_C21, REG_C22, REG_C23, 
    REG_C24, REG_C25, REG_C26, REG_C27, REG_C28, REG_C29, REG_C30, REG_C31,

    REG_SR, 
};

sc_bool is_general_reg(sc_uint reg) {
    return reg <= REG_127 ? TRUE : FALSE;
}

sc_bool is_stream_reg(sc_uint reg) {
    return REG_S0 <= reg && reg <= REG_S31 ? TRUE : FALSE;
}

sc_bool is_generator_reg(sc_uint reg) {
    return REG_G0 <= reg && reg <= REG_G31 ? TRUE : FALSE;
}

sc_bool is_consumer_reg(sc_uint reg) {
    return REG_C0 <= reg && reg <= REG_C31 ? TRUE : FALSE;
}

#define operand_one(i)   ((i >> 16) & 0xFF)
#define operand_two(i)   ((i >> 8)  & 0xFF)
#define operand_three(i) ((i)  & 0xFF)

// these must be kept in the same order as opcodes[] below, otherwise
// directly lookup will value. i.e. enum is used to index into opcodes[].
enum {
    MOV, MOVL, READ, WRITE,
    JMP, JMPZ, JMPNZ, NOP, CMP, CALL, RET, HALT,
    ADD, SUB, MUL, FTOI,
    ADDF, SUBF, MULF, ITOF,
    LDL, PUSH, POP,

    // Loads          Stores      Size and Type
    LDR,              STR,      // Word (32 bits)
    LDRB,             STRB,     // Byte (8 bits)
    LDRH,             STRH,     // Halfword (16 bits)
    LDRSB,                      // Signed byte
    LDRSH,                      // Signed halfword

    SPAWN, YIELD, START,

    // external blocks, might not always be supported
    CONSOLE,

    // now instructions that are not in assembler, but are in the binary format
    // mostly for supporting streams
    STREAM, SETSF, SETSC,
    ATTACH, 
};

//---------------------------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------------------------

// instructions
static sc_uint instructions[MAX_INSRUCTIONS];
static sc_ushort instruction_count = 0;

// literal pool
static sc_uint memory_pool[MAX_MEMORY];
static sc_uchar *memory_pool_char = (sc_uchar*)&memory_pool[0];
static sc_ushort memory_count = 0;

static sc_ushort entry_point = 0;

#define USE_DEVICE_CONSOLE 0x1
#define USE_DEVICE_SCREEN (0x1 << 1)
static sc_uint device_capabilities = 0;
 
static sc_uint registers[127] = {0};
static sc_uchar flags = 0;

// stacks

static sc_uint* stacks[16];
static sc_uint next_stack = 0;

static sc_uint* current_stack;

// tasks

typedef struct {
    sc_uint id_;
    sc_uint pc_;
    sc_uint *stack_;
    sc_uint top_;
} task;

static task tasks[32];
static sc_uint next_task = 0;

#define set_cmpbit() (flags |= 1)
#define clear_cmpbit() (flags &= 0xFE)
#define is_cmpbit() (flags & 1)

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------



sc_uint allocate_stack(sc_uint size) {
    sc_uint i = next_stack++;
    stacks[i] = (sc_uint*)malloc(size * sizeof(sc_uint)); 
    return i;
}

sc_uint allocate_task(sc_uint pc) {
    sc_uint id = next_task++;
    sc_uint s = allocate_stack(DEFAULT_STACK_SIZE);

    task task = {
        .id_ = id,
        .pc_ = pc,
        .stack_ = stacks[s],
        .top_ = 0,
    };
    tasks[id] = task;

    return id;    
}

static inline void stack_push(sc_uint *s, sc_uint *top, sc_uint v) {
    s[*top] = v;
    *top = *top + 1;
}

static inline sc_uint stack_pop(sc_uint *s, sc_uint *top) {
    sc_uint v = s[*top];
    *top = *top - 1;
    return v;
}

sc_bool run() {
    // allocate main task
    sc_uint main_id = allocate_task(entry_point);
    
    task t = tasks[main_id];
    sc_uint pc = t.pc_;
    sc_uint* s = t.stack_;
    sc_uint top = t.top_;

    DEBUG("Entering loop\n");
    for(;;) {
        sc_uint i = instructions[pc];
        sc_uint opcode = (i >> 24) & 0xFF;
        switch(opcode) {
            case MOV: {
                DEBUG("MOV\n");
                sc_uint reg_op1 = operand_one(i);
                sc_uint reg_op2 = operand_two(i);
                registers[reg_op1] = registers[reg_op2];
                break;
            }
            case MOVL: {
                DEBUG("MOVL\n");
                sc_uint reg = operand_one(i);
                sc_uint offset = operand_two(i);
                registers[reg] = offset; //*((char*)&memory_pool[offset]);
                pc = pc + 1;
                break;
            }
            case JMP: {
                DEBUG("JMP\n");
                pc = (i >> 16) & 0xFF;
                break;
            }
            case JMPZ: {
                DEBUG("JMPZ\n");
                if (is_cmpbit()) {
                    pc = (i >> 16) & 0xFF;
                }
                else {
                    pc = pc + 1;
                }
                break;
            }
            case CMP: {
                DEBUG("CMP\n");
                sc_uint reg_op1 = operand_one(i);
                sc_uint reg_op2 = operand_two(i);
                if (registers[reg_op1] == registers[reg_op2]) {
                    set_cmpbit();
                }
                else {
                    clear_cmpbit();
                }
                pc = pc + 1;
                break;
            }
            case CALL: {
                DEBUG("CALL\n");
                sc_uint pc_target = (i >> 16) & 0xFF;
                // push return address
                stack_push(s, &top, pc+1);
                pc = pc_target;
                break;
            }
            case RET: {
                pc = stack_pop(s, &top);
                break;
            }
            case HALT: {
                DEBUG("HALT\n");
                return TRUE;
            }
            case ADD: {
                DEBUG("ADD\n");
                sc_uint reg_dst = operand_one(i);
                sc_uint reg_op1 = operand_two(i);
                sc_uint reg_op2 = operand_three(i);
                registers[reg_dst] = registers[reg_op1] + registers[reg_op2];
                pc = pc + 1;
                break;
            }
            case LDR: {
                DEBUG("LDR\n");
                sc_uint reg_dst = operand_one(i);
                sc_uint reg_addr = operand_two(i);
                registers[reg_dst] = *((sc_uint*)(&memory_pool_char[registers[reg_addr]]));
                pc = pc + 1;
                break;
            }
            case LDRSB: {
                DEBUG("LDRSB %u\n", i);
                sc_uint reg_dst = operand_one(i);
                sc_uint reg_addr = operand_two(i);
                registers[reg_dst] = (sc_int)(memory_pool_char[registers[reg_addr]]);
                pc = pc + 1;
                break;
            }
            case CONSOLE: {
                DEBUG("CONSOLE\n");
                sc_uint console_command = operand_one(i);
                sc_uint reg = operand_two(i);

                if (console_command == 0) {
                    // write command
                    write_console(registers[reg]);
                }
                pc = pc + 1;
                break;
            }
            default:
                sc_error("ERROR: unknown opcode %u\n", opcode);
                return FALSE;
        }
    }
    return TRUE;
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

sc_bool load(char * filename) {

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        sc_error("Error opening file %s\n", filename);
        return FALSE;
    }

    header header_data;

    if (fread(&header_data, sizeof(header), 1, file) != 1) {
        sc_error("Error reading header from file\n");
        return FALSE;
    }

    fseek(file, header_data.literals_start_, SEEK_SET);
    size_t bytes_read = fread(memory_pool, 1, header_data.literals_length_, file);
    if (bytes_read != header_data.literals_length_) {
        sc_error("Error reading literals from file\n");
        return FALSE;
    }
    memory_count = header_data.literals_length_ / 4;

    fseek(file, header_data.code_start_, SEEK_SET);
    bytes_read = fread(instructions, 1, header_data.code_length_, file);
    if (bytes_read != header_data.code_length_) {
        sc_error("Error reading code from file\n");
        return FALSE;
    }

    instruction_count = header_data.code_length_ / 4;
    entry_point = header_data.entry_point_;
    device_capabilities = header_data.capabilities_;

    return TRUE;
}

sc_int main(int argc, char** argv) {
    if(argc == 2 && scmp(argv[1], "-v", 2)) {
        sc_print("scem - SC Emulator, 30th Sept 2024.\n");
        return 1;
    }
	if(argc != 2) {
        sc_print("usage: scem [-v] input.scrom");
        return 1;
    }

    if(load(argv[1])) {
        // binary loaded

        // check capabilities and initialize any required devices
        if (device_capabilities & USE_DEVICE_CONSOLE) {
            // initialise console
            if (!has_console_device()) {
                sc_error("ERROR: required console device not supported\n");
            }

            if (!init_console()) {
                sc_error("ERROR: console would not initialize\n");
            }
        }
        if (device_capabilities & USE_DEVICE_SCREEN) {
            if (!has_screen_device()) {
                sc_error("ERROR: required screen device not supported\n");
            }
        }

        // initialize VM
        DEBUG("Entering VM\n");
        
        run();
    }

    return 0;
}