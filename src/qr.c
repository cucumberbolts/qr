#include "qr.h"

#include <stdlib.h>

Symbol create_symbol(Version version, ErrorLevel err_lvl) {
    Symbol s;
    s.version = version;
    s.err_lvl = err_lvl;
    s.data = NULL;
    s.data_size = 0;

    return s;
}

void delete_symbol(Symbol* s) {
    free(s->data);
    s->data_size = 0;
}
