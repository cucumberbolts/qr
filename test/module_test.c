#include "module.c"

#include "error.c"

int test_word_generation() {
    uint8_t msg[] = {
        0b01000011, 0b01010101, 0b01000110, 0b10000110, 0b01010111, 0b00100110,
        0b01010101, 0b11000010, 0b01110111, 0b00110010, 0b00000110, 0b00010010,
        0b00000110, 0b01100111, 0b00100110, 0b11110110, 0b11110110, 0b01000010,
        0b00000111, 0b01110110, 0b10000110, 0b11110010, 0b00000111, 0b00100110,
        0b01010110, 0b00010110, 0b11000110, 0b11000111, 0b10010010, 0b00000110,
        0b10110110, 0b11100110, 0b11110111, 0b01110111, 0b00110010, 0b00000111,
        0b01110110, 0b10000110, 0b01010111, 0b00100110, 0b01010010, 0b00000110,
        0b10000110, 0b10010111, 0b00110010, 0b00000111, 0b01000110, 0b11110111,
        0b01110110, 0b01010110, 0b11000010, 0b00000110, 0b10010111, 0b00110010,
        0b11100000, 0b11101100, 0b00010001, 0b11101100, 0b00010001, 0b11101100,
        0b00010001, 0b11101100,
    };

    Version ver = 5;
    ErrorLevel lvl = ERROR_LEVEL_QUARTILE;

    uint8_t expected[] = {
        // Message codewords
        67, 246, 182, 70, 85, 246, 230, 247, 70, 66, 247, 118, 134, 7, 119,
        86, 87, 118, 50, 194, 38, 134, 7, 6, 85, 242, 118, 151, 194, 7,
        134, 50, 119, 38, 87, 224, 50, 86, 38, 236, 6, 22, 82, 17, 18, 198,
        6, 236, 6, 199, 134, 17, 103, 146, 151, 236, 38, 6, 50, 17, 7, 236,

        // Error correction codewords
        213, 87, 148, 140, 199, 204, 116, 100, 11,
        96, 177, 250, 45, 60, 212, 247, 115, 202,
        76, 108, 247, 182, 133, 131, 241, 124, 75,
        37, 223, 157, 242, 104, 229, 200, 238, 253,
        248, 134, 76, 113, 154, 27, 195, 111, 117,
        129, 230, 235, 154, 209, 189, 197, 111, 17,
        10, 83, 86, 163, 108, 6, 161, 163, 240,
        205, 111, 120, 192, 89, 39, 133, 141, 74,
    };

    uint8_t* final = get_final_message(msg, sizeof(msg), ver, lvl);

    int success = memcmp(final, expected, sizeof(expected)) == 0;

    free(final);

    return success;
}

int test_qr_image_write() {
#if 0
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
#endif

    return 1;
}

int main() {
    init_finite_field();
    init_generators();

    int success = 1;
    success &= test_word_generation();
    success &= test_qr_image_write();
    if (success) {
        printf("ALL TESTS COMPLETED SUCCESSFULLY!\n");
        exit(EXIT_SUCCESS);
    } else {
        printf("TEST FAILED!\n");
        exit(EXIT_FAILURE);
    }
}

