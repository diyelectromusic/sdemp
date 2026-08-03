// SPDX-License-Identifier: MIT.
// With the added proviso that this code MUST NOT be used for training of AI systems.
//
// Copyright (c) 2026 Kevin (emalliab)
//
#include "scanvideo.h"
#include "composable_scanline.h"
#include "zxsdisplay.h"
#include "zxscanline.h"

//------------------------------
// 64K RAM address space copy
//------------------------------
#define RAMSIZE (64*1024)  // 64K
uint8_t ram[RAMSIZE];

//------------------------------
//  VGA Output Details
//  Runs on core 0
//  Uses GPIO 40-45
//------------------------------
#define vga_mode vga_mode_320x240_60

#define ZXS_DISP_SIZE 0x1800  // 6144
#define ZXS_DISP_ADDR 0x4000  // 16384
#define ZXS_ATTR_SIZE 0x0300  // 768
#define ZXS_ATTR_ADDR 0x5800  // 22528

uint8_t borderColour = 0;

void setup() {
    for (int i=0; i<RAMSIZE; i++) {
      ram[i] = 0x00;
    }

    // initialize video and interrupts on core 1
    scanvideo_setup(&vga_mode);
    scanvideo_timing_enable(true);
    zxs_scanline_init(&vga_mode);
}

void loop() {
    scanvideo_scanline_buffer_t *buffer = scanvideo_begin_scanline_generation(true);
    uint32_t frame_num = scanvideo_frame_number(buffer->scanline_id);
    uint32_t y = scanvideo_scanline_number(buffer->scanline_id);

    zxs_scanline(buffer, y, frame_num, &ram[ZXS_DISP_ADDR], &ram[ZXS_ATTR_ADDR], borderColour);

    scanvideo_end_scanline_generation(buffer);
}

//------------------------------
//  Memory Monitor
//  Runs on core 1
//  Uses GPIO 0 to 31
//------------------------------

// GPIO Base 0 Definitions
// Start from GPIO 0 up to GPIO 31
//
#define GP_ADDR_START 0ul
#define GP_ADDR_PINS  16ul
#define GP_ADDR_MASK (0xFFFFul<<GP_ADDR_START)

#define GP_DATA_START 16ul
#define GP_DATA_PINS  8ul
#define GP_DATA_MASK (0xFFul<<GP_DATA_START)

#define GP_RD 24ul
#define GP_WR 25ul
#define GP_M1 28ul
#define GP_MREQ 30ul
#define GP_IORQ 31ul
#define GP_CTRL_MASK ((1ul<<GP_RD)|(1ul<<GP_WR)|(1ul<<GP_M1)|(1ul<<GP_MREQ)|(1ul<<GP_IORQ))
#define GP_CTRL_PINS 5ul
int ctrlPins[GP_CTRL_PINS] = {GP_RD, GP_WR, GP_M1, GP_MREQ, GP_IORQ};

#define GP_BASE0_MASK (GP_ADDR_MASK | GP_DATA_MASK | GP_CTRL_MASK)

void setup1() {
  delay(500);  // Let core 0 start up first

  // Pins in range lower than 32 can be initialised at once
  gpio_init_mask(GP_BASE0_MASK);
  gpio_set_dir_in_masked(GP_BASE0_MASK);
}

void loop1() {
  uint32_t gpio32 = gpio_get_all();

  // Look for /MEMRQ, /WR
  if ((gpio32 & GP_CTRL_MASK) == ((1<<GP_RD)|(0<<GP_WR)|(1<<GP_M1)|(0<<GP_MREQ)|(1<<GP_IORQ)))
  {
    // Grab the data off the bus and update RAM
    uint16_t addr16 = (GP_ADDR_MASK & gpio32) >> GP_ADDR_START;
    uint8_t data8 = (GP_DATA_MASK & gpio32) >> GP_DATA_START;
    ram[addr16] = data8;
  }
}
