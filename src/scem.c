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
#include <lfqueue.h>
#include <raylib.h>
#include <time.h>
#include <unistd.h>

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
    MOV, MOVL, SREAD, SWRITE, SREADY,
    JMP, JMPZ, JMPNZ, NOP, CMP, CMPLT, CALL, RET, HALT,
    ADD, SUB, MUL, DIV, MOD, FTOI,
    ADDF, SUBF, MULF, ITOF,
    SHIFTR, SHIFTL, AND, OR, XOR,
    LDL, PUSH, POP,

    // Loads          Stores      Size and Type
    LDR,              STR,      // Word (32 bits)
    LDRB,             STRB,     // Byte (8 bits)
    LDRH,             STRH,     // Halfword (16 bits)
    LDRSB,                      // Signed byte
    LDRSH,                      // Signed halfword

    SPAWN, YIELD, START,

    // external blocks, might not always be supported
    CONSOLE, SCREEN, MOUSE,

    // now instructions that are not in assembler, but are in the binary format
    // mostly for supporting streams
    STREAM, SETSF, SETSC,
    ATTACH, AWAIT,
};

// Console device
#define CONSOLE_WRITE 0

enum { 
    SCREEN_RESIZE=0, SCREEN_PIXEL, SCREEN_FILL, SCREEN_RECT, 
    SCREEN_BLIT, SCREEN_PALETTE, SCREEN_BEGIN, SCREEN_END, SCREEN_COLOUR,
    SCREEN_MOVE, SCREEN_FONT, SCREEN_TEXT,
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
 
static struct timespec start_time;

static sc_uint running_queue = 0;

// streams

#define MAX_NUM_STREAMS 32
#define STREAM_REG_INDEX(r) (r - REG_S0)
#define GENERATOR_REG_INDEX(r) (r - REG_G0)

static sc_queue * streams[MAX_NUM_STREAMS];

#define MOUSE_GENERATOR 1

// stacks

static sc_uint* current_stack;
static sc_uint* current_registers;
static sc_uint current_flags;

// timing stuff

// tasks

typedef struct {
    sc_uint id_;
    sc_uint pc_;
    sc_uchar flags_;
    sc_uint *registers_;
    sc_uint rate_;
    sc_uint *stack_;
    sc_int top_;
} task;

static task tasks[16];
static sc_uint tasks_count = 0;

static inline void set_cmpbit(sc_uint *flags) {
    *flags |= 1;
}

static inline void clear_cmpbit(sc_uint *flags) {
    *flags &= 0xFE;
}

static inline sc_int is_cmpbit(sc_uint flags) {
    return (flags & 1);
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

sc_uint allocate_task(sc_uint pc, sc_uint rate) {
    sc_uint id = tasks_count++;
    sc_uint* s = (sc_uint*)malloc(DEFAULT_STACK_SIZE * sizeof(sc_uint));
    sc_uint* r = (sc_uint*)malloc(128 * sizeof(sc_uint));

    task task = {
        .id_        = id,
        .pc_        = pc,
        .flags_     = 0,
        .registers_ = r,
        .rate_      = rate,
        .stack_     = s,
        .top_       = -1,
    };
    tasks[id] = task;

    return id;    
}

static inline void stack_push(sc_uint *s, sc_uint *top, sc_uint v) {
    *top = *top + 1;
    s[*top] = v;
}

static inline sc_uint stack_pop(sc_uint *s, sc_uint *top) {
    sc_uint v = s[*top];
    *top = *top - 1;
    return v;
}

static inline sc_uint stack_peep(sc_uint *s, sc_uint *top) {
    sc_uint v = s[*top];
    return v;
}

void dump_stack(sc_uint *s, sc_int top) {
    while (top >= 0) {
        sc_error("s[%d] = %d\n", top, s[top]);
        top--;
    }
}

sc_bool run(sc_uint task_id, sc_bool screen_enabled) {
    task t = tasks[task_id];

    // current executing task
    sc_uint pc = t.pc_;
    sc_uint* s = t.stack_;
    sc_uint top = t.top_;
    sc_uint* registers = t.registers_;
    sc_uint flags = t.flags_;
    sc_uint rate = t.rate_;

    DEBUG("Entering loop\n");
    for(;;) {
        sc_uint i = instructions[pc];
        sc_uint opcode = (i >> 24) & 0xFF;

        sc_error("(%d: %d) - ", pc, i);
        if (screen_enabled) {
            // TODO: probably need to close any open files...
            if (screen_should_close()) {
                exit(1);
            }
        }
    //      READ, 
    // 
        
        switch(opcode) {
            case MOV: {
                DEBUG("MOV\n");
                sc_uint reg_op1 = operand_one(i);
                sc_uint reg_op2 = operand_two(i);
                registers[reg_op1] = registers[reg_op2];
                pc = pc + 1;
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
            //SREAD, SWRITE, SREADY,
            case SREAD: {
                DEBUG("SREAD\n");
                sc_uint sreg = STREAM_REG_INDEX(operand_two(i));
                sc_queue* s = streams[sreg];
                if (!is_empty(s)) {
                    sc_uint value = dequeue(s);
                    sc_uint reg  = operand_one(i);
                    registers[reg] = value;
                    set_cmpbit(&flags);
                }
                else {
                    clear_cmpbit(&flags);
                }

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
                if (is_cmpbit(flags)) {
                    pc = (i >> 16) & 0xFF;
                }
                else {
                    pc = pc + 1;
                }
                break;
            }
            case JMPNZ: {
                DEBUG("JMPNZ\n");
                if (!is_cmpbit(flags)) {
                    pc = (i >> 16) & 0xFF;
                }
                else {
                    pc = pc + 1;
                }
                break;
            }
            case NOP: {
                DEBUG("NOP\n");
                pc = pc + 1;
                break;
            }
            case CMP: {
                DEBUG("CMP\n");
                sc_uint reg_op1 = operand_one(i);
                sc_uint reg_op2 = operand_two(i);
                if (registers[reg_op1] == registers[reg_op2]) {
                    set_cmpbit(&flags);
                }
                else {
                    clear_cmpbit(&flags);
                }
                pc = pc + 1;
                break;
            }
            case CMPLT: {
                DEBUG("CMPLT\n");
                sc_uint reg_op1 = operand_one(i);
                sc_uint reg_op2 = operand_two(i);
                if (registers[reg_op1] < registers[reg_op2]) {
                    set_cmpbit(&flags);
                }
                else {
                    clear_cmpbit(&flags);
                }
                pc = pc + 1;
                break;
            }
            case CALL: {
                DEBUG("CALL\n");
                dump_stack(s, top);
                sc_uint pc_target = (i >> 16) & 0xFF;
                // push return address
                stack_push(s, &top, pc+1);
                pc = pc_target;
                break;
            }
            case RET: {
                //TODO: seperate return stack
                dump_stack(s, top);
                pc = stack_pop(s, &top);
                DEBUG("RET (%d)\n", pc);

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
            case SUB: {
                DEBUG("SUB\n");
                sc_uint reg_dst = operand_one(i);
                sc_uint reg_op1 = operand_two(i);
                sc_uint reg_op2 = operand_three(i);
                registers[reg_dst] = registers[reg_op1] - registers[reg_op2];
                pc = pc + 1;
                break;
            }
            case MUL: {
                DEBUG("MUL\n");
                sc_uint reg_dst = operand_one(i);
                sc_uint reg_op1 = operand_two(i);
                sc_uint reg_op2 = operand_three(i);
                registers[reg_dst] = registers[reg_op1] * registers[reg_op2];
                pc = pc + 1;
                break;
            }
            case FTOI: {
                sc_uint reg_dst = operand_one(i);
                sc_uint reg_op1 = operand_two(i);
                sc_int v = (sc_int)*((float*)&registers[reg_op1]);
                registers[reg_dst] = *((sc_uint*)&v);
                pc = pc + 1;
                break;
            }
            case ADDF: {
                DEBUG("ADDF\n");
                sc_uint reg_dst = operand_one(i);
                sc_uint reg_op1 = operand_two(i);
                sc_uint reg_op2 = operand_three(i);
                sc_float op1 = (sc_float)*((sc_int*)&registers[reg_op1]);
                sc_float op2 = (sc_float)*((sc_int*)&registers[reg_op2]);
                sc_float result = op1 + op2;
                registers[reg_dst] = *((sc_uint*)&result);
                pc = pc + 1;
                break;
            }
            case SUBF: {
                DEBUG("SUBF\n");
                sc_uint reg_dst = operand_one(i);
                sc_uint reg_op1 = operand_two(i);
                sc_uint reg_op2 = operand_three(i);
                sc_float op1 = (sc_float)*((sc_int*)&registers[reg_op1]);
                sc_float op2 = (sc_float)*((sc_int*)&registers[reg_op2]);
                sc_float result = op1 - op2;
                registers[reg_dst] = *((sc_uint*)&result);
                pc = pc + 1;
                break;
            }
            case MULF: {
                DEBUG("MULF\n");
                sc_uint reg_dst = operand_one(i);
                sc_uint reg_op1 = operand_two(i);
                sc_uint reg_op2 = operand_three(i);
                sc_float op1 = (sc_float)*((sc_int*)&registers[reg_op1]);
                sc_float op2 = (sc_float)*((sc_int*)&registers[reg_op2]);
                sc_float result = op1 * op2;
                registers[reg_dst] = *((sc_uint*)&result);
                pc = pc + 1;
                break;
            }
            case ITOF: {
                DEBUG("ITOF\n");
                sc_uint reg_dst = operand_one(i);
                sc_uint reg_op1 = operand_two(i);
                sc_float v = (sc_float)*((sc_int*)&registers[reg_op1]);
                registers[reg_dst] = *((sc_uint*)&v);
                pc = pc + 1;
                break;
            }
            // TODO: move all binary ops together and then use a lookup table to get the operation?
            case SHIFTR: {
                DEBUG("SHIFTR\n");
                sc_uint reg_dst = operand_one(i);
                sc_uint reg_op1 = operand_two(i);
                sc_uint reg_op2 = operand_three(i);
                sc_uint op1 = (sc_uint)*((sc_int*)&registers[reg_op1]);
                sc_uint op2 = (sc_uint)*((sc_int*)&registers[reg_op2]);
                sc_uint result = op1 >> op2;
                registers[reg_dst] = *((sc_uint*)&result);
                pc = pc + 1;
                break;
            } 
            case SHIFTL: {
                DEBUG("SHIFTL\n");
                sc_uint reg_dst = operand_one(i);
                sc_uint reg_op1 = operand_two(i);
                sc_uint reg_op2 = operand_three(i);
                sc_uint op1 = (sc_uint)*((sc_int*)&registers[reg_op1]);
                sc_uint op2 = (sc_uint)*((sc_int*)&registers[reg_op2]);
                sc_uint result = op1 << op2;
                registers[reg_dst] = *((sc_uint*)&result);
                pc = pc + 1;
                break;
            }
            case AND: {
                DEBUG("AND\n");
                sc_uint reg_dst = operand_one(i);
                sc_uint reg_op1 = operand_two(i);
                sc_uint reg_op2 = operand_three(i);
                sc_uint op1 = (sc_uint)*((sc_int*)&registers[reg_op1]);
                sc_uint op2 = (sc_uint)*((sc_int*)&registers[reg_op2]);
                sc_uint result = op1 & op2;
                registers[reg_dst] = *((sc_uint*)&result);
                pc = pc + 1;
                break;
            } 
            case OR: {
                DEBUG("OR\n");
                sc_uint reg_dst = operand_one(i);
                sc_uint reg_op1 = operand_two(i);
                sc_uint reg_op2 = operand_three(i);
                sc_uint op1 = (sc_uint)*((sc_int*)&registers[reg_op1]);
                sc_uint op2 = (sc_uint)*((sc_int*)&registers[reg_op2]);
                sc_uint result = op1 | op2;
                registers[reg_dst] = *((sc_uint*)&result);
                pc = pc + 1;
                break;
            }
            case XOR: {
                DEBUG("XOR\n");
                sc_uint reg_dst = operand_one(i);
                sc_uint reg_op1 = operand_two(i);
                sc_uint reg_op2 = operand_three(i);
                sc_uint op1 = (sc_uint)*((sc_int*)&registers[reg_op1]);
                sc_uint op2 = (sc_uint)*((sc_int*)&registers[reg_op2]);
                sc_uint result = op1 ^ op2;
                registers[reg_dst] = *((sc_uint*)&result);
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
            case STR: {
                DEBUG("STR\n");
                sc_uint reg_addr = operand_one(i);
                sc_uint reg_src = operand_two(i);
                *((sc_uint*)(&memory_pool_char[registers[reg_addr]])) = registers[reg_src];
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
            // other load and stores
            //SPAWN, YIELD, START,
            case SPAWN: {
                DEBUG("SPAWN\n");
                sc_uint task_rate = registers[operand_one(i)];
                sc_uint task_pc   = operand_two(i);
                sc_uint id   = allocate_task(task_pc, task_rate);
                // add to running queue
                running_queue = id;
                pc = pc + 1;
                break;
            }
            case YIELD: {
                DEBUG("YIELD\n");
                //pc = tasks[running_queue].pc_;
                struct timespec end_time;
                clock_gettime(CLOCK_MONOTONIC, &end_time);
                // Calculate the elapsed time in nanoseconds
                long long elapsed_time_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000LL + end_time.tv_nsec - start_time.tv_nsec;

                pc = pc + 1;
                long long time_left = (1000000000 / rate) - elapsed_time_ns;
                if (time_left > 0) {
                    struct timespec sleep_time;
                    sleep_time.tv_sec = 0;
                    sleep_time.tv_nsec = time_left;
                    nanosleep(&sleep_time, NULL);
                } 
                start_time = end_time;
                break;
            }
            case START: {
                DEBUG("START\n");
                t = tasks[running_queue];

                // current executing task
                pc = t.pc_;
                s = t.stack_;
                top = t.top_;
                registers = t.registers_;
                flags = t.flags_;
                rate = t.rate_;
                break;
            }
            case CONSOLE: {
                DEBUG("CONSOLE\n");
                sc_uint console_command = operand_one(i);
                sc_uint reg = operand_two(i);

                if (console_command == CONSOLE_WRITE) {
                    // write command
                    write_console(registers[reg]);
                }
                pc = pc + 1;
                break;
            }
            case SCREEN: {
                DEBUG("SCREEN\n");
                sc_uint screen_command = operand_one(i);

                switch (screen_command) {
                    case SCREEN_RESIZE: {
                        // create/resize window command
                        sc_uint reg_w = operand_two(i);
                        sc_uint reg_h = operand_three(i);
                        sc_uint w = registers[reg_w];
                        sc_uint h = registers[reg_h];
                        screen_resize(w, h, 1);
                        break;   
                    }
                    case SCREEN_PIXEL: {
                        sc_uint reg_x = operand_two(i);
                        sc_uint reg_y = operand_three(i);
                        sc_uint x = registers[reg_x];
                        sc_uint y = registers[reg_y];
                        screen_move(x,y);
                        screen_pixel();
                        break;
                    }
                    case SCREEN_FILL: {
                        screen_fill();
                        break;
                    }
                    case SCREEN_RECT: {
                        sc_uint reg_x = operand_two(i);
                        sc_uint reg_y = operand_three(i);
                        sc_uint x = registers[reg_x];
                        sc_uint y = registers[reg_y];
                        screen_rect(x,y);
                        break;
                    }
                    case SCREEN_BEGIN: {
                        screen_process_events();
                        screen_begin_frame();
                        // screen_colour(0);
                        // screen_fill();
                        break;
                    }
                    case SCREEN_END: {
                        screen_end_frame();
                        break;
                    }
                    case SCREEN_COLOUR: {
                        sc_uint reg_c = operand_two(i);
                        sc_uint c = registers[reg_c];
                        screen_colour(c);
                        break;
                    }
                    case SCREEN_MOVE: {
                        sc_uint reg_x = operand_two(i);
                        sc_uint reg_y = operand_three(i);
                        sc_uint x = registers[reg_x];
                        sc_uint y = registers[reg_y];
                        screen_move(x,y);
                        break;
                    }
                    case SCREEN_FONT: {
                        sc_uint reg_index = operand_two(i);
                        sc_uint reg_filename = operand_three(i);
                        sc_uint index = registers[reg_index];
                        sc_char* filename = (sc_char*)(&memory_pool_char[registers[reg_filename]]);
                        sc_ushort point = (sc_ushort)stack_peep(s, &top);
                        screen_font(index, filename, point);
                        break;
                    }
                    case SCREEN_TEXT: {
                        sc_uint reg_index = operand_two(i);
                        sc_uint reg_str = operand_three(i);
                        sc_uint index = registers[reg_index];
                        sc_char* str = (sc_char*)(&memory_pool_char[registers[reg_str]]);
                        screen_text(index, str);
                        break;
                    }
                    default: {
                        sc_error("ERROR: unknown screen command %d\n", screen_command);
                        break;
                    }
                }
                pc = pc + 1;
                break;
            }
            case PUSH: {
                DEBUG("PUSH\n");
                sc_uint reg_op1 = operand_one(i);
                stack_push(s, &top, registers[reg_op1]);
                pc = pc + 1;
                break;
            }
            case POP: {
                DEBUG("POP\n");
                sc_uint reg_op1 = operand_one(i);
                registers[reg_op1] = stack_pop(s, &top);
                pc = pc + 1;
                break;
            }
            case STREAM: {
                DEBUG("STREAM\n");
                sc_uint sreg = STREAM_REG_INDEX(operand_one(i));
                sc_uint size = operand_two(i);
                
                if (size != 32) {
                    sc_error("ERROR: stream size not 32\n");
                    return FALSE;
                }
                streams[sreg] = allocate_queue(1024);

                pc = pc + 1;
                break;
            }
            case SETSF: {
                DEBUG("SETSF\n");
                pc = pc + 1;
                break;
            }
            case SETSC: {
                DEBUG("SETSC\n");
                pc = pc + 1;
                break;
            }
            case ATTACH: {
                DEBUG("ATTACH\n");
                sc_uint greg = GENERATOR_REG_INDEX(operand_one(i));
                sc_uint sreg = STREAM_REG_INDEX(operand_two(i));
                sc_uint reg = operand_three(i);

                if (greg == MOUSE_GENERATOR) {
                    // connect mouse generator to stream
                    attach_mouse_generator(streams[sreg]);
                }
                pc = pc + 1;
                break;
            }
            default:
                sc_error("ERROR: unknown opcode %u\n", opcode);
                for (;;) {

                }
                return FALSE;
        }
        // screen_end_frame();
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
        sc_bool screen_enabled = FALSE;
        if (device_capabilities & USE_DEVICE_SCREEN) {
            if (!has_screen_device()) {
                sc_error("ERROR: required screen device not supported\n");
            }
            DEBUG("Initializing screen\n");
            init_screen();
            screen_set_rate(30); // default screen rate
            screen_enabled = TRUE;
        }

        // initialize VM
        DEBUG("Entering VM\n");
        
        // allocate main task, 0 rate means fast as possible
        sc_uint main_id = allocate_task(entry_point, 0);

        // initialize clock
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        run(main_id, screen_enabled);

        if (screen_enabled) {
            delete_screen();
        }
    }

    return 0;
}