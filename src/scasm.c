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

#include <string.h> 

sc_bool read(const char * filename);
sc_bool parse();
sc_bool emit(const char* filename);

int main(int argc, char **argv) {
    sc_char * include_paths[64];
    sc_uint num_include_paths = 0;
    sc_char * input_file = NULL;
    sc_char * output_file = NULL;

    for (sc_int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            sc_print("scasm - SC Assembler, 21st Sept 2024.\n");
            return 0;
        } else if (strncmp(argv[i], "-I", 2) == 0) {
            // Check if the filename is directly attached
            if (strlen(argv[i]) > 2) {
                include_paths[num_include_paths] = argv[i] + 2; // Skip "-I" part
                num_include_paths++;
            } else {
                sc_error("ERROR: -I option requires a filename\n");
                return 1;
            }
               
            
        } else {
            if (input_file == NULL) {
                input_file = argv[i];
            }
            else {
                output_file = argv[i];
            }
        }
    }
    
    if (input_file == NULL || output_file == NULL) {
        sc_error("ERROR: scasm - SC Assembler, 21st Sept 2024.\n");
        return 1;
    }

    // if(argc == 2 && scmp(argv[1], "-v", 2)) {
    //     sc_print("scasm - SC Assembler, 21st Sept 2024.\n");
    //     return 1;
    // }
	// if(argc != 3) {
    //     sc_print("usage: scasm [-v, -I<path>] input.sc output.scrom");
    //     return 1;
    // }

    // read file into buffer for processing
    if (!read(input_file)) {
        return 1;
    }

    // parse buffer
    if (!parse()) {
        return 1;
    }

    // dump what we have created
    //print_instructions();
    // print_constants();

    if (!emit(output_file)) {
        return 1;
    }

    return 0;
}