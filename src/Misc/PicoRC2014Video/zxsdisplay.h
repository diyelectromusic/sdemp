// SPDX-License-Identifier: MIT.
// With the added proviso that this code MUST NOT be used for training of AI systems.
//
// Copyright (c) 2026 Kevin (emalliab)
//
//
//  The ZX Spectrum Display mapped over to VGA/CGA
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//                      Blanking pre ZX display
//        +---------------------------------------------------+
//        |                  ZX Top border                    |
//        |         +------------------------------+-         |
//        |         |                              |          |
//        |    ZX   |         ZX Display           |    ZX    |
//        |   Left  |         256 x 192            |   Right  |
//        |  Border |                              |  Border  |
//        |         |                              |          |
//        |         +------------------------------+-         |
//        |                 ZX Bottom border                  |
//        +---------------------------------------------------+
//                      Blanking post ZX display
//
//  Main display: 256 x 192
//  Attribute grid: 32 x 24
//  Typical PAL TV has 312 scanlines
//  PAL Dimensions (from "The ZX Spectrum ULA" figure 9.1:
//     Top blanking:   8
//     Top border:    56
//     Display:      192
//     Bottom border: 56
//
//     Left Blanking: 96
//     Left Border:   32
//     Display:      256
//     Right Border:  64
//
//  Refs: https://en.wikipedia.org/wiki/ZX_Spectrum_graphic_modes
//        http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/screen-memory-layout
//        "The ZX Spectrum ULA: How To Design a Microcomputer", Chris Smith
//
// Mapping this over to a 640 x 480 VGA/CGA display each Spectrum pixel = 2x2 VGA pixels
//                   VGA    ZX pixels
//     Top border:    48       24
//     Display:      384      192
//     Bottom border: 48       24
//            Total: 480      240
//
//     Left border:   64       32
//     Display:      512      256
//     Right border:  64       32
//            Total: 640      320

// ZX Spectrum pixels definitions
#define ZXS_DISP_WIDTH    256
#define ZXS_DISP_HEIGHT   192
#define ZXS_BORDER_TOP     24
#define ZXS_BORDER_BOTTOM  24
#define ZXS_BORDER_LEFT    32
#define ZXS_BORDER_RIGHT   32

// Attribute cells are 8x8 pixels
#define ZXS_ATTR_WIDTH  32
#define ZXS_ATTR_HEIGHT 24
