#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "qr.h"

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

// raises 2^x in GF(2^8)
// ex. converts 191 to 0b01000001
uint8_t ff_exp[255];

// retrieves exponent of field element
// ex. converts 0b01000001 to 191
uint8_t ff_log[255];

void init_finite_field() {
    // p(x) = x^8 + x^4 + x^3 + x^2 + 1

    memset(ff_log, 0, sizeof(ff_log));
    memset(ff_exp, 0, sizeof(ff_exp));

    uint8_t a8 = 0b00011101; // a^4 + a^3 + a^2 + 1
    uint8_t a  = 0b00000001; // 1

    for (int i = 0; i < 255; ++i) {
        ff_log[a] = i;
        ff_exp[i] = a;
        bool is_degree_7 = a & (1 << 7);
        a <<= 1;
        if (is_degree_7)
            a ^= a8;
    }
}

uint8_t ff_multiply(uint8_t a, uint8_t b) {
    return ff_exp[(ff_log[a] + ff_log[b]) % 255];
}

#define MAX_DEGREE 100

// a: the message polynomial (dividend), as integer values
// b: the generator polynomial (divisor), as exponent values
// a_deg: the degree of polynomial a
// b_deg: the degree of polynomial b
// a_shift: multiply the polynomial 'a' by x^a_shift
// Returns the coefficents of the remainder into rem
void poly_div(uint8_t* a, uint8_t* b, size_t a_deg, size_t b_deg, size_t a_shift, uint8_t* rem) {
    uint8_t buff[MAX_DEGREE] = { 0 };

    if (b_deg > a_deg)
         return;

    if (a_deg + 1 + a_shift > MAX_DEGREE) {
        printf("poly_div(): polynomial too big!\n");
        printf("Dividend degree: %zu Max degree: %d\n", a_deg + a_shift, MAX_DEGREE);
        return;
    }

    memcpy(buff, a, a_deg + 1);

    a_deg += a_shift;

    uint8_t coefficient = a[0];
    size_t i = 0;

    for (; i < a_deg - b_deg + 1; ++i) {
        // Multiply the divisor by the
        // leading coefficient of the dividend
        for (size_t j = 0; j < b_deg + 1; ++j) {
            // TODO: THIS IS A LITTLE DUBIOUS
            // (THE EXPONENT CAN EQUAL 0 FOR AN INTEGER VALUE OF 1)
            if (b[j] != 0)
                buff[i + j] ^= ff_exp[(b[j] + ff_log[coefficient]) % 255];
        }

        coefficient = buff[i + 1];
    }

    // Copy the remaining buffer terms into the remainder
    // The maximum degree of the remainder is (b_deg - 1)
    // Which means the remainder has b_deg number of terms
    memcpy(rem, buff + i, b_deg);
}

#define MAX_GEN_DEG 68
static uint8_t gen_coefficients[2346] = { 0 };

void init_generators() {
    // Determines the coefficients
    // to the generator polynomials
    // Biggest degree is 68

    // Initialize the generators with polynomial degree 1
    // (x - a^0) = (a^0*x - a^0)
    // gen_coefficients[0] = 0;
    // gen_coefficients[1] = 0;
    size_t offset = 2;

    // Generate polynomials up to and including degree 68
    for (uint32_t deg = 2; deg <= MAX_GEN_DEG; ++deg) {
        // First coefficient is always 1 or a^0
        gen_coefficients[offset] = 0;
        offset++;

        for (uint32_t i = 0; i < deg - 1; ++i) {
            uint8_t a = gen_coefficients[offset + i - deg];
            uint8_t b = gen_coefficients[offset + i - deg - 1];
            gen_coefficients[offset + i] = ff_log[ff_exp[a] ^ ff_exp[(b + deg - 1) % 255]];
        }

        uint8_t b = gen_coefficients[offset - 2];
        gen_coefficients[offset + deg - 1] = (b + deg - 1) % 255;

        offset += deg;
    }
}

