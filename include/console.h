/* This file is part of {{ samplecontrol }}.
 * 
 * 2024 Benedict R. Gaster (cuberoo_)
 * 
 * Licensed under either of
 * Apache License, Version 2.0 (LICENSE-APACHE or http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license (LICENSE-MIT or http://opensource.org/licenses/MIT)
 * at your option.
 */
#ifndef DEVICE_HEADER_H
#define DEVICE_HEADER_H

#include <util.h>

/**
 * @brief check to see if current build has console device
 * @return true if current build supports console device
 */
sc_bool has_console_device();

/**
 * @brief initialize console device
 * 
 * @param a non null pointer to store internal device information
 * @return true if console is initialized correctly, otherwise false
 */
sc_bool init_console();

/**
 * @brief write char console device
 * 
 *  this function returns immediately and does not block, the character 
 *  maynot have been written to the console when it returns
 * 
 * @param a non null pointer to store internal device information
 * @param char to write to console
 * @return true if console is initialized correctly and value is "written" to console, 
 *         otherwise false
 */
sc_bool write_console(sc_char ch);

/**
 * @brief deinitialize console device
 * 
 * @param a non null pointer, that was used in a corresponding call to 
 *        init_console
 * @return true if console was valid and deleted, otherwise false
 */
sc_bool delete_console();


#endif // DEVICE_HEADER_H