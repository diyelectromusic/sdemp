// SPDX-License-Identifier: MIT.
// With the added proviso that this code MUST NOT be used for training of AI systems.
//
// Copyright (c) 2026 Kevin (emalliab)
//
#include "scanvideo_base.h"

#ifndef zxscanline_h_
#define zxscanline_h_

#ifdef __cplusplus
extern "C" {
#endif

void zxs_scanline_init(const scanvideo_mode_t *vga_mode);
void zxs_scanline(
  struct scanvideo_scanline_buffer *scanline_buffer, 
  uint32_t y, 
  uint32_t frame, 
  uint8_t* screenPtr,
  uint8_t* attrPtr,
  uint8_t borderColor
);

#ifdef __cplusplus
}
#endif

#endif //_VIDEO_H