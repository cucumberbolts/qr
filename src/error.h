#ifndef __ERROR_H__
#define __ERROR_H__

#include <stdint.h>

void init_finite_field();
void init_generators();

uint8_t* get_error_codewords(uint8_t* msg, size_t msg_len, uint32_t gen_deg);

#endif
