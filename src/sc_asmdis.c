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

//-----------------------------------------------------------------------------------------------
// limits
//-----------------------------------------------------------------------------------------------

#define MAX_DIGITS_IN_NUM 1024


// max 32K 32-bit instructions
#define MAX_INSRUCTIONS 1024 * 32 

// max 8K 32-bit literals
#define MAX_LITERALS 1024 * 8

// max 4K labels
#define MAX_LABELS 1024 * 4

#define MAX_LABEL_LENGTH 256

#define MAX_REGISTER_NUM 255

#define MAX_OPCODE_SIZE 7

#define MAX_TOPLEVEL_SIZE 10
#define MAX_DEVICE_SIZE 10

//-----------------------------------------------------------------------------------------------
// some useful macros
//-----------------------------------------------------------------------------------------------

#define out_msg(s) sc_print("%s", s)
#define error_top(id, msg) printf("%s: %s\n", id, msg)
#define error_asm(id) printf("%s: %s in @%s, %s:%d.\n", id, token, scope, ctx->path, ctx->line)
#define error_ref(id) printf("%s: %s, %s:%d\n", id, r->name, r->data, r->line)

//-----------------------------------------------------------------------------------------------
// Types
//-----------------------------------------------------------------------------------------------


// tokens and classes
enum {
  Float, Int, Char, String,
};

typedef struct {
    sc_char *str_;
    sc_int hash_;
} string;

typedef struct {
    string name_;
    sc_int offset_;
} label;

typedef union {
    label * label_;
    sc_ushort literal_;
    sc_uchar operand_;
} operand_option;

enum {
    OP_Label, OP_Reg, OP_Lit, OP_Off
};

typedef struct {
    sc_int type_;
    operand_option op_;
} operand;

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

typedef struct {
    sc_char str_[MAX_OPCODE_SIZE+1];
    sc_int reg_;
} reserved_register;

const reserved_register reserved_regs[] = { 
    {"R0", REG_0}, {"R1", REG_1}, {"R2", REG_2}, {"R3", REG_3}, 
    {"R4", REG_4}, {"R5", REG_5}, {"R6", REG_6}, {"R7", REG_7}, 
    {"R8", REG_8}, {"R9", REG_9}, {"R10", REG_10}, {"R11", REG_11}, 
    {"R12", REG_12}, {"R13", REG_13}, {"R14", REG_14}, {"R15", REG_15}, 
    {"R16", REG_16}, {"R17", REG_17}, {"R18", REG_18}, {"R19", REG_19}, 
    {"R20", REG_20}, {"R21", REG_21}, {"R22", REG_22}, {"R23", REG_23}, 
    {"R24", REG_24}, {"R25", REG_25}, {"R26", REG_26}, {"R27", REG_27}, 
    {"R28", REG_28}, {"R29", REG_29}, {"R30", REG_30}, {"R31", REG_31}, 
    {"R32", REG_32}, {"R33", REG_33}, {"R34", REG_34}, {"R35", REG_35}, 
    {"R36", REG_36}, {"R37", REG_37}, {"R38", REG_38}, {"R39", REG_39}, 
    {"R40", REG_40}, {"R41", REG_41}, {"R42", REG_42}, {"R43", REG_43}, 
    {"R44", REG_44}, {"R45", REG_45}, {"R46", REG_46}, {"R47", REG_47}, 
    {"R48", REG_48}, {"R49", REG_49}, {"R50", REG_50}, {"R51", REG_51}, 
    {"R52", REG_52}, {"R53", REG_53}, {"R54", REG_54}, {"R55", REG_55}, 
    {"R56", REG_56}, {"R57", REG_57}, {"R58", REG_58}, {"R59", REG_59}, 
    {"R60", REG_60}, {"R61", REG_61}, {"R62", REG_62}, {"R63", REG_63}, 
    {"R64", REG_64}, {"R65", REG_65}, {"R66", REG_66}, {"R67", REG_67}, 
    {"R68", REG_68}, {"R69", REG_69}, {"R70", REG_70}, {"R71", REG_71}, 
    {"R72", REG_72}, {"R73", REG_73}, {"R74", REG_74}, {"R75", REG_75}, 
    {"R76", REG_76}, {"R77", REG_77}, {"R78", REG_78}, {"R79", REG_79}, 
    {"R80", REG_80}, {"R81", REG_81}, {"R82", REG_82}, {"R83", REG_83}, 
    {"R84", REG_84}, {"R85", REG_85}, {"R86", REG_86}, {"R87", REG_87}, 
    {"R88", REG_88}, {"R89", REG_89}, {"R90", REG_90}, {"R91", REG_91}, 
    {"R92", REG_92}, {"R93", REG_93}, {"R94", REG_94}, {"R95", REG_95}, 
    {"R96", REG_96}, {"R97", REG_97}, {"R98", REG_98}, {"R99", REG_99}, 
    {"R100", REG_100}, {"R101", REG_101}, {"R102", REG_102}, {"R103", REG_103}, 
    {"R104", REG_104}, {"R105", REG_105}, {"R106", REG_106}, {"R107", REG_107}, 
    {"R108", REG_108}, {"R109", REG_109}, {"R110", REG_110}, {"R111", REG_111},
    {"R112", REG_112}, {"R113", REG_113}, {"R114", REG_114}, {"R115", REG_115}, 
    {"R116", REG_116}, {"R117", REG_117}, {"R118", REG_118}, {"R119", REG_119}, 
    {"R120", REG_120}, {"R121", REG_121}, {"R122", REG_122}, {"R123", REG_123}, 
    {"R124", REG_124}, {"R125", REG_125}, {"R126", REG_126}, {"R127", REG_127},
    
    {"S0", REG_S0}, {"S1", REG_S1}, {"S2", REG_S2}, {"S3", REG_S3}, 
    {"S4", REG_S4}, {"S5", REG_S5}, {"S6", REG_S6}, {"S7", REG_S7}, 
    {"S8", REG_S8}, {"S9", REG_S9}, {"S10", REG_S10}, {"S11", REG_S11}, 
    {"S12", REG_S12}, {"S13", REG_S13}, {"S14", REG_S14}, {"S15", REG_S15}, 
    {"S16", REG_S16}, {"S17", REG_S17}, {"S18", REG_S18}, {"S19", REG_S19}, 
    {"S20", REG_S20}, {"S21", REG_S21}, {"S22", REG_S22}, {"S23", REG_S23}, 
    {"S24", REG_S24}, {"S25", REG_S25}, {"S26", REG_S26}, {"S27", REG_S27}, 
    {"S28", REG_S28}, {"S29", REG_S29}, {"S30", REG_S30}, {"S31", REG_S31},

    {"G0", REG_G0}, {"G1", REG_G1}, {"G2", REG_G2}, {"G3", REG_G3}, 
    {"G4", REG_G4}, {"G5", REG_G5}, {"G6", REG_G6}, {"G7", REG_G7}, 
    {"G8", REG_G8}, {"G9", REG_G9}, {"G10", REG_G10}, {"G11", REG_G11}, 
    {"G12", REG_G12}, {"G13", REG_G13}, {"G14", REG_G14}, {"G15", REG_G15}, 
    {"G16", REG_G16}, {"G17", REG_G17}, {"G18", REG_G18}, {"G19", REG_G19}, 
    {"G20", REG_G20}, {"G21", REG_G21}, {"G22", REG_G22}, {"G23", REG_G23}, 
    {"G24", REG_G24}, {"G25", REG_G25}, {"G26", REG_G26}, {"G27", REG_G27}, 
    {"G28", REG_G28}, {"G29", REG_G29}, {"G30", REG_G30}, {"G31", REG_G31}, 

    {"C0", REG_C0}, {"C1", REG_C1}, {"C2", REG_C2}, {"C3", REG_C3}, 
    {"C4", REG_C4}, {"C5", REG_C5}, {"C6", REG_C6}, {"C7", REG_C7}, 
    {"C8", REG_C8}, {"C9", REG_C9}, {"C10", REG_C10}, {"C11", REG_C11}, 
    {"C12", REG_C12}, {"C13", REG_C13}, {"C14", REG_C14}, {"C15", REG_C15}, 
    {"C16", REG_C16}, {"C17", REG_C17}, {"C18", REG_C18}, {"C19", REG_C19}, 
    {"C20", REG_C20}, {"C21", REG_C21}, {"C22", REG_C22}, {"C23", REG_C23}, 
    {"C24", REG_C24}, {"C25", REG_C25}, {"C26", REG_C26}, {"C27", REG_C27}, 
    {"C28", REG_C28}, {"C29", REG_C29}, {"C30", REG_C30}, {"C31", REG_C31},  

    { "SR", REG_SR },
};

