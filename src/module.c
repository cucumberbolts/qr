#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../lib/stb_image_write.h"

#include "qr.h"
#include "error.h"

// Notes:
// side_length = (version - 1) * 4 + 21
// Dark pixel is always at ((4 * version) + 9, 8)
// 0 -> white pixel
// 1 -> black pixel

// Table taken from https://www.thonky.com/qr-code-tutorial/alignment-pattern-locations
// First column is the qr code version
// Stide: 7 bytes
uint8_t align_pat_coords[] = {
    6, 18, 0,  0,  0, 0, 0, // Version 2
    6, 22, 0,  0,  0, 0, 0, // Version 3
    6, 26, 0,  0,  0, 0, 0, // etc.
    6, 30, 0,  0,  0, 0, 0,
    6, 34, 0,  0,  0, 0, 0,
    6, 22, 38, 0,  0, 0, 0,
    6, 24, 42, 0,  0, 0, 0,
    6, 26, 46, 0,  0, 0, 0,
    6, 28, 50, 0,  0, 0, 0,
    6, 30, 54, 0,  0, 0, 0,
    6, 32, 58, 0,  0, 0, 0,
    6, 34, 62, 0,  0, 0, 0,
    6, 26, 46, 66, 0, 0, 0,
    6, 26, 48, 70, 0, 0, 0,
    6, 26, 50, 74, 0, 0, 0,
    6, 30, 54, 78, 0, 0, 0,
    6, 30, 56, 82, 0, 0, 0,
    6, 30, 58, 86, 0, 0, 0,
    6, 34, 62, 90, 0, 0, 0,
    6, 28, 50, 72, 94, 0, 0,
    6, 26, 50, 74, 98, 0, 0,
    6, 30, 54, 78, 102, 0, 0,
    6, 28, 54, 80, 106, 0, 0,
    6, 32, 58, 84, 110, 0, 0,
    6, 30, 58, 86, 114, 0, 0,
    6, 34, 62, 90, 118, 0, 0,
    6, 26, 50, 74, 98,  122, 0,
    6, 30, 54, 78, 102, 126, 0,
    6, 26, 52, 78, 104, 130, 0,
    6, 30, 56, 82, 108, 134, 0,
    6, 34, 60, 86, 112, 138, 0,
    6, 30, 58, 86, 114, 142, 0,
    6, 34, 62, 90, 118, 146, 0,
    6, 30, 54, 78, 102, 126, 150,
    6, 24, 50, 76, 102, 128, 154,
    6, 28, 54, 80, 106, 132, 158,
    6, 32, 58, 84, 110, 136, 162,
    6, 26, 54, 82, 110, 138, 166,
    6, 30, 58, 86, 114, 142, 170,
};

// Returns the number of row-column coordinates from
// the alignment pattern table for a given qr version
uint32_t align_pat_side_coords(Version ver) {
    if (ver == 1) return 0;
    if (ver < 7)  return 2;
    if (ver < 14) return 3;
    if (ver < 21) return 4;
    if (ver < 28) return 5;
    if (ver < 35) return 6;
    return 7;
}

// Checks if an alignment pattern should be placed on the
// qr code given the coordinates of its centre module
// (Alignment patterns must not overlap with finder patterns or separators)
bool is_align_pat(Version ver, uint8_t x, uint8_t y) {
    // Side length of the code
    uint32_t side = (ver - 1) * 4 + 21;

    // Too far away from any, early out
    if (x > 10 && y > 10)
        return true;

    // Test the left-two squares
    if (x - 2 < 9) {
        if (y - 2 < 9)
            return false;
        if (y + 2 > side - 9)
            return false;
    }
    // Test the top-right square
    else if (x + 2 > side - 9 && y - 2 < 9) {
        return false;
    }

    return true;
}

void write_square(uint8_t* canvas, size_t canvas_size, const uint8_t* square, size_t square_size, uint32_t x, uint32_t y) {
    if (y * canvas_size + x + square_size * square_size > canvas_size * canvas_size) {
        printf("write_square(): out of bounds!\n");
        return;
    }

    for (uint32_t row = 0; row < square_size; ++row) {
    	for (uint32_t col = 0; col < square_size; ++col) {
            size_t canvas_coord = (y + row) * canvas_size + (x + col);
            size_t square_coord = row * square_size + col;
            canvas[canvas_coord] = square[square_coord];
        }
    }
}

// Masking patterns for QR codes
// Formulae taken from https://www.thonky.com/qr-code-tutorial/mask-patterns
// Return true if the bit at (x, y) should be flipped
typedef bool (*MaskFn)(size_t x, size_t y);

