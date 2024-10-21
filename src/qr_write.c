#include "qr.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    MODE_ECI         = 0b0111,
    MODE_NUMERIC     = 0b0001,
    MODE_ALPHANUM    = 0b0010,
    MODE_BYTE        = 0b0100,
    MODE_KANJI       = 0b1000,
    MODE_STRUCT_APP  = 0b0011,
    MODE_FNC1_FIRST  = 0b0101,
    MODE_FNC1_SECND  = 0b1001,
} ModeIndicator;

// Page 21 of standard
uint8_t encode_alphanumeric(char character) {
    switch (character) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'A': return 10;
        case 'B': return 11;
        case 'C': return 12;
        case 'D': return 13;
        case 'E': return 14;
        case 'F': return 15;
        case 'G': return 16;
        case 'H': return 17;
        case 'I': return 18;
        case 'J': return 19;
        case 'K': return 20;
        case 'L': return 21;
        case 'M': return 22;
        case 'N': return 23;
        case 'O': return 24;
        case 'P': return 25;
        case 'Q': return 26;
        case 'R': return 27;
        case 'S': return 28;
        case 'T': return 29;
        case 'U': return 30;
        case 'V': return 31;
        case 'W': return 32;
        case 'X': return 33;
        case 'Y': return 34;
        case 'Z': return 35;
        case ' ': return 36;
        case '$': return 37;
        case '%': return 38;
        case '*': return 39;
        case '+': return 40;
        case '-': return 41;
        case '.': return 42;
        case '/': return 43;
        case ':': return 44;
        default:  return 45;
    }
}

// Write 8 bits to a data array at a given bit position
void write_8(uint8_t* data, size_t data_size, uint8_t bits, size_t index) {
    size_t array_index = index >> 3;
    size_t byte_index = index & 0b111;

    if (array_index >= data_size) {
        printf("ERROR: write_8: Data array not big enough! Size: %zu Index: %zu\n", data_size, array_index);
        return;
    }
    if (array_index < data_size)
        data[array_index] |= bits >> byte_index;
    if (array_index + 1 < data_size)
        data[array_index + 1] = bits << (8 - byte_index);
}

// Write up to 16 bits to a data array at a given bit position
void write_bits(uint8_t* data, size_t data_size, uint16_t bits, size_t index, size_t count) {
    size_t array_index = index >> 3;
    size_t byte_index = index & 0b111;

    bits = bits << (16 - count);
    uint32_t shifted_bits = (uint32_t)bits << (16 - byte_index);

    if (array_index >= data_size) {
        printf("ERROR: write_bits: Data array not big enough! Size: %zu Index: %zu\n", data_size, array_index);
        return;
    }
    if (array_index < data_size)
        data[array_index]    |= shifted_bits >> 24;
    if (array_index + 1 < data_size)
        data[array_index + 1] = shifted_bits >> 16;
    if (array_index + 2 < data_size)
        data[array_index + 2] = shifted_bits >> 8;
}

// Returns the number of data codewords a qr code can fit
// Page 28 of standard
size_t codeword_capacity(Version version, ErrorLevel err_lvl) {
    static size_t codeword_capacities[4 * 400] = {
        19, 16, 13, 9, 34, 28, 22, 16, 55, 44, 34, 26, 80, 64, 48, 36,
        108, 86, 62, 46, 136, 108, 76, 60, 156, 124, 88, 66, 194, 154, 110, 86,
        232, 182, 132, 100, 274, 216, 154, 122, 324, 254, 180, 140, 370, 290, 206, 158,
        428, 334, 244, 180, 461, 365, 261, 197, 523, 415, 295, 223, 589, 453, 325, 253,
        647, 507, 367, 283, 721, 563, 397, 313, 795, 627, 445, 341, 861, 669, 485, 385,
        932, 714, 512, 406, 1006, 782, 568, 442, 1094, 860, 614, 464, 1174, 914, 664, 514,
        1276, 1000, 718, 538, 1370, 1062, 754, 596, 1468, 1128, 808, 628, 1531, 1193, 871, 661,
        1631, 1267, 911, 701, 1735, 1373, 985, 745, 1843, 1455, 1033, 793, 1955, 1541, 1115, 845,
        2071, 1631, 1171, 901, 2191, 1725, 1231, 961, 2306, 1812, 1286, 986, 2434, 1914, 1354, 1054,
        2566, 1992, 1426, 1096, 2702, 2102, 1502, 1142, 2812, 2216, 1582, 1222, 2956, 2334, 1666, 1276,
    };

    return codeword_capacities[(version - 1) * 4 + err_lvl];
}