void print_operand(operand op) {
    if (op.type_ == OP_Reg) {
        sc_print("%s", reserved_regs[op.op_.operand_].str_);
    }
    else if (op.type_ == OP_Label) {
        sc_print("_%s (%d)", op.op_.label_->name_.str_, op.op_.label_->offset_);
    }
    else if (op.type_ == OP_Lit) {
        sc_print("%d", op.op_.literal_);
    }
}

typedef struct  {
    sc_int opcode_;
    sc_int operand_count_;
    operand operands_[3];
} instruction;

operand make_operand(sc_int type, operand_option op) {
    operand o = { type, op};
    return o;
}

instruction make_instruction(sc_int opcode, sc_int operand_count, operand* operands) {
    instruction i;
    i.opcode_ = opcode;
    i.operand_count_ = operand_count;
    while (--operand_count >= 0) {
        i.operands_[operand_count] = operands[operand_count];
    }
    return i;
}

enum { TL_Stream, TL_Task };

// typedef struct {
//     string name_;
//     sc_ushort code_offset_;
// } task;

// typedef struct {
//     string name_;
// } stream;

// typedef struct {

// } toplevel;

//-----------------------------------------------------------------------------------------------
// Streams
//-----------------------------------------------------------------------------------------------



//-----------------------------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------------------------

static sc_int token;
static sc_char* src_buffer;
static sc_uint line = 1;
static sc_char label_prefix[MAX_LABEL_LENGTH] = { 0 };

#define ENTRY_NOTDEFINED -1
static sc_int entry_point = ENTRY_NOTDEFINED;

static sc_uint device_capabilities = 0;
#define USE_DEVICE_CONSOLE 0x1
#define USE_DEVICE_SCREEN (0x1 << 1) 

// instructions
static instruction instructions[MAX_INSRUCTIONS];
static sc_ushort instruction_count = 0;

// literal pool
static sc_uint literals[MAX_LITERALS];
static sc_ushort literal_count = 0;

// label pool
static label labels[MAX_LABELS];
static sc_ushort label_count = 0;

#define push_literal(l) (literals[(sc_int)literal_count]=*((sc_uint*)&l), literal_count++)
#define get_literal(offset) (literals[(sc_int)offset])

#define push_instruction(i) (instructions[instruction_count++] = i)

sc_ushort push_string_literal(sc_char *src, sc_int len) {
    // be carefull to leave literals 4-byte aligned
    //sc_char* lit_pool = (sc_char*)literals;
    sc_ushort literal_count_tmp = literal_count;

    mcopy(src, (sc_char*)(literals+literal_count), len);

    *(((sc_char*)(literals+literal_count))+len) = '\0';
    len = len + 1;
    // inc literal count, making sure to pad 4-byte alignment, if necessary
    literal_count = literal_count + ((len+1)/4) + ((len+1) % 4 > 0 ? 1 : 0);

    return literal_count_tmp;
}

