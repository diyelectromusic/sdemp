// SPDX-License-Identifier: MIT.
// With the added proviso that this code MUST NOT be used for training of AI systems.
//
// Copyright (c) 2026 Kevin (emalliab)
//
#include "hardware/pio.h"  // For definitions used in checks

#if 1
#define VGA_RGBY1111
#define PICO_SCANVIDEO_COLOR_PIN_COUNT  4u
#define PICO_SCANVIDEO_DPI_PIXEL_RCOUNT 2u
#define PICO_SCANVIDEO_DPI_PIXEL_GCOUNT 1u
#define PICO_SCANVIDEO_DPI_PIXEL_BCOUNT 1u
#define PICO_SCANVIDEO_DPI_PIXEL_RSHIFT 2u
#define PICO_SCANVIDEO_DPI_PIXEL_GSHIFT 1u
#define PICO_SCANVIDEO_DPI_PIXEL_BSHIFT 0u
#define PICO_SCANVIDEO_COLOR_PIN_BASE   12u
//#define PICO_SCANVIDEO_COLOR_PIN_BASE   40u
#else
#define VGA_RGB222
#define PICO_SCANVIDEO_COLOR_PIN_COUNT  6u
#define PICO_SCANVIDEO_DPI_PIXEL_RCOUNT 2u
#define PICO_SCANVIDEO_DPI_PIXEL_GCOUNT 2u
#define PICO_SCANVIDEO_DPI_PIXEL_BCOUNT 2u
#define PICO_SCANVIDEO_DPI_PIXEL_RSHIFT 4u
#define PICO_SCANVIDEO_DPI_PIXEL_GSHIFT 2u
#define PICO_SCANVIDEO_DPI_PIXEL_BSHIFT 0u
#define PICO_SCANVIDEO_COLOR_PIN_BASE   0u
//#define PICO_SCANVIDEO_COLOR_PIN_BASE   40u
#endif

// If using all GPIO > 32 then this will adjust scanvideo.c
//#define PSVMASKOFFSET 32
// If using all GPIO < 32 then this will leave scanvideo.c as is
#define PSVMASKOFFSET 0

#define PSVMASK(x) ((x)-PSVMASKOFFSET)

#if PICO_PIO_USE_GPIO_BASE==0 && PSVMASKOFFSET==32
#error Cannot use more than 32 GPIO if not building for RP2350
#endif

#if PICO_PIO_USE_GPIO_BASE==0 && (PICO_SCANVIDEO_COLOR_PIN_BASE+PICO_SCANVIDEO_COLOR_PIN_COUNT)>31
#error Cannot set base and count to more than 31 if not building for RP2350
#endif

#if PICO_SCANVIDEO_COLOR_PIN_BASE>31 && PSVMASKOFFSET==0
#error If using higher GPIO then must set PSVMASK
#endif
