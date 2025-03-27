#ifndef __ERROR_H__
#define __ERROR_H__

#include <stdint.h>

void init_finite_field();
void init_generators();

// Given message 'msg' and length 'msg_len',
// computes 'codeword_cnt' number of error
// correction codewords into 'dst'
void get_error_codewords(uint8_t* msg, size_t msg_len, uint8_t* dst, uint32_t codeword_cnt);

#endif
