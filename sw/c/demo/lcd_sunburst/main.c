// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "core/lucida_console_10pt.h"
#include "demo_system.h"
#include "gpio.h"
#include "lcd.h"
#include "sunburst-logo-small.h"
#include "spi.h"
#include "pwm.h"
#include "st7735/lcd_st7735.h"
#include "timer.h"

#define NUM_PWM_MODULES 12
#define NUM_LEDS 8

// Constants.
enum {
  // Pin out mapping.
  LcdCsPin = 0,
  LcdRstPin,
  LcdDcPin,
  LcdBlPin,
  LcdMosiPin,
  LcdSclkPin,
  // Spi clock rate.
  SpiSpeedHz = 5 * 100 * 1000,
};

// Buttons
typedef enum {
  BTN0 = 0b0001,
  BTN1 = 0b0010,
  BTN2 = 0b0100,
  BTN3 = 0b1000,
} Buttons_t;

// Local functions declaration.
static uint32_t spi_write(void *handle, uint8_t *data, size_t len);
static uint32_t gpio_write(void *handle, bool cs, bool dc);
static void timer_delay(uint32_t ms);

int main(void) {
  timer_init();

  // Set the initial state of the LCD control pins.
  set_output_bit(GPIO_OUT, LcdDcPin, 0x0);
  set_output_bit(GPIO_OUT, LcdBlPin, 0x1);
  set_output_bit(GPIO_OUT, LcdCsPin, 0x0);

  // Init spi driver.
  spi_t spi;
  spi_init(&spi, DEFAULT_SPI, SpiSpeedHz);

  // Reset LCD.
  set_output_bit(GPIO_OUT, LcdRstPin, 0x0);
  timer_delay(150);
  set_output_bit(GPIO_OUT, LcdRstPin, 0x1);

  // Init LCD driver and set the SPI driver.
  St7735Context lcd;
  LCD_Interface interface = {
      .handle      = &spi,         // SPI handle.
      .spi_write   = spi_write,    // SPI write callback.
      .gpio_write  = gpio_write,   // GPIO write callback.
      .timer_delay = timer_delay,  // Timer delay callback.
  };
  lcd_st7735_init(&lcd, &interface);

  // Set the LCD orientation.
  lcd_st7735_set_orientation(&lcd, LCD_Rotate180);

  // Setup text font bitmaps to be used and the colors.
  lcd_st7735_set_font(&lcd, &lucidaConsole_10ptFont);
  lcd_st7735_set_font_colors(&lcd, BGRColorWhite, BGRColorBlack);

  // Clean display with a white rectangle.
  lcd_st7735_clean(&lcd);

  // Draw the splash screen with a RGB 565 bitmap and text in the bottom.
  lcd_st7735_draw_rgb565(&lcd, (LCD_rectangle){.origin = {.x = 0, .y = 0}, .width = 160, .height = 98},
                         (uint8_t *)sunburst_logo);
  lcd_println(&lcd, "Sunburst - Sonata", alined_center, 8);

  uint32_t brightness = 0;
  uint32_t ascending = 1;
  uint64_t last_elapsed_time = get_elapsed_time();
  uint32_t toggle_count = 0;
  uint32_t toggle_state = 1;

  while(1) {
    timer_delay(10);

    for (int i = 3;i < NUM_PWM_MODULES; ++i) {
      set_pwm(PWM_FROM_ADDR_AND_INDEX(PWM_BASE, i), UINT8_MAX, brightness);
    }

    if (ascending) {
      if (brightness == 255) {
        brightness = 254;
        ascending = 0;
      } else {
        brightness++;
      }
    } else {
      if (brightness == 0) {
        ascending = 1;
        brightness = 1;
      } else {
        brightness--;
      }
    }

    toggle_count++;

    if (toggle_count == 100) {
      for (int i = 0;i < NUM_LEDS; ++i) {
        if (i % 2) {
          set_output_bit(GPIO_OUT, i + 4, toggle_state ? 0x1 : 0x0);
        } else {
          set_output_bit(GPIO_OUT, i + 4, toggle_state ? 0x0 : 0x1);
        }
      }

      toggle_count = 0;

      toggle_state = !toggle_state;
    }
  }

  return 0;
}

static uint32_t spi_write(void *handle, uint8_t *data, size_t len) {
  const uint32_t data_sent = len;
  while (len--) {
    spi_send_byte_blocking(handle, *data++);
  }
  while ((spi_get_status(handle) & spi_status_fifo_empty) != spi_status_fifo_empty)
    ;
  return data_sent;
}

static uint32_t gpio_write(void *handle, bool cs, bool dc) {
  set_output_bit(GPIO_OUT, LcdDcPin, dc);
  set_output_bit(GPIO_OUT, LcdCsPin, cs);
  return 0;
}

static void timer_delay(uint32_t ms) {
  // Configure timer to trigger every 1 ms
  timer_enable(50000);
  uint32_t timeout = get_elapsed_time() + ms;
  while (get_elapsed_time() < timeout) {
    asm volatile("wfi");
  }
  timer_disable();
}
