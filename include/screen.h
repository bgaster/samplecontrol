/* This file is part of {{ samplecontrol }}.
 * 
 * 2024 Benedict R. Gaster (cuberoo_)
 * 
 * Licensed under either of
 * Apache License, Version 2.0 (LICENSE-APACHE or http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license (LICENSE-MIT or http://opensource.org/licenses/MIT)
 * at your option.
 */
#ifndef SCREEN_HEADER_H
#define SCREEN_HEADER_H

#include <util.h>

/**
 * @brief check to see if current build has screen device
 * @return true if current build supports screen device
 */
sc_bool has_screen_device();

/**
 * @brief initialize screen device
 * 
 * @return true if screen is initialized correctly, otherwise false
 */
sc_bool init_screen();

void screen_pixel(sc_ushort x, sc_ushort y, sc_uchar color);
void screen_fill(sc_uchar color);
void screen_rect(sc_ushort x1, sc_ushort y1, sc_ushort x2, sc_ushort y2, sc_uchar color);
void screen_blit(sc_ushort x1, sc_ushort y1, sc_ushort x2, sc_ushort y2, sc_uchar *pixels);
void screen_palette(void);
void screen_resize(sc_ushort width, sc_ushort height, int scale);
void screen_redraw(void);

/**
 * @brief deinitialize screen device
 * 
 * @return true if screen was valid and deleted, otherwise false
 */
sc_bool delete_screen();


#endif // screen_HEADER_H