uint8_t* get_generator(uint32_t degree) {
    // Returns a pointer to the beginning of the
    // coefficients of the specified degree, length degree + 1
    if (degree > MAX_GEN_DEG) {
        printf("get_generator(): Degree too big.\n");
        return NULL;
    }

    // subtract 1 since the table does not include
    // polynomial degree 0, which would have 1 coefficient
    const size_t offset = (degree * degree + degree) / 2 - 1;
    return &gen_coefficients[offset];
}

// TODO: IS THIS NEEDED?
// Returns the error correction codewords
// of length 'gen_deg' for an array of message codewords
uint8_t* get_error_codewords(uint8_t* msg, size_t msg_len, uint32_t gen_deg) {
    uint8_t* e_codewords = (uint8_t*)malloc(gen_deg);

    uint32_t msg_deg = msg_len - 1;
    poly_div(msg, get_generator(gen_deg), msg_deg, gen_deg, gen_deg, e_codewords);

    return e_codewords;
}


#if 0
int main() {
    init_finite_field();
    init_generators();

    //2^170 * 2^164 = 2^(170 + 164) = 2^334 = 2^(334%255) = 2^79
    //215 * 198 = 240
    // printf("%d\n", ff_exp[79]);
    // printf("%d\n", ff_exp[(ff_log[215] + ff_log[198]) % 255]);
    // printf("%d\n", ff_multiply(215, 198));
    // printf("%d\n", ff_mul8(215, 198, 0b100011101));

#if 0
    // Generator polynomial test
    // uint32_t degree = 68;
    // printf("Degree 10 generator:\n");
    // for (int i = 0; i <= degree; i++) {
    //     printf("a^%dx^%d ", get_generator(degree)[i], degree - i);
    // }
    // printf("\n");

    // Message polynomial (dividend)
    // Degree 15 * x^10 -> Degree 25
    uint8_t msg[26] = {
        32, 91, 11, 120, 209, 114, 220, 77,
        67, 64, 236, 17, 236, 17, 236, 17,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    // Generator polynomial (divisor)
    // degree 10
    uint8_t gen[26] = { 0 };
    memcpy(gen, get_generator(10), 11);

    for (int i = 0; i < 11; i++) {
        printf("GEN: %d\n", gen[i]);
    }

    // Suffix polynomial (remainder)
    uint8_t suf[100] = { 0 };

    poly_div(msg, gen, 16, 10, 10, suf);

    printf("REMAINDER:\n");
    for (int i = 0; i < 25; i++) {
        printf("%d\n", suf[i]);
    }

    // for (int i = 0; i < 20; ++i) {
    //     printf("%d\n", suf[i]);
    // }

    // uint8_t data[] = {
    //     0b01000001, 0b11010110, 0b10000111, 0b01000111, 0b01000111, 0b00000111,
    //     0b00110011, 0b10100010, 0b11110010, 0b11110110, 0b00010111, 0b01000110,
    //     0b00110110, 0b11010010, 0b11100110, 0b11010110, 0b00010111, 0b01000110,
    //     0b10000110, 0b00010110, 0b11100110, 0b01000111, 0b01000110, 0b01010110,
    //     0b00110110, 0b10000010, 0b11100110, 0b11110111, 0b00100110, 0b01110010,
    //     0b11110000, 0b11101100, 0b00010001, 0b11101100, 0b00010001, 0b11101100,
    // };

#else

    uint8_t msg[] = {
        32, 91, 11, 120, 209, 114, 220, 77,
        67, 64, 236, 17, 236, 17, 236, 17,
    };

    uint8_t* e_codewords = get_error_codewords(msg, sizeof(msg), 10);

    for (int i = 0; i < 10; i++) {
        printf("Codeword: %d\n", e_codewords[i]);
    }

    free(e_codewords);
#endif
}
#endif
