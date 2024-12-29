#include <stdlib.h>

#include "error.c"

int test_generator_generation() {
    printf("test_generator_generation()\n");

    uint32_t degree = 10;
    printf("Degree 10 generator:\n");
    for (int i = 0; i <= degree; i++) {
        printf("a^%dx^%d ", get_generator(degree)[i], degree - i);
    }
    printf("\n");

    uint8_t expected[] = {
        0, 251, 67, 46, 61, 118, 70, 64, 94, 32, 45,
    };

    int success = memcmp(expected, get_generator(degree), sizeof(expected)) == 0;

    return success;
}

int test_hello_world_error() {
    printf("test_hello_world_error()\n");

    uint8_t msg[] = {
        32, 91, 11, 120, 209, 114, 220, 77,
        67, 64, 236, 17, 236, 17, 236, 17,
    };

    uint8_t* e_codewords = get_error_codewords(msg, sizeof(msg), 10);

    for (int i = 0; i < 10; i++) {
        printf("Codeword: %d\n", e_codewords[i]);
    }

    uint8_t expected[] = {
        196, 35, 39, 119, 235, 215, 231, 226, 93, 23,
    };

    int success = memcmp(expected, e_codewords, sizeof(expected)) == 0;

    free(e_codewords);

    return success;
}

int main() {
    init_finite_field();
    init_generators();

    int success = 1;
    success &= test_generator_generation();
    success &= test_hello_world_error();
    if (success) {
        printf("ALL TESTS COMPLETED SUCCESSFULLY!\n");
        exit(EXIT_SUCCESS);
    } else {
        printf("TEST FAILED!\n");
        exit(EXIT_FAILURE);
    }
}
