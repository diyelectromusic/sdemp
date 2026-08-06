// SPDX-License-Identifier: MIT.
// With the added proviso that this code MUST NOT be used for training of AI systems.
//
// Copyright (c) 2026 Kevin (emalliab)
//
// Much of this code is taken from, or based on, that in:
// pico_zxspectrum::src/ZxSpectrumPrepareScanvideoScanline.cpp
//
#include "vgamode.h"
#include "zxscanline.h"
#include "composable_scanline.h"
#include "zxsdisplay.h"

#ifdef VGA_RGBY1111
#define VGA_RGBY_1111(r,g,b,y) ((y##UL<<3)|(r##UL<<2)|(g##UL<<1)|b##UL)
const uint32_t zxd_colour_words[16] = {
  VGA_RGBY_1111(0,0,0,0), // Black
  VGA_RGBY_1111(0,0,1,0), // Blue
  VGA_RGBY_1111(1,0,0,0), // Red
  VGA_RGBY_1111(1,0,1,0), // Magenta
  VGA_RGBY_1111(0,1,0,0), // Green
  VGA_RGBY_1111(0,1,1,0), // Cyan
  VGA_RGBY_1111(1,1,0,0), // Yellow
  VGA_RGBY_1111(1,1,1,0), // White
  VGA_RGBY_1111(0,0,0,0), // Bright Black
  VGA_RGBY_1111(0,0,1,1), // Bright Blue
  VGA_RGBY_1111(1,0,0,1), // Bright Red
  VGA_RGBY_1111(1,0,1,1), // Bright Magenta
  VGA_RGBY_1111(0,1,0,1), // Bright Green
  VGA_RGBY_1111(0,1,1,1), // Bright Cyan
  VGA_RGBY_1111(1,1,0,1), // Bright Yellow
  VGA_RGBY_1111(1,1,1,1)  // Bright White
};
#endif

#ifdef VGA_RGB222
#define VGA_RGB_222(r,g,b) ((r##UL<<4)|(g##UL<<2)|b##UL)
const uint16_t zxd_colour_words[16] = {
  VGA_RGB_222(0,0,0), // Black
  VGA_RGB_222(0,0,2), // Blue
  VGA_RGB_222(2,0,0), // Red
  VGA_RGB_222(2,0,2), // Magenta
  VGA_RGB_222(0,2,0), // Green
  VGA_RGB_222(0,2,2), // Cyan
  VGA_RGB_222(2,2,0), // Yellow
  VGA_RGB_222(2,2,2), // White
  VGA_RGB_222(0,0,0), // Bright Black
  VGA_RGB_222(0,0,3), // Bright Blue
  VGA_RGB_222(3,0,0), // Bright Red
  VGA_RGB_222(3,0,3), // Bright Magenta
  VGA_RGB_222(0,3,0), // Bright Green
  VGA_RGB_222(0,3,3), // Bright Cyan
  VGA_RGB_222(3,3,0), // Bright Yellow
  VGA_RGB_222(3,3,3)  // Bright White
};
#endif

static uint32_t zx_invert_masks[] = {
  0x00,
  0xff
};

static scanvideo_mode_t video_mode;

void zxs_scanline_init(const scanvideo_mode_t *vga_mode) {
  video_mode = *vga_mode;
  // We are using automatic doubling of pixels by using VGA 320x240 mode.
  // This means everything will be in ZX Spectrum pixel coordinates.
  assert ((video_mode.width == 320) && (video_mode.height == 240));
}

inline uint16_t* single_color_run(uint16_t *buf, uint32_t width, uint32_t ci) {
  // | color_run | color | count-3
  *buf++ = COMPOSABLE_COLOR_RUN;
  *buf++ = zxd_colour_words[ci];
  *buf++ = width - 3;
  return buf;
}

void zxs_scanline(
  struct scanvideo_scanline_buffer *scanline_buffer,
  uint32_t y, 
  uint32_t frame, 
  uint8_t* screenPtr,
  uint8_t* attrPtr,
  uint8_t borderColor
) {
  uint16_t* buf = (uint16_t *)scanline_buffer->data;

  if ((y < ZXS_BORDER_TOP) || (y >= (ZXS_BORDER_TOP+ZXS_DISP_HEIGHT))) {
    // Output the top or bottom border
    buf = single_color_run(buf, ZXS_BORDER_LEFT + ZXS_DISP_WIDTH + ZXS_BORDER_RIGHT, borderColor);
  } 
  else
  {
    // Output a line of display

    // Left border first
    buf = single_color_run(buf, ZXS_BORDER_LEFT, borderColor);
    
    // Now the main display
    const uint v = y - ZXS_BORDER_TOP;
    const uint8_t *s = screenPtr + ((v & 0x7) << 8) + ((v & 0x38) << 2) + ((v & 0xc0) << 5);
    const uint8_t *a = attrPtr+((v>>3)<<5);
    const int m = (frame >> 5) & 1;   // Frame number is on top 16 bits.  Use to toggle invert attribute
    
    for (int i = 0; i < 32; ++i) {
      uint8_t c = *a++; // Fetch the attribute for the character
      uint8_t p = *s++ ^ zx_invert_masks[(c >> 7) & m]; // fetch a byte of pixel data
      uint8_t bci = (c >> 3) & 0xf; // The background colour index
      uint8_t fci = (c & 7) | (bci & 0x8); // The foreground colour index

      uint32_t cw[2];
      cw[0] = zxd_colour_words[bci]; // The background colour word
      cw[1] = zxd_colour_words[fci]; // The foreground colour word
      
      // | raw_run | color1 | count-3 | color2 | color3 | ... | colorn |
      *buf++ = COMPOSABLE_RAW_RUN;
      *buf++ = cw[(p >> 7) & 1];
      *buf++ = (8 - 3);   // count = 8 pixels
      *buf++ = cw[(p >> 6) & 1];
      *buf++ = cw[(p >> 5) & 1];
      *buf++ = cw[(p >> 4) & 1];
      *buf++ = cw[(p >> 3) & 1];
      *buf++ = cw[(p >> 2) & 1];
      *buf++ = cw[(p >> 1) & 1];
      *buf++ = cw[(p >> 0) & 1];
    }
    
    // Border right
    buf = single_color_run(buf, ZXS_BORDER_RIGHT, borderColor);
  }

  // end with a black pixel
  *buf++ = COMPOSABLE_RAW_1P;
  *buf++ = 0;

  // Check for 32-bit alignment
  if ((3u & (uint32_t) buf) == 0) {
    *buf++ = COMPOSABLE_EOL_SKIP_ALIGN;
    *buf++ = 0;
  } else {
    *buf++ = COMPOSABLE_EOL_ALIGN;
  }

  scanline_buffer->data_used = (uint32_t *)buf - scanline_buffer->data;
  scanline_buffer->status = SCANLINE_OK;  
}