bool mask1(size_t x, size_t y) { return (x + y) % 2 == 0; }
bool mask2(size_t x, size_t y) { return y % 2 == 0; }
bool mask3(size_t x, size_t y) { return x % 3 == 0; }
bool mask4(size_t x, size_t y) { return (x + y) % 3 == 0; }
bool mask5(size_t x, size_t y) { return ((y / 2) + (x + 3)) % 2 == 0; }
bool mask6(size_t x, size_t y) { return (x * y) % 2 + (x * y) % 3 == 0; }
bool mask7(size_t x, size_t y) { return ((x * y) % 2 + (x * y) % 3) % 2 == 0; }
bool mask8(size_t x, size_t y) { return ((x + y) % 2 + (x * y) % 3) % 2 == 0; }

const static MaskFn mask_pats[8] = {
     mask1, mask2, mask3, mask4, mask5, mask6, mask7, mask8
};

// bool (*mask_pats[8])(size_t x, size_t y) = {
//     mask1, mask2, mask3, mask4, mask5, mask6, mask7, mask8
// };

// The following 2 data tables are from
// https://www.thonky.com/qr-code-tutorial/format-version-tables

void write_function_patterns(Version ver, uint8_t* qr, size_t side) {
    // Finder pattern
    const static uint8_t finder_pat[] = {
        1, 1, 1, 1, 1, 1, 1,
        1, 0, 0, 0, 0, 0, 1,
        1, 0, 1, 1, 1, 0, 1,
        1, 0, 1, 1, 1, 0, 1,
        1, 0, 1, 1, 1, 0, 1,
        1, 0, 0, 0, 0, 0, 1,
        1, 1, 1, 1, 1, 1, 1,
    };

    // Write finder patterns
    const static uint8_t align_pat[] = {
        1, 1, 1, 1, 1,
        1, 0, 0, 0, 1,
        1, 0, 1, 0, 1,
        1, 0, 0, 0, 1,
        1, 1, 1, 1, 1,
    };

    // Write finder patterns
    write_square(qr, side, finder_pat, 7, 0, 0);
    write_square(qr, side, finder_pat, 7, side - 7, 0);
    write_square(qr, side, finder_pat, 7, 0, side - 7);

    // White border around the finder patterns
    // Horizontally
    memset(qr + side * 7, 0, 8);          // top left
    memset(qr + side * 8 - 8, 0, 8);      // top right
    memset(qr + side * (side - 8), 0, 8); // bottom left
    // Vertically
    size_t bottom_start = (side - 7) * side + 7;
    for (uint32_t i = 0; i < 7; ++i) {
        qr[i * side + 7] = 0;            // top left
        qr[i * side + side - 8] = 0;     // top right
        qr[i * side + bottom_start] = 0; // bottom left
    }

    // Write the dark module
    size_t dark_module = side * (side - 8) + 8; // The position of the dark module
    qr[dark_module] = 1;

    // Write alignment patterns
    uint32_t coord_pairs = align_pat_side_coords(ver);
    for (uint32_t a = 0; a < coord_pairs; ++a) {
        for (uint32_t b = 0; b < coord_pairs; ++b) {
            size_t offset = 7 * (ver - 2);
            uint32_t x = align_pat_coords[offset + a];
            uint32_t y = align_pat_coords[offset + b];
            if (is_align_pat(ver, x, y))
                write_square(qr, side, align_pat, 5, x - 2, y - 2);
        }
    }

    // Write timing patterns
    uint32_t start = 6 * side + 6;
    for (uint32_t i = 0; i < side - 13; ++i) {
        qr[start + i] = 1 - (i & 1);
        qr[start + i * side] = 1 - (i & 1);
    }
}

