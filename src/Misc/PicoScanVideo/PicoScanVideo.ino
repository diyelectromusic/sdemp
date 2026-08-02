#define ZXCOLOURS
//#define HORPATTERN

#include "scanvideo.h"
#include "composable_scanline.h"

#define vga_mode vga_mode_320x240_60
static bool invert = false;

#define VGA_RGBY_1111(r,g,b,y) ((y<<3)|(r<<2)|(g<<1)|b)
static uint16_t zxd_colour_words[16] = {
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

#ifdef HORPATTERN
void draw_zxcolor_bar(scanvideo_scanline_buffer_t *buffer) {
    uint32_t line_num = scanvideo_scanline_number(buffer->scanline_id);
    uint32_t col = (line_num * 16ul) / vga_mode.height;
    uint16_t numpxls = vga_mode.width;

    uint16_t *p = (uint16_t *) buffer->data;

    // | jmp color_run | color | count-3 |
    *p++ = COMPOSABLE_COLOR_RUN ;
    *p++ = zxd_colour_words[col];
    *p++ = numpxls - 3;

    *p++ = COMPOSABLE_RAW_1P;
    *p++ = 0;
    *p++ = COMPOSABLE_EOL_ALIGN;

    buffer->data_used = ((uint32_t *) p) - buffer->data;
    assert(buffer->data_used < buffer->data_max);
    buffer->status = SCANLINE_OK;
}

#else

void draw_zxcolor_bar(scanvideo_scanline_buffer_t *buffer) {
    uint32_t line_num = scanvideo_scanline_number(buffer->scanline_id);
    uint32_t col = (line_num * 8ul) / vga_mode.height;
    uint bar_width = vga_mode.width / 2;

    uint16_t *p = (uint16_t *) buffer->data;

    // | jmp color_run | color | count-3 |
    *p++ = COMPOSABLE_COLOR_RUN ;
    *p++ = zxd_colour_words[col];
    *p++ = bar_width - 3;

    *p++ = COMPOSABLE_COLOR_RUN ;
    *p++ = zxd_colour_words[col+8];  // Bright version
    *p++ = bar_width - 3;

    // black pixel to end line
    *p++ = COMPOSABLE_RAW_1P;
    *p++ = 0;
    // end of line with alignment padding
    *p++ = COMPOSABLE_EOL_SKIP_ALIGN;
    *p++ = 0;

    buffer->data_used = ((uint32_t *) p) - buffer->data;
    assert(buffer->data_used < buffer->data_max);
    buffer->status = SCANLINE_OK;
}

#endif

void draw_color_bar(scanvideo_scanline_buffer_t *buffer) {
    // figure out 1/32 of the color value
    uint line_num = scanvideo_scanline_number(buffer->scanline_id);
    uint32_t primary_color = 1u + (line_num * 7 / vga_mode.height);
    uint32_t color_mask = PICO_SCANVIDEO_PIXEL_FROM_RGB5(0x1f * (primary_color & 1u), 0x1f * ((primary_color >> 1u) & 1u), 0x1f * ((primary_color >> 2u) & 1u));
    uint bar_width = vga_mode.width / 32;

    uint16_t *p = (uint16_t *) buffer->data;

    uint32_t invert_bits = invert ? PICO_SCANVIDEO_PIXEL_FROM_RGB5(0x1f,0x1f,0x1f) : 0;
    for (uint bar = 0; bar < 32; bar++) {
        *p++ = COMPOSABLE_COLOR_RUN;
        uint32_t color = PICO_SCANVIDEO_PIXEL_FROM_RGB5(bar, bar, bar);
        *p++ = (color & color_mask) ^ invert_bits;
        *p++ = bar_width - 3;
    }

    // 32 * 3, so we should be word aligned
    assert(!(3u & (uintptr_t) p));

    // black pixel to end line
    *p++ = COMPOSABLE_RAW_1P;
    *p++ = 0;
    // end of line with alignment padding
    *p++ = COMPOSABLE_EOL_SKIP_ALIGN;
    *p++ = 0;

    buffer->data_used = ((uint32_t *) p) - buffer->data;
    assert(buffer->data_used < buffer->data_max);

    buffer->status = SCANLINE_OK;
}

void setup() {
    Serial.begin(9600);
    Serial.print("VGA/CGA Demo Starting...");
    delay(1000);
}

bool toprint = true;
void loop() {
    delay (10000);
    if (toprint) {
        toprint = false;
        for (uint line_num = 0; line_num<240; line_num+=15) {
            uint32_t col = (line_num * 16ul) / vga_mode.height;
            uint16_t usedcol = zxd_colour_words[col];

            Serial.print("Line: ");
            Serial.print(line_num);
            Serial.print("\tColor: ");
            Serial.print(col, HEX);
            Serial.print("\tusedcol ");
            Serial.print(usedcol, HEX);
            Serial.print("\n");
        }
    }
}

void setup1() {
    // initialize video and interrupts on core 1
    scanvideo_setup(&vga_mode);
    scanvideo_timing_enable(true);
}

void loop1() {
    scanvideo_scanline_buffer_t *scanline_buffer = scanvideo_begin_scanline_generation(true);
#ifdef ZXCOLOURS
    draw_zxcolor_bar(scanline_buffer);
#else
    draw_color_bar(scanline_buffer);
#endif
    scanvideo_end_scanline_generation(scanline_buffer);
}
