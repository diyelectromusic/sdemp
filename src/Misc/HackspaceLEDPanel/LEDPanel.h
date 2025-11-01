/*
      MIT License
      
      Copyright (c) 2025 emalliab.wordpress.com (Kevin)
      
      Permission is hereby granted, free of charge, to any person obtaining a copy of
      this software and associated documentation files (the "Software"), to deal in
      the Software without restriction, including without limitation the rights to
      use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
      the Software, and to permit persons to whom the Software is furnished to do so,
      subject to the following conditions:
      
      The above copyright notice and this permission notice shall be included in all
      copies or substantial portions of the Software.
      
      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
      FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
      COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHERIN
      AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
      WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef _LEDPanel_H
#define _LEDPanel_H

#include <Arduino.h>

#define BRIGHTNESS_MIN 12
#define BRIGHTNESS_MAX 100

#define BITRED   4
#define BITGREEN 2
#define BITBLUE  1
enum panelcolour {
  led_black = 0,
  led_blue = BITBLUE,
  led_green = BITGREEN,
  led_cyan = BITBLUE | BITGREEN,
  led_red = BITRED,
  led_magenta = BITBLUE | BITRED,
  led_yellow = BITGREEN | BITRED,
  led_white = BITRED | BITGREEN | BITBLUE
};

void panelInit (void);
void panelScan (void);
void panelClear (bool on=false);
void setPixel (int x, int y, panelcolour col);
panelcolour getPixel (int x, int y);
void setBrightness (uint8_t brightpercent);

#endif
