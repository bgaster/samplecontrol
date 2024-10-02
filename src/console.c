#include <console.h>

sc_bool has_console_device() {
    return TRUE;
}

sc_bool init_console() {
    return TRUE;
}

sc_bool write_console(sc_char ch) {
    putc(ch, stdout);
    return TRUE;
}

sc_bool delete_console() {
    return FALSE;
}
