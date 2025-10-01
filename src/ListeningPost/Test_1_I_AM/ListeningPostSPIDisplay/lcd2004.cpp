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
#include "lcd2004.h"
#include "lcd2004font.h"


CLCD2004::CLCD2004 (Adafruit_ST77xx *ptft, uint16_t fg, uint16_t bg, bool scrolled)
: m_ptft(ptft),
  m_fg(fg),
  m_bg(bg),
  m_scrolled(scrolled),
  m_pcanvas(nullptr),
  m_pdisplay(nullptr),
  m_scrollidx(0),
  m_cidx(0)
{
  if (m_scrolled) {
    m_pcanvas = new GFXcanvas16(MAX_X_PXL_SCROLLED, MAX_Y_PXL);
    if (m_pcanvas != nullptr) {
      if (m_pcanvas->getBuffer() != nullptr) {
        // Success...
        m_pdisplay = m_pcanvas;
      }
    }
  } else {
    m_pdisplay = ptft;
  }

  clearScrollBuf();
}

CLCD2004::~CLCD2004 (void) {
  if (m_pcanvas != nullptr) {
    delete (m_pcanvas);
    m_pcanvas = nullptr;
  }
  m_ptft = nullptr;
  m_pdisplay = nullptr;
}

void CLCD2004::clearScrollBuf (void) {
  // This is a +1 to allow for a final \0
  for (int i=0; i<CHAR_BUF+1; i++) {
    m_cbuf[i] = 0;
  }
  m_cidx = 0;
}

void CLCD2004::update (void) {
  if (m_scrolled && (m_pcanvas != nullptr) && (m_ptft != nullptr)) {
    // copy the contents of the in memory canvas
    // over to the display.
    m_ptft->drawRGBBitmap(0,0,(uint16_t *)m_pcanvas->getBuffer(), MAX_X_PXL_SCROLLED, MAX_Y_PXL);
    m_scrollidx=0;
  }
}

void CLCD2004::scroll (uint16_t step, bool wrap) {
  if (m_scrolled && (m_pcanvas != nullptr) && (m_ptft != nullptr)) {
    // copy the contents of the in memory canvas
    // over to the display.
    // Start at the current scroll index the increase it.
    //
    // We need the following Adafruit function, but this is pretty slow,
    // so instead I do it "by hand" to allow some optimisations...
    //m_ptft->drawRGBBitmap(m_scrollidx,0,(uint16_t *)m_pcanvas->getBuffer(), MAX_X_PXL_SCROLLED, MAX_Y_PXL);
    //
    drawRGBBitmap(-m_scrollidx,0,(uint16_t *)m_pcanvas->getBuffer(), MAX_X_PXL_SCROLLED, MAX_Y_PXL);

    m_scrollidx += step;
    if (m_scrollidx >= BIGCHAR_W) {
      // We've scrolled past the last character, so see if there is another one to add
      m_cidx++;
      if (m_cbuf[m_cidx] == 0) {
        if (wrap) {
          m_cidx = 0;
        }
      }
      if (m_cidx < CHAR_BUF) {
        int16_t wrapidx = 0;
        for (int i=0; i<MAX_BIGCHAR_X+1; i++) {
          if (m_cbuf[m_cidx+i] != 0) {
            printbigchar (' ', i, m_bg);
            printbigchar (m_cbuf[m_cidx+i], i, m_fg);
          } else {
            printbigchar (' ', i, m_bg);
            if (wrap) {
              // Grab characters from the start again
              printbigchar (m_cbuf[wrapidx], i, m_fg);
              wrapidx++;
            }
          }
        }
      }
      m_scrollidx = 0;
    }
  }
}