void write_format_info(Version ver, ErrorLevel lvl, uint32_t mask_pat, uint8_t* qr, size_t side) {
    const static uint16_t format_strs[32] = {
        0b111011111000100, 0b111001011110011,
        0b111110110101010, 0b111100010011101,
        0b110011000101111, 0b110001100011000,
        0b110110001000001, 0b110100101110110,
        0b101010000010010, 0b101000100100101,
        0b101111001111100, 0b101101101001011,
        0b100010111111001, 0b100000011001110,
        0b100111110010111, 0b100101010100000,
        0b011010101011111, 0b011000001101000,
        0b011111100110001, 0b011101000000110,
        0b010010010110100, 0b010000110000011,
        0b010111011011010, 0b010101111101101,
        0b001011010001001, 0b001001110111110,
        0b001110011100111, 0b001100111010000,
        0b000011101100010, 0b000001001010101,
        0b000110100001100, 0b000100000111011,
    };

    // Write the format string
    // uint16_t format_str = 0b110011000101111;
    uint16_t format_str = format_strs[lvl * 8 + mask_pat];

    // Write the bottom-left format strip
    size_t start = side * (side - 7) + 8;
    for (uint32_t i = 0; i < 7; ++i) {
        qr[start + (side * i)] = format_str >> (8 + i) & 1;
    }
    
    // Write the top-right format strip
    start = side * 8 + side - 8;
    for (uint32_t i = 0; i < 8; ++i) {
        qr[start + i] = format_str >> (7 - i) & 1;
    }

    // Write the top-left format strip

    // The three bits in the corner
    // (We do these separately since the timing
    // pattern interferes with the regular line)
    size_t bit_7_pos = side * 8 + 8;
    qr[bit_7_pos - 1]    = format_str >> 8 & 1;
    qr[bit_7_pos]        = format_str >> 7 & 1;
    qr[bit_7_pos - side] = format_str >> 6 & 1;

    // Top-left bottom row
    start = side * 8;
    for (uint32_t i = 0; i < 6; ++i) {
        qr[start + i] = format_str >> (14 - i) & 1;
    }

    // Top-left right column
    start = 8;
    for (uint32_t i = 0; i < 6; ++i) {
        qr[start + (side * i)] = format_str >> i & 1;
    }
}

void write_version_info(Version ver, uint8_t* qr, size_t side) {
    const static uint32_t version_strs[34] = {
        0b000111110010010100, 0b001000010110111100,
        0b001001101010011001, 0b001010010011010011,
        0b001011101111110110, 0b001100011101100010,
        0b001101100001000111, 0b001110011000001101,
        0b001111100100101000, 0b010000101101111000,
        0b010001010001011101, 0b010010101000010111,
        0b010011010100110010, 0b010100100110100110,
        0b010101011010000011, 0b010110100011001001,
        0b010111011111101100, 0b011000111011000100,
        0b011001000111100001, 0b011010111110101011,
        0b011011000010001110, 0b011100110000011010,
        0b011101001100111111, 0b011110110101110101,
        0b011111001001010000, 0b100000100111010101,
        0b100001011011110000, 0b100010100010111010,
        0b100011011110011111, 0b100100101100001011,
        0b100101010000101110, 0b100110101001100100,
        0b100111010101000001, 0b101000110001101001,
    };

    uint32_t version_str = version_strs[ver - 7];

    // Top-right version block
    uint32_t start = side - 11;
    for (uint32_t y = 0; y < 6; y++) {
        for (uint32_t x = 0; x < 3; x++) {
            size_t pos = start + (y * side) + x;
            uint32_t index = x * 3 + y;
            qr[pos] = version_str >> index & 1;
        }
    }

    // Top-right version block
    start = side * (side - 11);
    for (uint32_t x = 0; x < 6; x++) {
        for (uint32_t y = 0; y < 3; y++) {
            size_t pos = start + (y * side) + x;
            uint32_t index = x * 3 + y;
            qr[pos] = version_str >> index & 1;
        }
    }
}

void write_data(Version ver, MaskFn mask, uint8_t* qr, size_t side, uint8_t* data, size_t size) {
    size_t pos = side * side - 1; // The position on the qr code (start bottom-right)
    size_t idx = 0;               // The index of the current bit in data
    size_t stop = size * 8;       // TODO: GET STOP MODULE
    bool is_up = true;            // Is the current snake pattern moving up or down
    bool dir = 0;                 // 0 = move left, 1 = move diagonally up/down
    while (idx < stop) {
        if (qr[pos] == 2) {
            // Write the current bit to each mask according to its mask rules
            uint8_t byte = data[idx >> 3];
            uint8_t bit = (byte >> ((7 - idx) & 7)) & 1;
            size_t x = pos % side;
            size_t y = pos / side;
            qr[pos] = bit ^ mask(x, y);
            ++idx;
        }

        if (dir) { // Move diagonally
            // Check if top or bottom has been hit
            bool top = pos < side;
            bool bottom = pos > side * side - side;

            if (is_up) {
                if (top) {
                    pos -= 1;
                    is_up = false;
                } else {
                    pos -= side - 1;
                }
            } else {
                if (bottom) {
                    pos -= 1;
                    is_up = true;
                } else {
                    pos += side + 1;
                }
            }
        } else { // Move left
            --pos;
        }

        dir = !dir;
    }

    // Add remainder bits
    pos -= side - 1;
    printf("SIDE: %zu", side);
    printf("POSITION: X: %zu, Y: %zu\n", pos % side, pos / side);

    stop = side * 8;
    while (pos > stop) {
        size_t x = pos % side;
        size_t y = pos / side;
        qr[pos] = 0 ^ mask(x, y);
        pos -= side;
        printf("x: %zu y: %zu\n", x, y);
    }
}

