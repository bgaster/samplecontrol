#include <screen.h>

#if defined(__DESKTOP__)

sc_bool has_screen_device() {
    return TRUE;
}

sc_bool init_screen() {
    return FALSE;
}

void screen_pixel(sc_ushort x, sc_ushort y, sc_uchar color) {

}

void screen_fill(sc_uchar color) {

}

void screen_rect(sc_ushort x1, sc_ushort y1, sc_ushort x2, sc_ushort y2, sc_uchar color) {

}

void screen_blit(sc_ushort x1, sc_ushort y1, sc_ushort x2, sc_ushort y2, sc_uchar *pixels) {

}

void screen_palette(void) {

}

void screen_resize(sc_ushort width, sc_ushort height, int scale) {

}

void screen_redraw(void) {

}

sc_bool delete_screen() {
    return FALSE;
}

#else // __DESKTOP__

sc_bool has_screen_device() {
    return FALSE;
}

#endif // !__DESKTOP__