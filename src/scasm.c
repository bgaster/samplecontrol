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

sc_bool read(const char * filename);
sc_bool parse();
sc_bool emit(const char* filename);

int main(int argc, char **argv) {
    if(argc == 2 && scmp(argv[1], "-v", 2)) {
        sc_print("scasm - SC Assembler, 21st Sept 2024.\n");
        return 1;
    }
	if(argc != 3) {
        sc_print("usage: scasm [-v] input.sc output.scrom");
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
    //print_instructions();
    // print_constants();

    if (!emit(argv[2])) {
        return 1;
    }

    return 0;
}