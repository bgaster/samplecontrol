#include <util.h>

sc_int slen(const sc_char *a) {
    sc_int i = 0;
    while (a[i] != '\0') { 
        i++;
    }
    return i;
}

sc_int scmp(const sc_char *a, const sc_char *b, sc_int len) { 
    int i = 0; 
    while(a[i] == b[i]) {
        if(!a[i] || ++i >= len) {
            return TRUE;
        } 
    }
    return FALSE;
}

sc_char *copy(const sc_char *src, sc_char *dst, sc_char c) { 
    while(*src && *src != c) { 
        *dst++ = *src++; 
    }
    *dst++ = 0;
    return dst; 
}

sc_char *mcopy(const sc_char *src, sc_char *dst, sc_size_t len) { 
    for (sc_size_t i = 0; i < len; i++) {
        *dst++ = *src++;
    } 
    return dst;
}

sc_bool is_digit(sc_int c) {
    return (c >= '0' && c <= '9') ? TRUE : FALSE;
}

sc_bool is_alpha(sc_int c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) ? TRUE : FALSE;
}

sc_bool is_upper(sc_int c) {
    return (c >= 'A' && c <= 'Z') ? TRUE : FALSE;
}

sc_void *sc_malloc( sc_size_t size ) {
    return malloc(size);
}

sc_void sc_free(sc_void *ptr) {
    free(ptr);
}