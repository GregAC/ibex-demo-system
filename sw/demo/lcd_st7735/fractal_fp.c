#include "fractal_fp.h"
#include "lcd.h"
#include <stdint.h>

#define FP_EXP 12
#define FP_MANT 15

typedef uint32_t cmplx_packed_t;

uint16_t rgb_iters_palette[51] = {
  0x91e7,
  0x7ca7,
  0x5ca7,
  0x44a7,
  0x3ca9,
  0x3cab,
  0x3cad,
  0x3caf,
  0x3cb1,
  0x3cb2,
  0x3c52,
  0x3bf2,
  0x3b92,
  0x3b52,
  0x3b12,
  0x3ab2,
  0x3a72,
  0x3a32,
  0x39f2,
  0x41f2,
  0x49f2,
  0x51f2,
  0x59f2,
  0x59f2,
  0x61f2,
  0x69f2,
  0x71f2,
  0x79f2,
  0x79f2,
  0x81f2,
  0x89f2,
  0x91f2,
  0x91f2,
  0x91f1,
  0x91f1,
  0x91f0,
  0x91ef,
  0x91ef,
  0x91ee,
  0x91ed,
  0x91ed,
  0x91ec,
  0x91eb,
  0x91eb,
  0x91ea,
  0x91ea,
  0x91e9,
  0x91e9,
  0x91e8,
  0x91e7,
  0x91e7
};

typedef struct {
  int32_t real;
  int32_t imag;
} cmplx_t;

inline cmplx_packed_t cmplx_mul_insn(uint32_t a, uint32_t b) {
  uint32_t result;

  asm (".insn r CUSTOM_0, 0, 0, %0, %1, %2" :
       "=r"(result) :
       "r"(a), "r"(b));

  return result;
}

inline cmplx_packed_t cmplx_add_insn(uint32_t a, uint32_t b) {
  uint32_t result;

  asm (".insn r CUSTOM_0, 1, 0, %0, %1, %2" :
       "=r"(result) :
       "r"(a), "r"(b));

  return result;
}

inline int32_t cmplx_abs_sq_insn(uint32_t a) {
  int32_t result;

  asm (".insn r CUSTOM_0, 2, 0, %0, %1, x0" :
       "=r"(result) :
       "r"(a));

  return result;
}

int32_t fp_clamp(int32_t x) {
  if ((x < 0) && (x < -(1 << FP_MANT))) {
      return -(1 << FP_MANT);
  }

  if ((x > 0) && (x >= (1 << FP_MANT))) {
    return (1 << FP_MANT) - 1;
  }

  return x;
}

int32_t to_fp(int32_t x) {
  int32_t res;
  res = fp_clamp(x << FP_EXP);

  return res;
}

int32_t fp_add(int32_t a, int32_t b) {
  return fp_clamp(a + b);
}

int32_t fp_mul(int32_t a, int32_t b) {
  return fp_clamp((a * b) >> FP_EXP);
}

int32_t cmplx_abs_sq(cmplx_t c) {
  return fp_add(fp_mul(c.real, c.real), fp_mul(c.imag, c.imag));
}

cmplx_t cmplx_sq(cmplx_t c) {
  cmplx_t res;

  res.real = fp_add(fp_mul(c.real, c.real), -fp_mul(c.imag, c.imag));
  res.imag = fp_mul(to_fp(2), fp_mul(c.real, c.imag));

  return res;
}

cmplx_t cmplx_add(cmplx_t c1, cmplx_t c2) {
  cmplx_t res;

  res.real = fp_add(c1.real, c2.real);
  res.imag = fp_add(c1.imag, c2.imag);

  return res;
}


cmplx_packed_t pack_cmplx(cmplx_t c) {
  return ((c.real & 0xffff) << 16) | (c.imag & 0xffff);
}

int mandel_iters(cmplx_t c, uint32_t max_iters) {
  cmplx_packed_t c_packed = pack_cmplx(c);
  cmplx_packed_t iter_val;

  iter_val = c_packed;
  for (uint32_t i = 0; i < max_iters; ++i) {
    iter_val = cmplx_add_insn(cmplx_mul_insn(iter_val, iter_val), c_packed);
    if (cmplx_abs_sq_insn(iter_val) > to_fp(4)) {
      return i;
    }
  }

  return max_iters;
}

void fractal_mandelbrot_fp(St7735Context *lcd) {
  cmplx_t cur_p;
  int32_t inc;
  LCD_rectangle rectangle = {.origin = {.x = 16, .y = 0},
    .width = 128, .height = 128 };
  lcd_st7735_clean(lcd);
  lcd_st7735_rgb565_start(lcd, rectangle);

  cur_p.real = to_fp(-2);
  cur_p.imag = to_fp(-2);

  inc = 1 << (FP_EXP - 5);

  for(int x = 0;x < 128; ++x) {
    for(int y = 0;y < 128; ++y) {
      int iters = mandel_iters(cur_p, 50);

      uint16_t rgb = rgb_iters_palette[iters];
      lcd_st7735_rgb565_put(lcd, (uint8_t*)&rgb, sizeof(rgb));

      cur_p.real += inc;
    }

    cur_p.imag += inc;
    cur_p.real = to_fp(-2);
  }

  lcd_st7735_rgb565_finish(lcd);
}
