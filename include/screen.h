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
#include <lfqueue.h>

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
void screen_set_rate(sc_int fps);
sc_bool screen_should_close();
void screen_begin_frame();
void screen_end_frame();
sc_bool screen_process_events();
sc_bool attach_mouse_generator(sc_queue * queue);

void screen_pixel();
void screen_fill();
void screen_rect(sc_ushort w, sc_ushort h);
void screen_move(sc_ushort x, sc_ushort y);
void screen_colour(sc_uchar index);
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