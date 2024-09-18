#include "ff.h"
#include <linux/slab.h>

static uint8_t inverse_elem(uint8_t a, uint8_t p);
static uint8_t complement_elem(uint8_t a, uint8_t p);
static bool ff_equal(ff_t *ff1, ff_t *ff2);
static void normalize_deg(ff_elem_t *a);
static uint64_t fast_power(uint8_t x, uint8_t n);
static ff_elem_t *ff_elem_pow(ff_elem_t *a, uint64_t n);
static ff_elem_t *copy_element(ff_elem_t *a);
/* Special voids for kernel */
void *xkmalloc(size_t size);
void *xkcalloc(size_t nmemb, size_t size);

void *xkmalloc(size_t size) {
  void *result = kmalloc(size, GFP_KERNEL);
  if (!result) {
    return NULL;
  }
  return result;
}

void *xkcalloc(size_t nmemb, size_t size) {
  void *result = kcalloc(nmemb, size, GFP_KERNEL);
  if (!result) {
    return NULL;
  }
  return result;
}

// x^8 + x^4 + x^3 + x^2 + 1
uint8_t ff_d8_p2_coeff[9] = {1, 0, 1, 1, 1, 0, 0, 0, 1};
ff_t ff_d8_p2 = {.char_p = 2, .deg = 8, .coeff = ff_d8_p2_coeff};

// x^16 + x^9 + x^8 + x^7 + x^6 + x^4 + x^3 + x^2 + 1
uint8_t ff_d16_p2_coeff[17] = {1, 0, 1, 1, 1, 0, 1, 1, 1,
                               1, 0, 0, 0, 0, 0, 0, 1};
ff_t ff_d16_p2 = {.char_p = 2, .deg = 16, .coeff = ff_d16_p2_coeff};