uint8_t* create_qr(Version ver, ErrorLevel lvl, uint8_t* data, size_t size) {
    // Side length of the qr code
    uint32_t side = (ver - 1) * 4 + 21;

    // Allocate space for 8 versions of the qr code (for masking)
    uint8_t* masked_qrs[8];
    for (uint32_t i = 0; i < 8; ++i)
        masked_qrs[i] = (uint8_t*)malloc(side * side);

    // Write the functional patterns to the first one
    // then copy them into the rest of the qr code versions
    uint8_t* qr = masked_qrs[0];

    // Fill the code with 2 (unwritten) so that we can determine
    // which modules are for data after we write the function
    // patterns and format info (since 1s and 0s are written for function patterns)
    memset(qr, 2, side * side);

    // Write the function patterns
    write_function_patterns(ver, qr, side);

    // Write version information (version 7+)
    if (ver > 6)
        write_version_info(ver, qr, side);

    // Repeat for every mask type
    for (uint8_t m = 0; m < 8; ++m) {
        // Write the version and format information
        // write_format_info(ver, lvl, m, masked_qrs[m], side);

        // Write the data modules (data and error correction codes)
        // write_data(ver, mask_pats[m], masked_qrs[m], side, data, size);
    }
    write_format_info(ver, lvl, 0, masked_qrs[0], side);
    write_data(ver, mask_pats[0], qr, side, data, size);

    // TODO: Evaluate the masks
    
    // Delete the unneeded masks
    size_t chosen_one = 0;
    qr = masked_qrs[chosen_one];
    for (uint32_t i = 0; i < 8; ++i) {
        if (i == chosen_one)
            continue;
        free(masked_qrs[i]);
    }

    return qr;
}

void print_qr(const uint8_t* data, Version ver) {
    uint32_t side = (ver - 1) * 4 + 21;
    
    for (uint32_t y = 0; y < side; ++y) {
        for (uint32_t x = 0; x < side; ++x) {
            if (data[y * side + x] == 0)
                printf(" ");
            else if (data[y * side + x] == 1)
                printf("#");
            else if (data[y * side + x] == 2)
                printf(".");
        }

        printf("\n");
    }
}

