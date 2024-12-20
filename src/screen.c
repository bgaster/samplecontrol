/* This file is part of {{ samplecontrol }}.
 * 
 * 2024 Benedict R. Gaster (cuberoo_)
 * 
 * Licensed under either of
 * Apache License, Version 2.0 (LICENSE-APACHE or http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license (LICENSE-MIT or http://opensource.org/licenses/MIT)
 * at your option.
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>
#include "SDL_FontCache.h"

#include <screen.h>

//#if defined(__DESKTOP__)

//-------------------------------------------------------------------------------------

typedef struct {
    unsigned char red_;
    unsigned char green_;
    unsigned char blue_;
} colour;

static colour colour_pallet[16] = { 
    { 0, 0, 0},        // black
    { 255, 255, 255 }, // white
    { 255, 0, 0, },     // red
    { 0, 255, 0 },      // green
    { 0, 0, 255},       // blue 

    0 
};

//-------------------------------------------------------------------------------------

#define MAX_FONTS 9

static FC_Font* fonts[MAX_FONTS] = { 0 };

static sc_int number_fonts = 0;

//-------------------------------------------------------------------------------------

static sc_int window_width = 400;
static sc_int window_height = 400;
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static sc_short screen_x = 0;
static sc_short screen_y = 0;
static sc_uchar colour_index = 0;
static sc_bool quit = FALSE;
static sc_queue *mouse_queue = NULL;
static sc_int mouse_x = 0;
static sc_int mouse_y = 0;

sc_bool has_screen_device() {
    return TRUE;
}

void screen_set_rate(sc_int fps) {
}

sc_bool screen_should_close() {
    return quit;
}

sc_bool init_screen() {
    // returns zero on success else non-zero
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        sc_error("ERROR: initializing SDL: %s\n", SDL_GetError());
    }

    window = SDL_CreateWindow("sample control",
                        SDL_WINDOWPOS_CENTERED,
                        SDL_WINDOWPOS_CENTERED,
                        window_width, window_height, SDL_WINDOW_HIDDEN);

    if (window == NULL) {
        sc_error("ERROR: SDL window failed to initialise: %s\n", SDL_GetError());
        return 1;
    }

    renderer =  SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    if (renderer == NULL) {
        sc_error("ERROR: SDL window failed to initialise: %s\n", SDL_GetError());
        return 1;
    }
    //SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE);
    SDL_SetRelativeMouseMode(SDL_FALSE);
    // SDL_SetRelativeMouseMode(SDL_TRUE);
    mouse_x = 0;
    mouse_y = 0;

    return TRUE;
}

void screen_begin_frame() {
}

void screen_end_frame() {
    SDL_RenderPresent(renderer);
}

void screen_line(unsigned short x, unsigned short y) {
    SDL_RenderDrawLine(renderer, screen_x, screen_y, x, y);
}

void screen_pixel() {
    SDL_RenderDrawPoint(renderer, screen_x, screen_y);
}

void screen_fill() {
    // Clear winow
    SDL_RenderClear( renderer );
}

void screen_colour(sc_uchar index) {
    colour_index = index;
    colour c = colour_pallet[index];
    SDL_SetRenderDrawColor( renderer, c.red_, c.green_, c.blue_, 255 ); 
}

void screen_move(sc_ushort x, sc_ushort y) {
    screen_x = x;
    screen_y = y;
}

void screen_rect(sc_ushort w, sc_ushort h) {
    // Render rect
    SDL_Rect r;
    r.x = screen_x;
    r.y = screen_y;
    r.w = w;
    r.h = h;
    SDL_RenderFillRect( renderer, &r );
}

void screen_blit(sc_ushort x1, sc_ushort y1, sc_ushort x2, sc_ushort y2, sc_uchar *pixels) {

}

void screen_palette(void) {

}

void screen_resize(sc_ushort width, sc_ushort height, int scale) {
    window_width = width;
    window_height = height;
    mouse_x = 0; // window_width / 2;
    mouse_y = 0; //window_height / 2;
    SDL_RenderSetScale(renderer, scale,scale);
    SDL_SetWindowSize(window, width, height);
    SDL_ShowWindow(window);
}

void screen_redraw(void) {

}

void screen_font(sc_ushort index, const sc_char * filename, sc_ushort point) {
    if (index >= 0 && index < MAX_FONTS) {
        FC_Font* font = FC_CreateFont();
        colour c = colour_pallet[colour_index];
        FC_LoadFont(
            font, renderer, filename, point, FC_MakeColor(c.red_,c.green_,c.blue_,255), TTF_STYLE_NORMAL);
        fonts[index] = font;
    }
}

void screen_text(sc_ushort index, const sc_char * str) {
    FC_Draw(fonts[index], renderer, screen_x, screen_y, str);
}

sc_bool screen_process_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e)){
        switch (e.type) {
            case SDL_QUIT: {
                quit = TRUE;
                break;
            }
            case SDL_KEYDOWN: {
                quit = TRUE;
                break;
            }
            // mouse events are handled via a single 32-bit uint
            // top bit is 1 if it is a button message
            //     bottom 8 bits then represent the button pressed
            // top bit is 0 if it is a mouse motion message
            //     top 16-bits represent x
            //     bottom 16-bits represent y
            case SDL_MOUSEBUTTONDOWN: {
                if (mouse_queue) {
                    SDL_MouseButtonEvent b = e.button;
                    sc_uint v = ((sc_ushort)b.button) | 0x80000000; // set top bit
                    enqueue(mouse_queue, v);
                }
                break;
            }
            case SDL_MOUSEMOTION: {
                if (mouse_queue) {
                    sc_ushort mouse_x = e.motion.x;
                    sc_ushort mouse_y = e.motion.y;
                    sc_uint v = (mouse_x << 16 | mouse_y) & 0x7FFFFFFF; 
                    // top bit is cleared for mouse move, this should not actually be necessary...
                    enqueue(mouse_queue, v);
                }
                break;
                // top 2 bits 
                // 00 => mouse motion
                // 01 => reserved
                // 10 => button press
                // 11 => keyboard 
            }
            default:
                break;    
        }
    }
    return quit;
}

sc_bool attach_mouse_generator(sc_queue * queue) {
    mouse_queue = queue;
    return TRUE;
}

sc_bool delete_screen() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return TRUE;
}

// #else // __DESKTOP__

// sc_bool has_screen_device() {
//     return FALSE;
// }

// #endif // !__DESKTOP__