// x^32 + x^22 + x^2 + x^1 + 1
uint8_t ff_d32_p2_coeff[33] = {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                               1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
ff_t ff_d32_p2 = {.char_p = 2, .deg = 32, .coeff = ff_d32_p2_coeff};

static uint8_t complement_elem(uint8_t a, uint8_t p) { return (p - a) % p; }

static uint64_t fast_power(uint8_t x, uint8_t n) {
  uint64_t result = 1;
  uint64_t multiplier = x;
  while (n > 0) {
    if (n % 2 != 0) {
      result *= multiplier;
    }
    multiplier *= multiplier;
    n /= 2;
  }
  return result;
}

static uint8_t inverse_elem(uint8_t a, uint8_t p) {
  return fast_power(a, p - 2) % p;
}

static uint8_t get_deg(size_t length, uint8_t *coeff) {
  uint8_t deg = length - 1;
  while ((deg > 0) && (coeff[deg] == 0)) {
    deg -= 1;
  }
  return deg;
}

static void normalize_deg(ff_elem_t *a) {
  if (!a) {
    return;
  }
  a->deg = get_deg(a->deg + 1, a->coeff);
  return;
}

ff_elem_t *negative_ff_elem(ff_elem_t *a) {
  if (!a) {
    return NULL;
  }
  ff_elem_t *c = xkmalloc(sizeof(*c));
  if (!c) {
    return NULL;
  }
  uint8_t *coeff = xkmalloc(sizeof(uint8_t) * (a->deg + 1));
  if (!coeff) {
    ff_elem_free(c);
    return NULL;
  }
  for (size_t i = 0; i <= a->deg; i++) {
    coeff[i] = complement_elem(a->coeff[i], a->ff->char_p);
  }
  c->deg = a->deg;
  c->ff = a->ff;
  c->coeff = coeff;
  return c;
}

static bool ff_equal(ff_t *ff1, ff_t *ff2) {
  if (!ff1 || !ff2) {
    return false;
  }
  if (ff1->deg != ff2->deg) {
    return false;
  }
  int coeff_match =
      memcmp(ff1->coeff, ff2->coeff, sizeof(uint8_t) * (ff1->deg + 1));
  if (coeff_match != 0) {
    return false;
  }
  return true;
}

static ff_elem_t *copy_element(ff_elem_t *a) {
  if (!a) {
    return NULL;
  }
  ff_elem_t *b = ff_get_zero(a->ff);
  if (!b) {
    return NULL;
  }
  b->ff = a->ff;
  b->deg = a->deg;
  memcpy(b->coeff, a->coeff, a->ff->deg);
  return b;
}

static ff_elem_t *ff_elem_pow(ff_elem_t *a, uint64_t n) {
  ff_elem_t *result = ff_get_one(a->ff);
  ff_elem_t *tmp_base = copy_element(a);
  while (n > 0) {
    if (n % 2 != 0) {
      ff_elem_t *tmp = ff_multiply(result, tmp_base);
      ff_elem_free(result);
      result = tmp;
    }
    ff_elem_t *tmp = ff_multiply(tmp_base, tmp_base);
    ff_elem_free(tmp_base);
    tmp_base = tmp;
    n /= 2;
  }
  ff_elem_free(tmp_base);
  return result;
}

int ff_elem_cmp(ff_elem_t *a, ff_elem_t *b) {
  if (!a || !b) {
    return 0;
  } else if (!ff_equal(a->ff, b->ff)) {
    return 0;
  } else if (a->deg > b->deg) {
    return -1;
  } else if (a->deg < b->deg) {
    return 1;
  } else {
    return memcmp(a->coeff, b->coeff, sizeof(uint8_t) * (a->deg + 1));
  }
}

void ff_elem_free(ff_elem_t *m) {
  if (m) {
    kfree(m->coeff);
    kfree(m);
  }
  return;
}

ff_elem_t *ff_get_zero(ff_t *ff) {
  if (!ff) {
    return NULL;
  }
  ff_elem_t *zero = xkmalloc(sizeof(*zero));
  if (!zero) {
    return NULL;
  }
  zero->ff = ff;
  zero->deg = 0;
  zero->coeff = xkcalloc(ff->deg, sizeof(*zero->coeff));
  if (!zero->coeff) {
    kfree(zero);
    return NULL;
  }
  return zero;
}

ff_elem_t *ff_get_one(ff_t *ff) {
  if (!ff) {
    return NULL;
  }
  ff_elem_t *one = ff_get_zero(ff);
  if (!one) {
    return NULL;
  }
  one->coeff[0] = 1;
  return one;
}

static void modulo_poly(uint8_t length, uint8_t *coeff, ff_t *ff) {
  if (!ff || !coeff) {
    return;
  }

  if (length < ff->deg) {
    return;
  }

  for (size_t k = (length - 1 - ff->deg) + 1; k > 0; k--) {
    uint8_t q =
        coeff[(k - 1) + ff->deg] * inverse_elem(ff->coeff[ff->deg], ff->char_p);
    for (size_t j = ff->deg + (k - 1) + 1; j > (k - 1); j--) {
      uint8_t new_value = (q * ff->coeff[(j - 1) - (k - 1)]) % ff->char_p;
      new_value = complement_elem(new_value, ff->char_p);
      coeff[j - 1] = (coeff[j - 1] + new_value) % ff->char_p;
    }
  }
  return;
}

ff_elem_t *ff_elem_from_array(size_t length, uint8_t *coeff, ff_t *ff) {
  if (!coeff || !ff) {
    return NULL;
  }
  ff_elem_t *m = xkmalloc(sizeof(*m));
  if (!m) {
    return NULL;
  }
  m->ff = ff;
  m->coeff = xkcalloc(ff->deg, sizeof(uint8_t));
  if (!m->coeff) {
    ff_elem_free(m);
    return NULL;
  }
  for (size_t i = 0; i < length; i++) {
    coeff[i] %= ff->char_p;
  }
  size_t deg = get_deg(length, coeff);

  modulo_poly(deg + 1, coeff, ff);
  deg = get_deg(deg + 1, coeff);
  memcpy(m->coeff, coeff, sizeof(uint8_t) * (deg + 1));
  m->deg = deg;
  return m;
}

ff_elem_t *ff_sum(ff_elem_t *a, ff_elem_t *b) {
  if (!a || !b) {
    return NULL;
  }
  if (!ff_equal(a->ff, b->ff)) {
    return NULL;
  }
  ff_elem_t *c = ff_get_zero(a->ff);
  if (!c) {
    return NULL;
  }
  for (size_t i = 0; i < a->ff->deg; i++) {
    c->coeff[i] = (a->coeff[i] + b->coeff[i]) % a->ff->char_p;
  }
  c->deg = a->ff->deg - 1;
  normalize_deg(c);
  return c;
}

ff_elem_t *ff_multiply(ff_elem_t *a, ff_elem_t *b) {
  if (!a || !b) {
    return NULL;
  }
  if (!ff_equal(a->ff, b->ff)) {
    return NULL;
  }
  ff_elem_t *c = ff_get_zero(a->ff);
  if (!c) {
    return NULL;
  }
  uint8_t max_length = 2 * a->ff->deg;
  uint8_t tmp_coeff[max_length];
  memset(tmp_coeff, 0, sizeof(uint8_t) * max_length);
  for (size_t i = 0; i <= a->deg; i++) {
    for (size_t j = 0; j <= b->deg; j++) {
      tmp_coeff[i + j] =
          (tmp_coeff[i + j] + a->coeff[i] * b->coeff[j]) % a->ff->char_p;
    }
  }
  size_t deg = get_deg(max_length, tmp_coeff);
  modulo_poly(deg + 1, tmp_coeff, a->ff);
  deg = get_deg(deg + 1, tmp_coeff);
  memcpy(c->coeff, tmp_coeff, sizeof(uint8_t) * (deg + 1));
  c->deg = deg;
  return c;
}

ff_elem_t *ff_sub(ff_elem_t *a, ff_elem_t *b) {
  if (!a || !b) {
    return NULL;
  }
  ff_elem_t *negative_b = negative_ff_elem(b);
  ff_elem_t *c = ff_sum(a, negative_b);
  ff_elem_free(negative_b);
  normalize_deg(c);
  return c;
}

ff_elem_t *uint8_to_ff_elem(uint8_t x) {
  ff_elem_t *m = ff_get_zero(&ff_d8_p2);
  m->deg = 7;
  size_t i = 0;
  while (x > 0) {
    m->coeff[i] = x % 2;
    x /= 2;
    i++;
  }
  normalize_deg(m);
  return m;
}

uint8_t ff_elem_to_uint8(ff_elem_t *a) {
  uint8_t value = 0;
  uint8_t multiplier = 1;
  for (size_t i = 0; i <= a->deg; ++i) {
    value += a->coeff[i] * multiplier;
    multiplier *= 2;
  }
  return value;
}

ff_elem_t *uint16_to_ff_elem(uint16_t x) {
  ff_elem_t *m = ff_get_zero(&ff_d16_p2);
  m->deg = 15;
  size_t i = 0;
  while (x > 0) {
    m->coeff[i] = x % 2;
    x /= 2;
    i++;
  }
  normalize_deg(m);
  return m;
}

uint16_t ff_elem_to_uint16(ff_elem_t *a) {
  uint16_t value = 0;
  uint16_t multiplier = 1;
  for (size_t i = 0; i <= a->deg; ++i) {
    value += a->coeff[i] * multiplier;
    multiplier *= 2;
  }
  return value;
}

ff_elem_t *uint32_to_ff_elem(uint32_t x) {
  ff_elem_t *m = ff_get_zero(&ff_d32_p2);
  m->deg = 31;
  size_t i = 0;
  while (x > 0) {
    m->coeff[i] = x % 2;
    x /= 2;
    i++;
  }
  normalize_deg(m);
  return m;
}

uint32_t ff_elem_to_uint32(ff_elem_t *a) {
  uint32_t value = 0;
  uint32_t multiplier = 1;
  for (size_t i = 0; i <= a->deg; ++i) {
    value += a->coeff[i] * multiplier;
    multiplier *= 2;
  }
  return value;
}

ff_elem_t *inverse_ff_elem(ff_elem_t *a) {
  if (!a) {
    return NULL;
  }
  if ((a->deg == 0) && (a->coeff[0] == 1)) {
    return ff_get_one(a->ff);
  }
  return ff_elem_pow(a, fast_power(a->ff->char_p, a->ff->deg) - 2);
}

ff_elem_t *ff_divide(ff_elem_t *a, ff_elem_t *b) {
  if (!a || !b) {
    return NULL;
  }
  if (!ff_equal(a->ff, b->ff)) {
    return NULL;
  }
  if (b->deg == 0) {
    return NULL;
  }
  ff_elem_t *inverse_b = inverse_ff_elem(b);
  if (!inverse_b) {
    return NULL;
  }
  ff_elem_t *result = ff_multiply(a, inverse_b);
  ff_elem_free(inverse_b);
  return result;
}