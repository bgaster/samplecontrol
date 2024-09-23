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
#include <lexer.h>

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

#define MAX_OPCODE_SIZE 5

#define MAX_TOPLEVEL_SIZE 10

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

void print_operand(operand op) {
    if (op.type_ == OP_Reg) {
        sc_print("R%d", op.op_.operand_);
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
// Globals
//-----------------------------------------------------------------------------------------------

static sc_int token;
static sc_char* src_buffer;
static sc_uint line = 1;
static sc_char label_prefix[MAX_LABEL_LENGTH] = { 0 };

#define ENTRY_NOTDEFINED -1
static sc_int entry_point = ENTRY_NOTDEFINED;

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

#define push_instruction(i) (instructions[instruction_count++] = i)

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
    MOV, MOVL, 
    JMP, JMPNE, NOP, 
    ADD, SUB, MUL, FTOI,
    ADDF, SUBF, MULF, ITOF,
    LDL, PUSH, POP,
    SPAWN, START,
};

const opcode opcodes[] = { 
    { "MOV", MOV, 2}, { "MOVL", MOVL, 2}, 
    { "JMP", JMP, 1}, {"JMPNE", JMPNE, 1}, {"NOP", NOP, 0}, 
    {"ADD", ADD, 3}, {"SUB", SUB, 3}, {"MUL", ADD, 3}, {"FTOI", FTOI, 2},
    {"ADDF", ADD, 3}, {"SUBF", SUBF, 3}, {"MULF", ADD, 3}, {"ITOF", ITOF, 2},
    {"LDL", LDL, 1}, {"PUSH", PUSH, 1}, {"POP", POP, 1},
    {"SPAWN", SPAWN, 1}, {"START", START, 0},
 };

sc_bool match_opcode(sc_char *op, opcode * dst_opcode) {
    sc_int len = slen(op);
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
    sc_print("   %s ", opcodes[inst.opcode_].str_);
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
        sc_print("%d", i);
        print_instruction(instructions[i]);
        sc_print("\n");
    }
}

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
    mcopy(label_prefix, lab, prefix_length);
    mcopy(label_start, lab+prefix_length, label_length);
    lab[label_length] = '\0';
    label* dst;

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
            
            sc_uint value;
            parse_unsigned_int(digits, &value);

            if (value > MAX_REGISTER_NUM) {
                sc_error("ERROR: line(%d) invalid register number\n", line);
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

        if (scmp(op, "MOVL", 4)) {
            if (gen_operands[0].type_ != OP_Reg || gen_operands[1].type_ != OP_Lit) {
                sc_error("ERROR: line(%d) invalid operands for %s\n", line, op);
                return FALSE;
            }            
        }
        else if (scmp(op, "MOV", 3)) {
            if (gen_operands[0].type_ != OP_Reg || gen_operands[1].type_ == OP_Lit) {
                sc_error("ERROR: line(%d) invalid operand(s) for %s\n", line, op);
                return FALSE;
            }
        } 
        else if (scmp(op, "JMP", 3) || scmp(op, "JMPNE", 3) || scmp(op, "SPAWN", 5)) {
            if (gen_operands[0].type_ == OP_Lit) {
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
            sc_char tl[MAX_TOPLEVEL_SIZE+1];
            sc_int tl_length = 0;
            token = *src_buffer++;
            while (token != ' ' && token != 0 && token != '\n' && tl_length <= MAX_TOPLEVEL_SIZE) {
                tl[tl_length++] = token;
                token = *src_buffer++;
            }
            tl[tl_length] = '\0';
            if (scmp(tl, "task", 4)) {
                label* dst_label;
                if (parse_label(&dst_label, TRUE)) {
                    sc_int len = slen(dst_label->name_.str_);
                    mcopy(dst_label->name_.str_, label_prefix, len);
                }
                else {
                    sc_error("ERROR: line(%d) expected label\n", line);
                    return FALSE;
                }
            }
            else if (scmp(tl, "entry", 4)) {
                // check that there is only one entry !!
                if (entry_point != ENTRY_NOTDEFINED) {
                    sc_error("ERROR: line(%d) multiple entry points\n", line);
                    return FALSE;
                }
                label_prefix[0] = '\0';
                entry_point = instruction_count;
            }
            else {
                sc_error("ERROR: line(%d) unknown toplevel definition\n", line);
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
            // parse mnemonic
            src_buffer--; // move back 1, so can be consumed as whole instruction
            if (!parse_instruction()) {
                sc_error("ERROR: line(%d) invalid instruction #\n", line);
                return FALSE;
            }
        }
        // else {
        //     // no inline registers
        //     sc_error("ERROR: line(%d) unexpected token %c\n", line, token);
        //     return FALSE;
        // }
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

int main(int argc, char **argv) {
    if(argc == 2 && scmp(argv[1], "-v", 2)) {
        out_msg("scasm - SC Assembler, 21st Sept 2024.\n");
        return 1;
    }
	if(argc != 3) {
        error_top("usage", "scasm [-v] input.sc output.rom");
        return 1;
    }

    // read file into buffer for processing
    if (!read(argv[1])) {
        return 1;
    }

    // parse buffer
    if (!parse()) {
        return 1;
    }

    // dump what we have created
    print_instructions();
    // print_constants();


    return 0;
}