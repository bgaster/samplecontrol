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
    
sc_bool dis(char *filename);

sc_int main(int argc, char** argv) {
    if(argc == 2 && scmp(argv[1], "-v", 2)) {
        sc_print("scdis - SC Diassembler, 26st Sept 2024.\n");
        return 1;
    }
	if(argc != 2) {
        sc_print("usage: scdis [-v] input.scrom");
        return 1;
    }

    dis(argv[1]);

    return 0;
}