// 1: Error correction codewords per block
// 2: Number of blocks in first group
// 3: Number of data codewords in each block
// 4: Number of blocks in second group
// 5: Number of data codewords in each block
// Data taken from https://www.thonky.com/qr-code-tutorial/error-correction-table
const uint8_t error_table[] = {
    7, 1, 19, 0, 0, 10, 1, 16, 0, 0, 13, 1, 13, 0, 0,
    17, 1, 9, 0, 0, 10, 1, 34, 0, 0, 16, 1, 28, 0, 0,
    22, 1, 22, 0, 0, 28, 1, 16, 0, 0, 15, 1, 55, 0, 0,
    26, 1, 44, 0, 0, 18, 2, 17, 0, 0, 22, 2, 13, 0, 0,
    20, 1, 80, 0, 0, 18, 2, 32, 0, 0, 26, 2, 24, 0, 0,
    16, 4, 9, 0, 0, 26, 1, 108, 0, 0, 24, 2, 43, 0, 0,
    18, 2, 15, 2, 16, 22, 2, 11, 2, 12, 18, 2, 68, 0, 0,
    16, 4, 27, 0, 0, 24, 4, 19, 0, 0, 28, 4, 15, 0, 0,
    20, 2, 78, 0, 0, 18, 4, 31, 0, 0, 18, 2, 14, 4, 15,
    26, 4, 13, 1, 14, 24, 2, 97, 0, 0, 22, 2, 38, 2, 39,
    22, 4, 18, 2, 19, 26, 4, 14, 2, 15, 30, 2, 116, 0, 0,
    22, 3, 36, 2, 37, 20, 4, 16, 4, 17, 24, 4, 12, 4, 13,
    18, 2, 68, 2, 69, 26, 4, 43, 1, 44, 24, 6, 19, 2, 20,
    28, 6, 15, 2, 16, 20, 4, 81, 0, 0, 30, 1, 50, 4, 51,
    28, 4, 22, 4, 23, 24, 3, 12, 8, 13, 24, 2, 92, 2, 93,
    22, 6, 36, 2, 37, 26, 4, 20, 6, 21, 28, 7, 14, 4, 15,
    26, 4, 107, 0, 0, 22, 8, 37, 1, 38, 24, 8, 20, 4, 21,
    22, 12, 11, 4, 12, 30, 3, 115, 1, 116, 24, 4, 40, 5, 41,
    20, 11, 16, 5, 17, 24, 11, 12, 5, 13, 22, 5, 87, 1, 88,
    24, 5, 41, 5, 42, 30, 5, 24, 7, 25, 24, 11, 12, 7, 13,
    24, 5, 98, 1, 99, 28, 7, 45, 3, 46, 24, 15, 19, 2, 20,
    30, 3, 15, 13, 16, 28, 1, 107, 5, 108, 28, 10, 46, 1, 47,
    28, 1, 22, 15, 23, 28, 2, 14, 17, 15, 30, 5, 120, 1, 121,
    26, 9, 43, 4, 44, 28, 17, 22, 1, 23, 28, 2, 14, 19, 15,
    28, 3, 113, 4, 114, 26, 3, 44, 11, 45, 26, 17, 21, 4, 22,
    26, 9, 13, 16, 14, 28, 3, 107, 5, 108, 26, 3, 41, 13, 42,
    30, 15, 24, 5, 25, 28, 15, 15, 10, 16, 28, 4, 116, 4, 117,
    26, 17, 42, 0, 0, 28, 17, 22, 6, 23, 30, 19, 16, 6, 17,
    28, 2, 111, 7, 112, 28, 17, 46, 0, 0, 30, 7, 24, 16, 25,
    24, 34, 13, 0, 0, 30, 4, 121, 5, 122, 28, 4, 47, 14, 48,
    30, 11, 24, 14, 25, 30, 16, 15, 14, 16, 30, 6, 117, 4, 118,
    28, 6, 45, 14, 46, 30, 11, 24, 16, 25, 30, 30, 16, 2, 17,
    26, 8, 106, 4, 107, 28, 8, 47, 13, 48, 30, 7, 24, 22, 25,
    30, 22, 15, 13, 16, 28, 10, 114, 2, 115, 28, 19, 46, 4, 47,
    28, 28, 22, 6, 23, 30, 33, 16, 4, 17, 30, 8, 122, 4, 123,
    28, 22, 45, 3, 46, 30, 8, 23, 26, 24, 30, 12, 15, 28, 16,
    30, 3, 117, 10, 118, 28, 3, 45, 23, 46, 30, 4, 24, 31, 25,
    30, 11, 15, 31, 16, 30, 7, 116, 7, 117, 28, 21, 45, 7, 46,
    30, 1, 23, 37, 24, 30, 19, 15, 26, 16, 30, 5, 115, 10, 116,
    28, 19, 47, 10, 48, 30, 15, 24, 25, 25, 30, 23, 15, 25, 16,
    30, 13, 115, 3, 116, 28, 2, 46, 29, 47, 30, 42, 24, 1, 25,
    30, 23, 15, 28, 16, 30, 17, 115, 0, 0, 28, 10, 46, 23, 47,
    30, 10, 24, 35, 25, 30, 19, 15, 35, 16, 30, 17, 115, 1, 116,
    28, 14, 46, 21, 47, 30, 29, 24, 19, 25, 30, 11, 15, 46, 16,
    30, 13, 115, 6, 116, 28, 14, 46, 23, 47, 30, 44, 24, 7, 25,
    30, 59, 16, 1, 17, 30, 12, 121, 7, 122, 28, 12, 47, 26, 48,
    30, 39, 24, 14, 25, 30, 22, 15, 41, 16, 30, 6, 121, 14, 122,
    28, 6, 47, 34, 48, 30, 46, 24, 10, 25, 30, 2, 15, 64, 16,
    30, 17, 122, 4, 123, 28, 29, 46, 14, 47, 30, 49, 24, 10, 25,
    30, 24, 15, 46, 16, 30, 4, 122, 18, 123, 28, 13, 46, 32, 47,
    30, 48, 24, 14, 25, 30, 42, 15, 32, 16, 30, 20, 117, 4, 118,
    28, 40, 47, 7, 48, 30, 43, 24, 22, 25, 30, 10, 15, 67, 16,
    30, 19, 118, 6, 119, 28, 18, 47, 31, 48, 30, 34, 24, 34, 25,
    30, 20, 15, 61, 16,
};