// Faster replacement for the following:
// Adafruit_GFX::drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h)
void CLCD2004::drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h) {

  int16_t _width = m_ptft->width();
  int16_t _height = m_ptft->height();

  // Want to find the overlap between the display area
  // and the bitmap, accepting that the bitmap could be
  // required to start before the display or part way into
  // the display, and could be larger or smaller than the
  // display...
  //
  // Notes:
  //   (x,y) -> display coords that the bitmap (0,0) will map to.
  //            Can be negative.
  //   (w,h) -> width and height of the bitmap to be displayed.
  //            Can be larger or smaller than display.
  //   bitmap -> bitmap in Canvas16 format.
  //
  // Find the start (x,y) and size (w,h) of the window to display.
  uint16_t win_x, win_y, win_w, win_h;
  if (x < 0) {
    // bitmap will start offscreen to the left
    win_x = 0;
    if (w + x > _width) {
      // bitmap is still longer than screen
      win_w = _width;
    } else {
      // bitmap ends before screen
      win_w = w + x;
    }
  } else if (x > 0) {
    // bitmap starts part way into the screen
    win_x = x;
    if (w + x > _width) {
      win_w = _width - x;
    } else {
      win_w = w - x;
    }
  } else {
    // x == 0; bitmap and display start in same place
    win_x = 0;
    if (w > _width) {
      // bitmap is wider than display, so clip
      win_w = _width;
    } else {
      // bitmap is same size or smaller
      win_w = w;
    }
  }
  if (y < 0) {
    // bitmap will start offscreen to the top
    win_y = 0;
    if (h + y > _height) {
      win_h = _height;
    } else {
      win_h = h + y;
    }
  } else if (y > 0) {
    // bitmap starts part way into the screen
    win_y = y;
    if (h + y > _height) {
      win_h = _height - y;
    } else {
      win_h = h - y;
    }
  } else {
    // y == 0
    win_y = 0;
    if (h > _height) {
      win_h = _height;
    } else {
      win_h = h;
    }
  }

  m_ptft->startWrite();
  m_ptft->setAddrWindow(win_x, win_y, win_w, win_h);

  // Now loop through the whole bitmap, but only
  // plot pixels that appear in the window for display.
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      int16_t p_x = x+i;
      int16_t p_y = y+j;
      if ((p_x >= win_x) && (p_x < win_x+win_w) && (p_y >= win_y) && (p_y < win_y+win_h)) {
        uint16_t p_color = bitmap[j * w + i];
        m_ptft->SPI_WRITE16(p_color);
      }
    }
  }
  m_ptft->endWrite();
}

void CLCD2004::setColour (uint16_t fg, uint16_t bg) {
  m_fg = fg;
  m_bg = bg;
}

void CLCD2004::printclr (void) {
  for (int xc=0; xc<MAX_BIGCHAR_X+1; xc++) {
    printbigchar(' ', xc, m_bg);
  }
  // Also clear out the scrolled buffer
  clearScrollBuf();
}

void CLCD2004::print (char *c) {
  if (c == nullptr) return;

  if (!m_scrolled) {
    uint16_t xc = 0;
    while ((c[xc] != 0) && (xc < MAX_BIGCHAR_X)) {
      printbigchar (' ', xc, m_bg);
      printbigchar (c[xc], xc, m_fg);
      m_cbuf[xc] = c[xc];
      xc++;
    }
  } else { // scrolled
    uint16_t xc = 0;
    while ((c[xc] != 0) && (xc < MAX_BIGCHAR_X+1) && (xc < CHAR_BUF)) {
      printbigchar (' ', xc, m_bg);
      printbigchar (c[xc], xc, m_fg);
      m_cbuf[xc] = c[xc];
      xc++;
    }
    // Store the remaining characters to add in later
    while ((c[xc] != 0) && (xc < CHAR_BUF)) {
      m_cbuf[xc] = c[xc];
      xc++;
    }
    // Zero out the rest of the buffer (inc +1)
    while (xc < CHAR_BUF+1) {
      m_cbuf[xc] = 0;
      xc++;
    }
    m_cidx = 0;
  }
}

void CLCD2004::printbigchar (char c, uint16_t x, uint16_t colour) {
  if ((c < LCD2004FONT_START) || (c >= LCD2004FONT_START+LCD2004FONT_SIZE)) {
    // Print space in the background colour
    clearglyph (x*BIGCHAR_X, 0, m_bg);
    clearglyph (x*BIGCHAR_X, 1, m_bg);
    clearglyph (x*BIGCHAR_X, 2, m_bg);
    clearglyph (x*BIGCHAR_X, 3, m_bg);
    return;
  }

  c = c - LCD2004FONT_START;

  printglyph (lcd2004font[c][0], x*BIGCHAR_X, 0, colour);
  printglyph (lcd2004font[c][1], x*BIGCHAR_X, 1, colour);
  printglyph (lcd2004font[c][2], x*BIGCHAR_X, 2, colour);
  printglyph (lcd2004font[c][3], x*BIGCHAR_X, 3, colour);
}

void CLCD2004::printglyph (uint16_t g, uint16_t x, uint16_t y, uint16_t colour) {
  // If the glyph is 0 then ignore - treat as "transparent"
  if (g & 0xF000) {
    printchar('0' + (g>>12 & 0xF), x, y, colour);
  }
  if (g & 0x0F00) {
    printchar('0' + (g>>8 & 0xF), x+1, y, colour);
  }
  if (g & 0x00F0) {
    printchar('0' + (g>>4 & 0xF), x+2, y, colour);
  }
  if (g & 0x000F) {
    printchar('0' + (g & 0xF), x+3, y, colour);
  }
}

void CLCD2004::clearglyph (uint16_t x, uint16_t y, uint16_t colour) {
  printchar('0', x, y, colour);
  printchar('0', x+1, y, colour);
  printchar('0', x+2, y, colour);
  printchar('0', x+3, y, colour);
}