//-----------------------------------------------------------------------------------------------
// OPCODES
//-----------------------------------------------------------------------------------------------

typedef struct {
    sc_char str_[MAX_OPCODE_SIZE+1];
    sc_int opcode_;
    sc_int num_operands_;
} opcode;

// individual instruction opcode

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

const opcode opcodes[] = {
    // Move registers and literals 
    { "MOV", MOV, 2}, { "MOVL", MOVL, 2}, 
    
    // Streams
    {"READ", READ, 2}, { "WRITE", WRITE, 2},
    
    // Control flow (expect tasks)
    { "JMP", JMP, 1}, {"JMPZ", JMPZ, 1}, {"JMPNZ", JMPNZ, 1}, {"NOP", NOP, 0}, {"CMP", CMP, 2}, 
    {"CALL", CALL, 1}, {"RET", RET, 0}, {"HALT", HALT, 0},


    // arithmeric
    {"ADD", ADD, 3}, {"SUB", SUB, 3}, {"MUL", ADD, 3}, {"FTOI", FTOI, 2},
    {"ADDF", ADD, 3}, {"SUBF", SUBF, 3}, {"MULF", ADD, 3}, {"ITOF", ITOF, 2},
    {"LDL", LDL, 1}, {"PUSH", PUSH, 1}, {"POP", POP, 1},

    // LD/ST
    {"LDR", LDR, 2},  {"STR", STR, 2}, {"LDRB", LDRB, 2}, {"STRB", STRB, 2},
    {"LDRH", LDRH, 2 }, {"STRH", STRH, 2},{"LDRSB", LDRSB, 2},  {"LDRSH", LDRSH, 2},                   
    
    // TASKS
    {"SPAWN", SPAWN, 2}, {"YIELD", YIELD, 0}, {"START", START, 0},

    // Loads            Stores      Size and Type
    // LDR              STR         Word (32 bits)
    // LDRB             STRB        Byte (8 bits)
    // LDRH             STRH        Halfword (16 bits)
    // LDRSB                        Signed byte
    // LDRSH                        Signed halfword
    // LDM              STM         Multiple words

    {"CONSOLE", CONSOLE, 2}, // CONSOLE <console-op> REG

    // stream insructions
    {"STREAM", STREAM, 4}, {"SETSF", SETSF, 2}, {"SETSC", SETSC, 2}, 
    {"ATTACH", ATTACH, 3},
 };

