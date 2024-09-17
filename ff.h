#pragma once

#include <linux/types.h>

typedef struct ff
{   
    uint8_t char_p;
    uint8_t deg; 
    uint8_t *coeff; 
} ff_t;

typedef struct ff_elem
{
    ff_t *ff;
    uint8_t deg; 
    uint8_t *coeff;
} ff_elem_t;


extern ff_t ff_d8_p2;
extern ff_t ff_d16_p2;
extern ff_t ff_d32_p2;

void ff_elem_free (ff_elem_t *m);
int ff_elem_cmp(ff_elem_t *a, ff_elem_t *b);
ff_elem_t *ff_get_zero(ff_t *ff);
ff_elem_t *ff_get_one(ff_t *ff);
ff_elem_t *negative_ff_elem(ff_elem_t *a);
ff_elem_t *inverse_ff_elem(ff_elem_t *a);
ff_elem_t *ff_elem_from_array(size_t length, uint8_t *coeff, ff_t *ff);
ff_elem_t *ff_sum(ff_elem_t *a, ff_elem_t *b);
ff_elem_t *ff_sub(ff_elem_t *a, ff_elem_t *b);
ff_elem_t *ff_multiply(ff_elem_t *a, ff_elem_t *b);
ff_elem_t *ff_divide(ff_elem_t *a, ff_elem_t *b);
ff_elem_t *uint8_to_ff_elem(uint8_t x);
ff_elem_t *uint16_to_ff_elem(uint16_t x);
ff_elem_t *uint32_to_ff_elem(uint32_t x);
uint8_t ff_elem_to_uint8(ff_elem_t *a);
uint16_t ff_elem_to_uint16(ff_elem_t *a);
uint32_t ff_elem_to_uint32(ff_elem_t *a);
