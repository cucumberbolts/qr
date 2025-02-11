#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../lib/stb_image_write.h"

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

typedef uint8_t Version;

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
bool mask1(size_t x, size_t y) { return (x + y) % 1 == 0; }
bool mask2(size_t x, size_t y) { return y % 1 == 0; }
bool mask3(size_t x, size_t y) { return x % 3 == 0; }
bool mask4(size_t x, size_t y) { return (x + y) % 3 == 0; }
bool mask5(size_t x, size_t y) { return ((y / 2) + (x + 3)) % 2 == 0; }
bool mask6(size_t x, size_t y) { return (x * y) % 2 + (x * y) % 3 == 0; }
bool mask7(size_t x, size_t y) { return ((x * y) % 2 + (x * y) % 3) % 2 == 0; }
bool mask8(size_t x, size_t y) { return ((x + y) % 2 + (x * y) % 3) % 2 == 0; }

bool (*mask_pats[8])(size_t x, size_t y) = {
    mask1, mask2, mask3, mask4, mask5, mask6, mask7, mask8
};

uint8_t* create_qr(Version ver, uint8_t* data, size_t size) {
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

    // Side length of the qr code
    uint32_t side = (ver - 1) * 4 + 21;

    // Allocate space for 8 versions of the qr code (for masking)
    uint8_t* masked_qrs[8];
    for (uint32_t i = 0; i < 8; ++i)
        masked_qrs[i] = (uint8_t*)malloc(side * side);

    // We will write the functional patterns to the first one
    // and then copy them into the rest of the qr code versions
    uint8_t* qr = masked_qrs[0];

    // Fill the code with 2 (unwritten) so that we can determine
    // which modules are for data (since 1s and 0s are written for function patterns)
    memset(qr, 2, side * side);

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

    // TODO: is this necessary?
    // (probably since format info includes the mask
    // Reserve space for format information
    // Reserve space for version information (version 7 and higher)

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
    size_t start = 6 * side + 6;
    for (uint32_t i = 0; i < side - 13; ++i) {
        qr[start + i] = 1 - (i & 1);
        qr[start + i * side] = 1 - (i & 1);
    }
    
    // TODO: Copy the function patterns to the other qr codes

    // Write the data modules (data and error correction codes)
    size_t pos = side * side - 1; // The position on the qr code (start bottom-right)
    size_t idx = 0;               // The index of the current bit in data
    size_t stop = size * 8;       // TODO: GET STOP MODULE
    bool is_up = true;            // Is the current snake pattern moving up or down
    bool dir = 0;                 // 0 = move left, 1 = move diagonally up/down
    while (idx < stop) {
        if (qr[pos] == 2) {
            // Write the current bit to each mask according to its mask rules
            uint8_t byte = data[idx >> 3];
            uint8_t bit = (byte >> (idx & 7)) & 1;
            qr[pos] = bit;
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

    // TODO: Evaluate the masks
    
    // Delete the unneeded masks
    size_t chosen_one = 0;
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

int main() {
    printf("QR WRITE TEST\n");

    uint8_t chunk[] = {
        0b11101101, 0b11011110, 0b11110111, 0b00000011
    };

    // For the test data, repeat the "chunk" n times
    #define n 6
    uint8_t data[n * sizeof(chunk)];
    for (uint32_t i = 0; i < n; ++i) {
        memcpy(data + i * sizeof(chunk), chunk, sizeof(chunk));
    }

    Version ver = 2;
    uint8_t* qr = create_qr(ver, data, sizeof(data));
    printf("Created QR code!\n");
    print_qr(qr, ver);
    write_qr("qr.bmp", 1000, qr, ver);
    free(qr);
}