// Encodes array of data into data codewords
// Page 17 of standard
void encode_data(const uint8_t* data, size_t size, ModeIndicator mode, Symbol* sym) {
    size_t codeword_cnt = codeword_capacity(sym->version, sym->err_lvl);
    uint8_t* codewords  = malloc(codeword_cnt);
    memset(codewords, 0, codeword_cnt);

    codewords[0] = mode << 4;
    size_t index = 4; // The bit index into codewords

    switch (mode) {
        case MODE_NUMERIC: {
            // The number of bits in the character count
            uint8_t char_cnt_len;
            if (sym->version < 10)      char_cnt_len = 10;
            else if (sym->version < 27) char_cnt_len = 12;
            else                        char_cnt_len = 14;

            write_bits(codewords, codeword_cnt, (uint16_t)size, index, char_cnt_len);
            index += char_cnt_len;

            // The number is split into blocks of 3 digits
            // which are then converted into binary 
            int i = 0;
            for (; i < size - (size % 3); i += 3) {
                // The binary representation of 3 base-10 digits
                uint16_t chunk = (data[i] - '0') * 100;
                chunk += (data[i + 1] - '0') * 10;
                chunk += (data[i + 2] - '0');
                write_bits(codewords, codeword_cnt, chunk, index, 10);
                index += 10;
            }

            // Account for remaining digits
            if (size - i == 1) {        // 1 remaining digit
                write_bits(codewords, codeword_cnt, data[i] - '0', index, 4);
                index += 4;
            } else if (size - i == 2) { // 2 remaining digits
                uint16_t chunk = (data[i] - '0') * 10 + (data[i + 1] - '0');
                write_bits(codewords, codeword_cnt, chunk, index, 7);
                index += 7;
            }
        } break; // case NUMERIC

        case MODE_ALPHANUM: {
            // The number of bits in the character count
            uint8_t char_cnt_len;
            if (sym->version < 10)      char_cnt_len = 9;
            else if (sym->version < 27) char_cnt_len = 11;
            else                        char_cnt_len = 13;

            write_bits(codewords, codeword_cnt, (uint16_t)size, index, char_cnt_len);
            index += char_cnt_len;

            // The text is split into groups of 2 characters
            int i = 0;
            for (; i < size - (size % 2); i += 2) {
                // The binary encodation of 2 alpha numeric characters
                uint16_t chunk = encode_alphanumeric(data[i]) * 45;
                chunk += encode_alphanumeric(data[i + 1]);
                write_bits(codewords, codeword_cnt, chunk, index, 11);
                index += 11;
            }

            // Account for any remaining character
            if (size - i) {
                write_bits(codewords, codeword_cnt, encode_alphanumeric(data[i]), index, 6);
                index += 6;
            }
        } break; // case ALPHANUMERIC
        
        case MODE_BYTE: {
            // The number of bits in the character count
            uint8_t char_cnt_len;
            if (sym->version < 10) char_cnt_len = 8;
            else                   char_cnt_len = 16;

            write_bits(codewords, codeword_cnt, (uint16_t)size, index, char_cnt_len);
            index += char_cnt_len;

            for (int i = 0; i < size; i += 1) {
                write_8(codewords, codeword_cnt, data[i], index);
                index += 8;
            }
        } break; // case Byte

        default: {
            printf("ERROR: MODE NOT SUPPROTED!\n");
        }
    }

    // Add the terminator "0000"
    index = min(index + 4, codeword_cnt * 8);

    // Round up to the nearest multiple of 8
    size_t nearest = index + 7 - (index - 1) % 8;
    index = min(nearest, codeword_cnt * 8);

    // Add pad codewords
    while (index < codeword_cnt * 8) {
        write_8(codewords, codeword_cnt, 0b11101100, index);
        index += 8;

        if (index >= codeword_cnt * 8)
            break;

        write_8(codewords, codeword_cnt, 0b00010001, index);
        index += 8;
    }

    if (sym->data_size != 0)
        free(sym->data);

    sym->data = codewords;
    sym->data_size = codeword_cnt;
}