void CLCD2004::printline (char *c, uint16_t x, uint16_t y, uint16_t colour) {
  if (c == nullptr) return;

  uint16_t xc = 0;
  while ((c[xc] != 0) && (xc < MAX_X)) {
    printchar (c[xc], xc+x, y, colour);
    xc++;
  }
}

void CLCD2004::printchar (char c, uint16_t x, uint16_t y, uint16_t colour) {
  switch (c) {
    case '0':
    case ' ':
      clear(x, y, colour);
      break;
    case '1':
      fullBlock(x, y, colour);
      break;
    case '2':
      leftBlock(x, y, colour);
      break;
    case '3':
      rightBlock(x, y, colour);
      break;
    case '4':
      topBlock(x, y, colour);
      break;
    case '5':
      botBlock(x, y, colour);
      break;
    case '6':
      botLTri(x, y, colour);
      break;
    case '7':
      botRTri(x, y, colour);
      break;
    case '8':
      topLTri(x, y, colour);
      break;
    case '9':
      topRTri(x, y, colour);
      break;
  }
}

void CLCD2004::clear (uint16_t colour) {
  if (m_pdisplay==nullptr) {
    return;
  }

  m_pdisplay->fillScreen(colour);
}

void CLCD2004::clear (uint16_t x, uint16_t y, uint16_t colour) {
  if (m_pdisplay==nullptr) {
    return;
  }

  m_pdisplay->fillRect(x*CHAR_W, y*CHAR_H, CHAR_W, CHAR_H, colour);
}

void CLCD2004::fullBlock (uint16_t x, uint16_t y, uint16_t colour) {
  if (m_pdisplay==nullptr) {
    return;
  }

  m_pdisplay->fillRect(1+x*CHAR_W, 1+y*CHAR_H, CHAR_W-1, CHAR_H-1, colour);
}

void CLCD2004::leftBlock (uint16_t x, uint16_t y, uint16_t colour) {
  if (m_pdisplay==nullptr) {
    return;
  }

  m_pdisplay->fillRect(1+x*CHAR_W, 1+y*CHAR_H, CHAR_W/2-1, CHAR_H-1, colour);
}

void CLCD2004::rightBlock (uint16_t x, uint16_t y, uint16_t colour) {
  if (m_pdisplay==nullptr) {
    return;
  }

  m_pdisplay->fillRect(1+x*CHAR_W+CHAR_W/2, 1+y*CHAR_H, CHAR_W/2-1, CHAR_H-1, colour);
}

void CLCD2004::topBlock (uint16_t x, uint16_t y, uint16_t colour) {
  if (m_pdisplay==nullptr) {
    return;
  }

  m_pdisplay->fillRect(1+x*CHAR_W, 1+y*CHAR_H, CHAR_W-1, CHAR_H/2-1, colour);
}

void CLCD2004::botBlock (uint16_t x, uint16_t y, uint16_t colour) {
  if (m_pdisplay==nullptr) {
    return;
  }

  m_pdisplay->fillRect(1+x*CHAR_W, 1+y*CHAR_H+CHAR_H/2, CHAR_W-1, CHAR_H/2-1, colour);
}

void CLCD2004::botLTri (uint16_t x, uint16_t y, uint16_t colour) {
  if (m_pdisplay==nullptr) {
    return;
  }

  uint16_t xlen = 0;
  for (int i=1; i<CHAR_H; i++) {
    m_pdisplay->drawFastHLine(1+x*CHAR_W, i+y*CHAR_H, xlen/10, colour);
    xlen = xlen + (CHAR_W*10-1)/CHAR_H;
  }
}

void CLCD2004::botRTri (uint16_t x, uint16_t y, uint16_t colour) {
  if (m_pdisplay==nullptr) {
    return;
  }

  uint16_t xlen = 0;
  for (int i=1; i<CHAR_H; i++) {
    m_pdisplay->drawFastHLine(x*CHAR_W+CHAR_W-xlen/10, i+y*CHAR_H, xlen/10, colour);
    xlen = xlen + (CHAR_W*10-1)/CHAR_H;
  }
}

void CLCD2004::topLTri (uint16_t x, uint16_t y, uint16_t colour) {
  if (m_pdisplay==nullptr) {
    return;
  }

  uint16_t xlen = 0;
  for (int i=CHAR_H-1; i>0; i--) {
    m_pdisplay->drawFastHLine(1+x*CHAR_W, i+y*CHAR_H, xlen/10, colour);
    xlen = xlen + (CHAR_W*10-1)/CHAR_H;
  }
}

void CLCD2004::topRTri (uint16_t x, uint16_t y, uint16_t colour) {
  if (m_pdisplay==nullptr) {
    return;
  }

  uint16_t xlen = 0;
  for (int i=CHAR_H-1; i>0; i--) {
    m_pdisplay->drawFastHLine(x*CHAR_W+CHAR_W-xlen/10, i+y*CHAR_H, xlen/10, colour);
    xlen = xlen + (CHAR_W*10-1)/CHAR_H;
  }
}

