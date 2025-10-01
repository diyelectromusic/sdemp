/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Mini Listening Post Experiments
//  
//
      MIT License
      
      Copyright (c) 2025 diyelectromusic (Kevin)
      
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
#include <Adafruit_GFX.h>
#include <Adafruit_ST77xx.h>
#ifndef __lcd2004_h
#define __lcd2004_h

// 16x5 character display on a
// 160x80 pixel display
#define MAX_X      16
#define MAX_Y       5
#define MAX_X_PXL 160
#define MAX_Y_PXL  80

// Each character is a 5x8 grid
// of 2x2 sized pixels
#define PXL_X      2
#define PXL_Y      2
#define CHAR_X     5
#define CHAR_Y     8
#define CHAR_W     (CHAR_X*PXL_X)
#define CHAR_H     (CHAR_Y*PXL_Y)

// Big characters are a 4x4 grid
// of the original non-big characters.
// Each display can show 1 row of 4 characters.
#define BIGCHAR_X  4  // Doesn't allow for any gaps between letters...
#define BIGCHAR_Y  4
#define MAX_BIGCHAR_X  4
#define MAX_BIGCHAR_Y  1
#define BIGCHAR_W  (BIGCHAR_X*CHAR_W)
#define BIGCHAR_H  (BIGCHAR_Y*CHAR_H)

// Allow for 1 extra "big char" on a scrolled display
#define MAX_X_PXL_SCROLLED (MAX_X_PXL+BIGCHAR_W)

class CLCD2004 {
public:
  CLCD2004 (Adafruit_ST77xx *tft, uint16_t fg, uint16_t bg, bool scrolled=false);
  ~CLCD2004 (void);

  void update (void);
  void scroll (uint16_t step, bool wrap=false);
  void setColour (uint16_t fg, uint16_t bg);

  void printclr (void);
  void print (char *c);
  void printbigchar (char c, uint16_t x, uint16_t colour);

  void printline (char *c, uint16_t x, uint16_t y, uint16_t colour);
  void printchar (char c, uint16_t x, uint16_t y, uint16_t colour);
  void printglyph (uint16_t g, uint16_t x, uint16_t y, uint16_t colour);
  void clearglyph (uint16_t x, uint16_t y, uint16_t colour);

  void clear (uint16_t colour);
  void clear (uint16_t x, uint16_t y, uint16_t colour);
  void fullBlock (uint16_t x, uint16_t y, uint16_t colour);
  void leftBlock (uint16_t x, uint16_t y, uint16_t colour);
  void rightBlock (uint16_t x, uint16_t y, uint16_t colour);
  void topBlock (uint16_t x, uint16_t y, uint16_t colour);
  void botBlock (uint16_t x, uint16_t y, uint16_t colour);
  void botLTri (uint16_t x, uint16_t y, uint16_t colour);
  void botRTri (uint16_t x, uint16_t y, uint16_t colour);
  void topLTri (uint16_t x, uint16_t y, uint16_t colour);
  void topRTri (uint16_t x, uint16_t y, uint16_t colour);

private:
  void drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h);
  void clearScrollBuf (void);

private:
  Adafruit_ST77xx *m_ptft;
  uint16_t m_fg;
  uint16_t m_bg;
  bool m_scrolled;
  GFXcanvas16 *m_pcanvas;
  Adafruit_GFX *m_pdisplay;
  int16_t m_scrollidx;

#define CHAR_BUF 64
  uint16_t m_cidx;
  char m_cbuf[CHAR_BUF+1];
};

#endif