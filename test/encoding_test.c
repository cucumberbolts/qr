#include "qr.c"
#include "qr_write.c"

/* Print bytes in binary */
void print_bits(uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        for (int b = 0; b < 8; b++) {
            int bit = 1 & (data[i] >> (7 - b));
            putchar('0' + bit);
        }

        putchar(' ');
    }

    putchar('\n');
}

int test_numeric_encode() {
    printf("test_numeric_encode()\n");

    int success = 1;

    Symbol sym = create_symbol(1, ERROR_LEVEL_HIGH);

    {
        const char* str = "01234567";
        uint8_t expected[] = {
            0b00010000, 0b00100000,
            0b00001100, 0b01010110,
            0b01100001, 0b10000000,
            // Pad codewords
            0b11101100, 0b00010001,
            0b11101100,
        };

        encode_data(str, strlen(str), MODE_NUMERIC, &sym);

        printf("Expected:\n");
        print_bits(expected, sizeof(expected));

        printf("Result:\n");
        print_bits(sym.data, sym.data_size);

        success &= memcmp(expected, sym.data, sym.data_size) == 0;
        success &= sym.data_size == sizeof(expected);
    }

    {
        const char* str = "0123456789012345";
        uint8_t expected[] = {
            0b00010000, 0b01000000,
            0b00001100, 0b01010110,
            0b01101010, 0b01101110,
            0b00010100, 0b11101010,
            0b01010000,
        };

        encode_data(str, strlen(str), MODE_NUMERIC, &sym);

        printf("Expected:\n");
        print_bits(expected, sizeof(expected));

        printf("Result:\n");
        print_bits(sym.data, sym.data_size);

        success &= memcmp(expected, sym.data, sym.data_size) == 0;
        success &= sym.data_size == sizeof(expected);
    }

    delete_symbol(&sym);

    return success;
}

int test_alphanumeric_encode() {
    printf("test_alpha_numeric_encode()\n");

    int success = 1;

    Symbol sym = create_symbol(1, ERROR_LEVEL_HIGH);

    {
        const char* str = "AC-42";
        uint8_t expected[] = {
            0b00100000, 0b00101001,
            0b11001110, 0b11100111,
            0b00100001, 0b00000000,
            // Pad codewords
            0b11101100, 0b00010001,
            0b11101100,
        };

        encode_data(str, strlen(str), MODE_ALPHANUM, &sym);

        printf("Expected:\n");
        print_bits(expected, sizeof(expected));

        printf("Result:\n");
        print_bits(sym.data, sym.data_size);

        success &= memcmp(expected, sym.data, sym.data_size) == 0;
        success &= sym.data_size == sizeof(expected);
    }

    delete_symbol(&sym);

    return success;
}

int test_byte_encode() {
    printf("test_byte_encode()\n");

    int success = 1;

    Symbol sym = create_symbol(1, ERROR_LEVEL_HIGH);

    {
        uint8_t data[] = { 0xAA, 0xBB, 0xCC };
        uint8_t expected[] = {
            0b01000000, 0b00111010,
            0b10101011, 0b10111100,
            0b11000000,
            // Pad codewords
            0b11101100, 0b00010001,
            0b11101100, 0b00010001,
        };

        encode_data(data, sizeof(data), MODE_BYTE, &sym);

        printf("Expected:\n");
        print_bits(expected, sizeof(expected));

        printf("Result:\n");
        print_bits(sym.data, sym.data_size);

        success &= memcmp(expected, sym.data, sym.data_size) == 0;
        success &= sym.data_size == sizeof(expected);
    }

    delete_symbol(&sym);

    return success;
}

int main() {
    int success = 1;
    success &= test_numeric_encode();
    success &= test_alphanumeric_encode();
    success &= test_byte_encode();
    if (success) {
        printf("ALL TESTS COMPLETED SUCCESSFULLY!\n");
        exit(EXIT_SUCCESS);
    } else {
        printf("TEST FAILED!\n");
        exit(EXIT_FAILURE);
    }
}