uint8_t* get_final_message(uint8_t* msg, size_t msg_len, Version ver, ErrorLevel lvl) {
    size_t offset = 20 * (ver - 1) + 5 * lvl;
    // Number of error codewords for each block
    uint8_t err_cnt = error_table[offset];
    // Number of blocks in group 1
    uint8_t block_cnt_1 = error_table[offset + 1];
    // Number of data codewords in each group 1 block
    uint8_t word_cnt_1 = error_table[offset + 2];
    // Number of blocks in group 2
    uint8_t block_cnt_2 = error_table[offset + 3];
    // Number of data codewords in each group 2 block
    uint8_t word_cnt_2 = error_table[offset + 4];

    // Verify the length of msg
    const size_t total_data_words = (size_t)block_cnt_1 * (size_t)word_cnt_1 + (size_t)block_cnt_2 * (size_t)word_cnt_2;
    if (msg_len != total_data_words) {
        printf("get_final_message(): Suspicious msg len!\n");
        printf("Expected size: %zu, Actual size: %zu\n", total_data_words, msg_len);
        return NULL;
    }

    // Add one since 'error_cnt' is the degree, and
    // the number of terms is 1 more than that
    const size_t total_err_words = err_cnt * (block_cnt_1 + block_cnt_2);
    uint8_t* err_words = (uint8_t*)malloc(total_err_words);

    // Generate error correction codewords for each block
    size_t msg_offset = 0;
    size_t err_offset = 0;

    const uint8_t block_cnt = block_cnt_1 + block_cnt_2;

    // Generate the error correction codewords for each block in group 1
    for (uint32_t i = 0; i < block_cnt_1; ++i) {
        get_error_codewords(msg + msg_offset, word_cnt_1, err_words + err_offset, err_cnt);
        msg_offset += word_cnt_1;
        err_offset += err_cnt;
    }

    // Generate the error correction codewords for each block in group 2
    for (uint32_t i = 0; i < block_cnt_2; ++i) {
        get_error_codewords(msg + msg_offset, word_cnt_2, err_words + err_offset, err_cnt);
        msg_offset += word_cnt_2;
        err_offset += err_cnt;
    }

    // Structure final message
    const size_t final_size = total_data_words + total_err_words;
    uint8_t* final = (uint8_t*)malloc(final_size);

    // Interleave message codewords
    for (uint32_t i = 0; i < word_cnt_1; ++i) {
        for (uint32_t b = 0; b < block_cnt; ++b) {
            size_t y;
            if (b < block_cnt_1)
                y = b * word_cnt_1;
            else
                y = block_cnt_1 * word_cnt_1 + (b - block_cnt_1) * word_cnt_2;

            final[i * block_cnt + b] = msg[y + i];
        }
    }

    // Add on the extra words from group 2
    // If group 2 exists, the number of codewords in
    // each group 2 block is one more than group 1
    for (uint32_t b = 0; b < block_cnt_2; ++b)
        final[word_cnt_1 * block_cnt + b] = msg[word_cnt_1 * block_cnt_1 + (b + 1) * word_cnt_2 - 1];

    // Interleave error correction codewords
    for (uint32_t i = 0; i < err_cnt; ++i)
        for (uint32_t b = 0; b < block_cnt; ++b)
            final[msg_len + i * block_cnt + b] = err_words[b * err_cnt + i];

    free(err_words);

    return final;
}

void write_qr(const char* file, int img_width, const uint8_t* data, Version ver) {
    uint32_t side = (ver - 1) * 4 + 21;

    if (img_width < side) {
        printf("write_qr(): image_width too small: %d\n", img_width);
        return;
    }

    uint8_t* img_data = (uint8_t*)malloc(img_width * img_width);
    uint32_t square_width = img_width / side;

    for (uint32_t i = 0; i < img_width * img_width; ++i) {
        uint8_t module = data[(i / img_width) / square_width * side + (i % img_width) / square_width];
        if (module == 0)
            img_data[i] = 0xff;
        else if (module == 1)
            img_data[i] = 0x00;
        else if (module == 2)
            img_data[i] = 0x88;
    }

    if (!stbi_write_bmp(file, img_width, img_width, 1, img_data)) {
        printf("write_qr(): Could not write to file %s\n", file);
    }

    free(img_data);
}

