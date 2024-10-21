#ifndef __QR_H__
#define __QR_H__

#include <stdint.h>

typedef enum {
    ERROR_LEVEL_LOW,
    ERROR_LEVEL_MEDIUM,
    ERROR_LEVEL_QUARTILE,
    ERROR_LEVEL_HIGH,
} ErrorLevel;

typedef uint32_t Version;

typedef struct {
    Version version;
    ErrorLevel err_lvl;
    uint8_t* data;
    size_t data_size;
} Symbol;

Symbol create_symbol(Version version, ErrorLevel err_lvl);
void delete_symbol(Symbol* s);

#endif