sc_bool match_opcode(sc_char *op, opcode * dst_opcode) {
    sc_int len = slen(op);

    // return false in the case that it is a stream instruction
    // these are handled seperatly and not allowed to appear in standard
    // instruction stream for assembler
    if (scmp("STREAM", op, len) || scmp("ATTACH", op, len)) {
        return FALSE;
    }

    for (sc_int i = 0; i < sizeof(opcodes) / sizeof(opcode); i++) {
        if (scmp(opcodes[i].str_, op, len)) {
            if (dst_opcode) {
                *dst_opcode = opcodes[i];
            }
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * @brief print instruction to stdout
 * 
 * @param instruction to print.
 */
void print_instruction(instruction inst) {
    sc_print("%s ", opcodes[inst.opcode_].str_);
    for (sc_int j = 0; j < inst.operand_count_; j++) {
        print_operand(inst.operands_[j]);
        sc_print(" ");
    }
}

/**
 * @brief print all insructions to stdout
 */
void print_instructions() {
    for (sc_int i = 0; i < instruction_count; i++) {
        sc_print("%d\t", i);
        print_instruction(instructions[i]);
        sc_print("\n");
    }
}

sc_uint encode_operand(operand operand) {
    if (operand.type_ == OP_Reg) {
        return operand.op_.operand_;
    }
    else if (operand.type_ == OP_Lit) {
        return operand.op_.literal_ * sizeof(sc_int); // convert to byte offsets
    }
    else {
        return operand.op_.label_->offset_;
    }
}

sc_int encode_instruction(instruction inst) {
    sc_uint i = 0;
    i = inst.opcode_ << 24;
    sc_uint shift = 16;

    if (inst.operand_count_ > 0) {
        sc_uint o = encode_operand(inst.operands_[0]);
        i |= (o & 0xFF) << 16;

        if (inst.operand_count_ > 1) {
            o = encode_operand(inst.operands_[1]);
            i |= (o & 0xFF) << 8;
        }
        
        if (inst.operand_count_ > 2) {
            o = encode_operand(inst.operands_[2]);
            i |= (o & 0xFF);
        }
    }
    
    return i;
}

void print_encode_instruction(sc_uint inst, char* prefix) {
    sc_uint i = inst;
    sc_uint opcode = (i >> 24) & 0xFF;
    sc_print("%s%s\t", prefix, opcodes[opcode].str_);

    if (opcodes[opcode].num_operands_ > 0) {
        sc_print("%u\t", (i >> 16) & 0xFF);
        
        if (opcodes[opcode].num_operands_ > 1) {
            sc_print("%u\t", (i >> 8) & 0xFF);
        }
        
        if (opcodes[opcode].num_operands_ > 2) {
            sc_print("%u\t", i & 0xFF);
        }
    }
    sc_print("\n");
}

//-----------------------------------------------------------------------------------------------
// DEVICES
//-----------------------------------------------------------------------------------------------

// Console device
#define CONSOLE_WRITE 0

//-----------------------------------------------------------------------------------------------
// Labels
//-----------------------------------------------------------------------------------------------

#define LABEL_FORWARD -1

/**
 * @brief make label.
 *
 * @param name of label.
 * @param offset of label, which can be LABEL_UNDEFINED.
 * @return pointer to constructed label
 */
label* make_label(sc_char* l, sc_int loc) {
    label lab = {{l, 0}, loc};
    labels[label_count] = lab;
    return &labels[label_count++];
}

/**
 * @brief is_label_defined.
 *
 * @param label to check is defined.
 * @param location to return label if found.
 * @return true if successful, otherwise false.
 */
sc_bool is_label_defined(sc_char* l, label** dst_label) {
    sc_int len = slen(l);
    for (sc_int i = 0; i < label_count; i++) {
        if (scmp(labels[i].name_.str_, l, len)) {
            if (dst_label) {
                *dst_label = &labels[i];
            }
            return TRUE;
        }
    }
    return FALSE;
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------

/**
 * @brief read file into buffer.
 *
 * @param filename to be read into buffer.
 * @return true if successful, otherwise false.
 */
sc_bool read(const char * filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        sc_error("Error opening file: %s\n", filename);
        return FALSE;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    src_buffer = (char *)malloc(file_size + 1);
    if (src_buffer == NULL) {
        sc_error("Error allocating memory\n");
        fclose(file);
        return FALSE;
    }

    size_t bytes_read = fread(src_buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        sc_error("Error reading file: %s\n", filename);
        free(src_buffer);
        fclose(file);
        return FALSE;
    }

    src_buffer[file_size] = '\0';
    fclose(file);

    return TRUE;
}

//-----------------------------------------------------------------------------------------------
// Parsering functions
//-----------------------------------------------------------------------------------------------

/**
 * @brief strip whitespace from source buffer
 * 
 */
void strip_whitespace() {
    while ((token == ' ' || token == '\t') && token != 0 && token != '\n') {
        token = *src_buffer++; 
    }
}


/**
 * @brief parse unsigned int.
 *
 * @param string containing input.
 * @param address to store parsed value.
 * @return true if successful, otherwise false.
 */
sc_bool parse_unsigned_int(const sc_char *str, sc_uint *dst) {
    sc_char *endptr;
    *dst = strtoul(str, &endptr, 10);

    if (*endptr != '\0') {
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief parse int.
 *
 * @param string containing input.
 * @param address to store parsed value.
 * @return true if successful, otherwise false.
 */
sc_int parse_int(const sc_char *str, sc_int *dst) {
    sc_char *endptr;
    *dst = strtol(str, &endptr, 10);

    if (*endptr != '\0') {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief parse float.
 *
 * @param string containing input.
 * @param address to store parsed value.
 * @return true if successful, otherwise false.
 */
sc_float parse_float(const char *str, sc_float *dst) {
    char *endptr;
    *dst = strtod(str, &endptr);

    if (*endptr != '\0') {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief parse a literal
 * 
 * @param pointer to where offset into literal pool will be returned, if not null 
 * @return true if successful, otherwise false.
 */
sc_bool parse_literal(sc_ushort * lit_offset) {
    strip_whitespace();

     // parse literal
    token = *src_buffer++;
    sc_bool is_negative = FALSE;
    if (token == '-') {
        is_negative = TRUE;
        token = *src_buffer++;
    }

    sc_char digits[MAX_DIGITS_IN_NUM];
    sc_int count = 0;
    sc_bool is_float = FALSE;

    while (token != ' ' && token != 0 && token != '\n') {
        if (is_digit(token)) {
            digits[count++] = token;
        }
        else if (token == '.' && !is_float) {
            is_float = TRUE;
            digits[count++] = token;
        }
        else {
            sc_error("ERROR: line(%d) invalid literal\n", line);
            return FALSE;
        } 
        token = *src_buffer++;
    }
    
    if ((count == 0 && !is_float) || (count == 1 && is_float)) {
        sc_error("ERROR: line(%d) invalid literal\n", line);
        return FALSE;
    }

    digits[count] = '\0';

    sc_ushort lo;
    if (is_float) {
        sc_float value;
        parse_float(digits, &value);
        if (is_negative) {
            value = value * -1.0f;
        }
        lo = push_literal(value);
    }
    else if (is_negative) {
        sc_int value;
        parse_int(digits, &value);
        value = value * -1;
        lo = push_literal(value);
    }
    else {
        sc_uint value;
        parse_unsigned_int(digits, &value);
        lo = push_literal(value);
    }

    if (lit_offset) {
        *lit_offset = lo;
    }
    
    return TRUE;
}

/**
 * @brief parse a literal
 * 
 * @param pointer to where offset into literal pool will be returned, if not null 
 * @return true if successful, otherwise false.
 */
sc_bool parse_string_literal(sc_ushort * lit_offset) {
    strip_whitespace();

     // parse literal
    token = *src_buffer++;
    
    sc_char * string_start = src_buffer;

    sc_char buffer[1024];
    sc_int buffer_count = 0;

    while (token != '"' && token != 0 && token != '\n') {
        //sc_print("%c (%c) ", token, *(src_buffer+1));
        if (token == '\\' && *(src_buffer) == 'n') {
            src_buffer++;
            buffer[buffer_count++] = '\n';
        }
        else {
            //sc_print("%c", *(src_buffer+1));
            buffer[buffer_count++] = token;
        }
        token = *src_buffer++;
    }
    sc_print("\n");

    if (token != '"') {
            sc_error("ERROR: line(%d) invalid string literal\n", line);
            return FALSE;
    }

    // sc_size_t string_length =  (src_buffer-1) - string_start;
    // sc_char * string = sc_malloc(string_length * sizeof(sc_char));
    // mcopy(string_start, string, string_length); 

    sc_ushort lo = push_string_literal(buffer, buffer_count);

    if (lit_offset) {
        *lit_offset = lo;
    }

    token = *src_buffer++;

    return TRUE;
}


/**
 * @brief parse a label, including definition and inline
 * 
 * @param pointer to where is defined will be returned, if not null 
 * @return true if successful, otherwise false.
 */
sc_bool parse_label(label **dst_label, sc_bool should_be_definition) {
    // parse literal
    strip_whitespace();

    sc_char * label_start = src_buffer;

    while (token != ' ' && token != 0 && token != '\n' & token != ':') {
        if (!is_alpha(token) && !is_digit(token) && token != '_') {
            sc_error("ERROR: line(%d) invalid label\n", line);
            return FALSE;
        }
        token = *src_buffer++;
    }

    sc_int prefix_length = slen(label_prefix);
    sc_size_t label_length = prefix_length + (src_buffer-1) - label_start;
    sc_char * lab = sc_malloc(label_length+1 * sizeof(sc_char));

    // check if defined as top_level
    sc_size_t label_tmp_length =  (src_buffer-1) - label_start;
    mcopy(label_start, lab, label_tmp_length); 
    lab[label_tmp_length] = '\0';
    label* dst;
    if (is_label_defined(lab, &dst)) {
        if (dst_label) {
            *dst_label = dst;
        }
        return TRUE;
    }

    mcopy(label_prefix, lab, prefix_length);
    mcopy(label_start, lab+prefix_length, label_length);
    lab[label_length] = '\0';
    
    if (should_be_definition) {
        if (token == ':') {
            if (is_label_defined(lab, &dst)) {
                if (dst->offset_ != LABEL_FORWARD) {
                    sc_error("ERROR: line(%d) duplicate label %s\n", line, lab);
                    return FALSE;
                }
                dst->offset_ = instruction_count;
            }
            else {
                dst = make_label(lab, instruction_count);
            }
        }
        else {
            sc_error("ERROR: line(%d) expected :\n", line);
            return FALSE;
        }
    }
    else {
        if (!is_label_defined(lab, &dst)) {
            // not previously defined or referenced
            dst = make_label(lab, LABEL_FORWARD);
        }
    }

    if (dst_label) {
        *dst_label = dst;
    }

    return TRUE;
}

/**
 * @brief parse instruction operand
 * 
 * @param pointer to where parsed operand will be placed (can be null) 
 * @return true if successful, otherwise false.
 */
sc_bool parse_operand(operand* dst_operand) {
    strip_whitespace();
    // while (token != ' ' && token != 0 && token != '\n') {
        if (token == 'R') {
            // register
            sc_char digits[MAX_DIGITS_IN_NUM];
            sc_int count = 0;
            token = *src_buffer++;

            while (token != ' ' && token != 0 && token != '\n' && is_digit(token)) {
                digits[count++] = token;
                token = *src_buffer++;
            }

            if (count == 0) {
                sc_error("ERROR: line(%d) invalid register\n", line);
                return FALSE;
            }

            digits[count] = '\0';            
            sc_uint value;
            parse_unsigned_int(digits, &value);

            if (!is_general_reg(value)) {
                sc_error("ERROR: line(%d) invalid (%d) register number\n", line, value);
                return FALSE;
            }
            operand_option option;
            option.operand_ = (sc_uchar)value;
            operand op = {OP_Reg, option};
            if (dst_operand) {
                *dst_operand = op;
            }
            return TRUE;
        }
        else if (token == 'S' && *src_buffer == 'R') {
                operand_option option;
                option.operand_ = REG_SR;
                operand op = {OP_Reg, option};
                if (dst_operand) {
                    *dst_operand = op;
                }
                src_buffer++;
                token = *src_buffer++;
                return TRUE;
        }
        else if (token == 'S' || token == 'G' || token == 'C') {
            // stream, generator, or consumer reg
            sc_char reg_token = token;
            sc_char digits[MAX_DIGITS_IN_NUM];
            sc_int count = 0;
            token = *src_buffer++;

            while (token != ' ' && token != 0 && token != '\n' && is_digit(token)) {
                digits[count++] = token;
                token = *src_buffer++;
            }

            if (count == 0) {
                sc_error("ERROR: line(%d) invalid register\n", line);
                return FALSE;
            }
            
            sc_uint value;
            parse_unsigned_int(digits, &value);
            if (reg_token == 'S') { 
                value = value + REG_S0;

                if (!is_stream_reg(value)) {
                    sc_error("ERROR: line(%d) invalid register number\n", line);
                    return FALSE;
                }
            }
            else if (reg_token == 'G') {
                value = value + REG_G0;

                if (!is_generator_reg(value)) {
                    sc_error("ERROR: line(%d) invalid register number\n", line);
                    return FALSE;
                }
            }
            else {
                value = value + REG_C0;

                if (!is_consumer_reg(value)) {
                    sc_error("ERROR: line(%d) invalid register number\n", line);
                    return FALSE;
                }
            }

            operand_option option;
            option.operand_ = (sc_uchar)value;
            operand op = {OP_Reg, option};
            if (dst_operand) {
                *dst_operand = op;
            }
            return TRUE;
        }
        else if (token == '#') {
            sc_ushort lo;
            if (!parse_literal(&lo)) {
                sc_error("ERROR: line(%d) invalid literal\n", line);
                return FALSE;
            }
            operand_option option;
            option.literal_ = lo;
            operand op = {OP_Lit, option};
            if (dst_operand) {
                *dst_operand = op;
            }
            return TRUE;
        }
        else if (token == '"') {
            DEBUG("begin string\n");
            sc_ushort lo;
            if (!parse_string_literal(&lo)) {
                sc_error("ERROR: line(%d) invalid literal\n", line);
                return FALSE;
            }
            operand_option option;
            option.literal_ = lo;
            operand op = {OP_Lit, option};
            if (dst_operand) {
                *dst_operand = op;
            }
            DEBUG("end string\n");
            return TRUE;
        }
        else if (token == '_') {
            label * dst_label;
            if (!parse_label(&dst_label, FALSE)) {
                sc_error("ERROR: line(%d) invalid label\n", line);
                return FALSE;
            }
            operand_option option;
            option.label_ = dst_label;
            operand op = {OP_Label, option};
            if (dst_operand) {
                *dst_operand = op;
            }
            return TRUE;
        }
    // }
    return FALSE;
}

/**
 * @brief parse an instruction
 * 
 * if success, then the pasred instruction is pushed on to the list of instructions
 * 
 * @return true if successful, otherwise false.
 */
sc_bool parse_instruction() {
    sc_char op[MAX_OPCODE_SIZE+1];
    sc_int op_length = 0;

    token = *src_buffer++;
    while (token != ' ' && token != 0 && token != '\n' && op_length <= MAX_OPCODE_SIZE) {
        op[op_length++] = token;
        token = *src_buffer++;
    }

    op[op_length++] = '\0';

    opcode dst_opcode;
    if (match_opcode(op, &dst_opcode)) {
        // now pass a instructions operands
        operand gen_operands[3];
        sc_int operand_count = 0;
        while (operand_count < dst_opcode.num_operands_ && token != '\n') {
            if (!parse_operand(&gen_operands[operand_count++])) {
                sc_print("line(%d) %s %d\n", line, op, operand_count);
                sc_error("ERROR: line(%d) invalid number of operands\n", line);
                return FALSE;
            }
        }
        
        sc_print("line(%d) %s %d\n", line, op, operand_count);

        // validate instruction's argments
        if (operand_count != dst_opcode.num_operands_) {
            sc_error("ERROR: line(%d) invalid number of operands for %s\n", line, op);
            return FALSE;
        }

        if (scmp(op, "JMPZ", 5) || scmp(op, "JMPNZ", 5)) {

            if (gen_operands[0].type_ != OP_Reg && gen_operands[0].type_ != OP_Label) {
                sc_error("ERROR: line(%d) invalid operands for %s %d\n", line, op,gen_operands[0].type_  );
                return FALSE;
            }
        }
        else if (scmp(op, "MOVL", 4)) {
            if (gen_operands[0].type_ != OP_Reg || gen_operands[1].type_ != OP_Lit) {
                sc_error("ERROR: line(%d) invalid operands for %s\n", line, op);
                return FALSE;
            }            
        }
        else if (scmp(op, "MOV", 3) || scmp(op, "SPAWN", 5)) {
            if (gen_operands[0].type_ != OP_Reg || gen_operands[1].type_ == OP_Lit) {
                sc_error("ERROR: line(%d) invalid operand(s) for %s\n", line, op);
                return FALSE;
            }
        } 
        else if (scmp(op, "JMP", 3) || scmp(op, "CALL", 3)) {
            if (gen_operands[0].type_ == OP_Lit) {
                sc_error("ERROR: line(%d) invalid operand(s) for %s\n", line, op);
                return FALSE;
            }
        }
        else if (scmp(op, "LDS", 3)) {
            if (gen_operands[0].type_ != OP_Reg || 
                gen_operands[1].type_ != OP_Reg || !is_stream_reg(gen_operands[1].op_.operand_)) {
                sc_error("ERROR: line(%d) invalid operand(s) for %s\n", line, op);
                return FALSE;
            }
        }
        else if (scmp(op, "STS", 3)) {
            if (gen_operands[1].type_ != OP_Reg || 
                gen_operands[0].type_ != OP_Reg || !is_stream_reg(gen_operands[0].op_.operand_)) {
                sc_error("ERROR: line(%d) invalid operand(s) for %s\n", line, op);
                return FALSE;
            }
        }
        else {
            // all other instructions take only registers as arguments
            for (sc_int i = 0; i < operand_count; i++) {
                if (gen_operands[0].type_ != OP_Reg) {
                    sc_error("ERROR: line(%d) invalid operand(s) for %s\n", line, op);
                    return FALSE;
                }
            }
        }

        instruction i = make_instruction(dst_opcode.opcode_, operand_count, gen_operands);
        push_instruction(i);
    }
    else {
        sc_error("ERROR: line(%d) invalid opcode %s\n", line, op);
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief parse @stream.
 *
 */
sc_bool parse_stream() {
    strip_whitespace();

    operand stream_reg;
    if (!parse_operand(&stream_reg)) {    
        sc_error("ERROR: line(%d) expected stream register\n", line);
        return FALSE;
    }

    if (stream_reg.type_ != OP_Reg || !is_stream_reg(stream_reg.op_.operand_)) {
        sc_error("ERROR: line(%d) expected stream register\n", line);
        return FALSE;
    }

    sc_ushort lit_size_offset;
    if (!parse_literal(&lit_size_offset)) {
        sc_error("ERROR: line(%d) expected size\n", line);
        return FALSE;
    }

    sc_int lit = get_literal(lit_size_offset);
    if (lit != 8 && lit != 16 && lit != 32) {
        sc_error("ERROR: line(%d) unexpected size %d\n", line, lit);
        return FALSE;        
    }
    operand size_operand;
    size_operand.type_ = OP_Lit;
    size_operand.op_.literal_ = lit_size_offset;

    strip_whitespace();
    operand rate_operand;
    if (token == 'S' && *src_buffer == 'R') {
        rate_operand.type_ = OP_Reg;
        rate_operand.op_.operand_ = REG_SR;
        src_buffer++;
        token = *src_buffer++;
    }
    else {   
        sc_ushort lit_rate_offset;
        if (!parse_literal(&lit_rate_offset)) {
            sc_error("ERROR: line(%d) expected rate\n", line);
            return FALSE;
        }
        rate_operand.type_ = OP_Lit;
        rate_operand.op_.literal_ = lit_rate_offset;
    }    

    strip_whitespace();
    sc_ushort lit_continuous_offset;
    if (!parse_literal(&lit_continuous_offset)) {
        sc_error("ERROR: line(%d) expected continuous literal\n", line);
        return FALSE;
    }

    lit = get_literal(lit_continuous_offset);
    if (lit != 0 && lit != 1) {
        sc_error("ERROR: line(%d) unexpected continuous %d\n", line, lit);
        return FALSE;        
    }
    operand continuous_operand;
    continuous_operand.type_ = OP_Lit;
    continuous_operand.op_.literal_ = lit_continuous_offset;

    operand operands[3];

    // first stream create instruction
    operands[0] = stream_reg;
    operands[1] = size_operand;
    instruction i = make_instruction(STREAM, 2, operands);
    push_instruction(i);

    // now set rate instruction
    operands[0] = stream_reg;
    operands[1] = size_operand;
    i = make_instruction(SETSF, 2, operands);
    push_instruction(i);

    operands[0] = stream_reg;
    operands[1] = continuous_operand;
    i = make_instruction(SETSC, 2, operands);
    push_instruction(i);

    return TRUE;
}

/**
 * @brief parse @attach.
 *
 */
sc_bool parse_attach() {
    strip_whitespace();

    operand operand_one;
    if (!parse_operand(&operand_one)) {    
        sc_error("ERROR: line(%d) expected stream register\n", line);
        return FALSE;
    }

    if (operand_one.type_ != OP_Reg || !(is_stream_reg(operand_one.op_.operand_) || 
        is_generator_reg(operand_one.op_.operand_) || is_consumer_reg(operand_one.op_.operand_))) {
        sc_error("ERROR: line(%d) expected stream register\n", line);
        return FALSE;
    }

    operand operand_two;
    if (!parse_operand(&operand_two)) {    
        sc_error("ERROR: line(%d) expected stream register\n", line);
        return FALSE;
    }

    if (operand_one.type_ != OP_Reg || !(is_stream_reg(operand_two.op_.operand_) || 
        is_generator_reg(operand_two.op_.operand_) || is_consumer_reg(operand_two.op_.operand_))) {
        sc_error("ERROR: line(%d) expected stream register\n", line);
        return FALSE;
    }

    sc_ushort lit_size_offset;
    if (!parse_literal(&lit_size_offset)) {
        sc_error("ERROR: line(%d) expected size\n", line);
        return FALSE;
    }

    operand operand_size;
    operand_size.type_ = OP_Lit;
    operand_size.op_.literal_ = lit_size_offset;

    // emit ATTACH instruction
    operand operands[3];
    operands[0] = operand_one;
    operands[1] = operand_two;
    operands[2] = operand_size;

    instruction i = make_instruction(ATTACH, 3, operands);
    push_instruction(i);

    return TRUE;
}

sc_bool parse_console() {
    if (token != '/') {
        sc_error("ERROR: line(%d) expected / following device\n", line);
        return FALSE;
    }

    strip_whitespace();

    sc_char func[MAX_DEVICE_SIZE+1];
    sc_int func_length = 0;
    token = *src_buffer++;
    while (token != ' ' && token != 0 && token != '\n' && func_length <= MAX_DEVICE_SIZE) {
        func[func_length++] = token;
        token = *src_buffer++;
    }
    func[func_length] = '\0';

    if (scmp(func, "write", 5)) {
        operand operand_two;
        if (!parse_operand(&operand_two)) {    
            sc_error("ERROR: line(%d) expected operand\n", line);
            return FALSE;
        }

        operand operand_one;
        operand_one.type_ = OP_Lit;
        operand_one.op_.literal_ = CONSOLE_WRITE;

        operand operands[3];
        operands[0] = operand_one;
        operands[1] = operand_two;

        instruction i = make_instruction(CONSOLE, 2, operands);
        push_instruction(i);

    } else {
        sc_error("ERROR: line(%d) unknown device function\n", line);
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief parse input stream.
 *
 */
sc_bool parse() {
    char *last_pos;

    DEBUG("begin\n");

    while ((token = *src_buffer++)) {
        //sc_print("token = %d\n", token);
        if (token == ';') {
            // skip comments
            while (*src_buffer != 0 && *src_buffer != '\n') {
                src_buffer++;
            }
        }
        else if (token == '@') {
            // toplevel construct (e.g. stream, task, etc)
            // reset label prefix...
            label_prefix[0] = '\0';

            sc_char tl[MAX_TOPLEVEL_SIZE+1];
            sc_int tl_length = 0;
            token = *src_buffer++;
            while (token != ' ' && token != 0 && token != '\n' && tl_length <= MAX_TOPLEVEL_SIZE) {
                tl[tl_length++] = token;
                token = *src_buffer++;
            }
            tl[tl_length] = '\0';
            if (scmp(tl, "task", 4)) {
                DEBUG("start task\n");
                label* dst_label;
                if (parse_label(&dst_label, TRUE)) {
                    sc_int len = slen(dst_label->name_.str_);
                    mcopy(dst_label->name_.str_, label_prefix, len);
                    label_prefix[len] = '\0';
                }
                else {
                    sc_error("ERROR: line(%d) expected label\n", line);
                    return FALSE;
                }
            }
            else if (scmp(tl, "func", 4)) {
                DEBUG("start func\n");
                label* dst_label;
                if (parse_label(&dst_label, TRUE)) {
                    sc_int len = slen(dst_label->name_.str_);
                    mcopy(dst_label->name_.str_, label_prefix, len);
                    label_prefix[len] = '\0';
                }
                else {
                    sc_error("ERROR: line(%d) expected label\n", line);
                    return FALSE;
                }
            }
            else if (scmp(tl, "stream", 6)) {
                if (!parse_stream()) {
                    return FALSE;
                }
            }
            else if (scmp(tl, "attach", 6)) {
                DEBUG("start attach\n");
                if (!parse_attach()) {
                    return FALSE;
                }
            }
            else if (scmp(tl, "entry", 4)) {
                // check that there is only one entry !!
                if (entry_point != ENTRY_NOTDEFINED) {
                    sc_error("ERROR: line(%d) multiple entry points\n", line);
                    return FALSE;
                }
                entry_point = instruction_count;
            }
            else {
                sc_error("ERROR: line(%d) unknown toplevel definition\n", line);
                return FALSE;
            }
        }
        else if (token == '.') { 
            // devices
            sc_char dev[MAX_DEVICE_SIZE+1];
            sc_int dev_length = 0;
            token = *src_buffer++;
            while (token != ' ' && token != 0 && token != '/' && token != '\n' && dev_length <= MAX_DEVICE_SIZE) {
                dev[dev_length++] = token;
                token = *src_buffer++;
            }
            dev[dev_length] = '\0';

            if (scmp(dev, "Console", 7)) {
                if (!parse_console()) {
                    return FALSE;
                }
                device_capabilities |= USE_DEVICE_CONSOLE;
            } else {
                sc_error("ERROR: line(%d) unknown device\n", line);
                return FALSE;
            }
        }
        else if (token == '#') {
            // no inline literals
            sc_error("ERROR: line(%d) unexpected #\n", line);
            return FALSE;
            
        }
        else if (token == '_') {
            // parse label definition
            parse_label(NULL, TRUE);
        }
        else if (is_alpha(token)) {
            DEBUG("start mnemoic\n");
            // parse mnemonic
            src_buffer--; // move back 1, so can be consumed as whole instruction
            if (!parse_instruction()) {
                sc_error("ERROR: line(%d) invalid instruction #\n", line);
                return FALSE;
            }
            DEBUG("end mnemoic\n");
        }
        
        if (token == '\n') {
            // process line
            line++;
        }
    }

    DEBUG("end\n");

    return TRUE;
}

//-----------------------------------------------------------------------------------------------
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

sc_bool emit(const char* filename) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        sc_error("ERROR: could not opening file %s\n", filename);
        return FALSE;
    }

    sc_uint lit_bytes = literal_count * sizeof(sc_int);
    sc_uint code_bytes = instruction_count * sizeof(sc_int);
    header my_header = {
        .magic_ = 0xDEADBEEF,
        .capabilities_ = device_capabilities,
        .entry_point_ = (sc_ushort)entry_point,
        .literals_start_ = sizeof(header),
        .literals_length_ = lit_bytes, // in bytes
        .code_start_ = sizeof(header) + lit_bytes,
        .code_length_ = code_bytes,
    };

    sc_print("sc = %d %d\n", literal_count, instruction_count);

    print_instructions();

    size_t bytes_written = fwrite(&my_header, sizeof(header), 1, file);
    if (bytes_written != 1) {
        sc_error("Error writing to file\n");
        fclose(file);
        return FALSE;
    }

    bytes_written = fwrite(literals, 1, lit_bytes, file);
    if (bytes_written != lit_bytes) {
        sc_error("Error writing data to file\n");
        fclose(file);
        return FALSE;
    }

    // encode and write each instruction
    for (int i = 0; i < instruction_count; i++) {
        sc_uint ei = encode_instruction(instructions[i]);
        bytes_written = fwrite(&ei, 1, 4, file);
        if (bytes_written != sizeof(sc_uint)) {
            sc_error("Error writing instruction data to file %zu\n", bytes_written);
            fclose(file);
            return FALSE;
        }
    }

    fclose(file);

    return TRUE;
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------

sc_bool dis(char* filename) {
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
    size_t bytes_read = fread(literals, 1, header_data.literals_length_, file);
    if (bytes_read != header_data.literals_length_) {
        sc_error("Error reading literals from file\n");
        return FALSE;
    }
    literal_count = header_data.literals_length_ / 4;

    sc_uint* encoded_instructions = (sc_uint*)malloc(header_data.code_length_);
    fseek(file, header_data.code_start_, SEEK_SET);
    bytes_read = fread(encoded_instructions, 1, header_data.code_length_, file);
    if (bytes_read != header_data.code_length_) {
        sc_error("Error reading code from file\n");
        return FALSE;
    }

    instruction_count = header_data.code_length_ / 4;
    entry_point = header_data.entry_point_;

    //sc_print("sc = %d %d\n", literal_count, instruction_count);

    // display capabilities
    if (header_data.capabilities_ & USE_DEVICE_CONSOLE) {
        sc_print("device .Console\n");
    }
    if (header_data.capabilities_ & USE_DEVICE_SCREEN) {
        sc_print("requires device .Screen\n");
    }

    for(sc_int i = 0; i < instruction_count; i++) {
        // sc_print("i = %u\n", encoded_instructions[i]);
        if (i == entry_point) {
             sc_print("@entry\n");
        }

        print_encode_instruction(encoded_instructions[i], "\t");
    }

    return TRUE;
}