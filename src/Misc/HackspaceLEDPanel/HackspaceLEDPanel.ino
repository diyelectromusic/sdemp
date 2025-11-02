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
#include <TimerOne.h>
#include "LEDPanel.h"

// NOTE: Probably won't be able to do GOL alongside the others.
//       The Arduino Uno will run out of memory and do weird things...
//
//#define DO_BRIGHTNESS
//#define DO_LINES
//#define DO_MANDEL
#define DO_GOL

void testBrightness (void) {
  delay(1000);
  panelClear(true);
  delay(1000);
  for (int br=100; br>0; br-=10) {
    setBrightness(br);
    panelClear(true);
    delay(1000);
  }
  panelClear();
  delay(1000);
}

#define MAX_ITERATIONS 128
#define REALSTEP 64
#define IMAGSTEP 64
#define REALMAX 1.0
#define REALMIN -2.0
#define IMAGMAX 1.5
#define IMAGMIN -1.5
#define COLSTEPS 8
panelcolour cols[COLSTEPS] = {led_blue, led_green, led_cyan, led_magenta, led_red, led_yellow, led_white, led_black};
uint16_t mandelcalc(int x, int y) {
  double xstep = (REALMAX-REALMIN) / REALSTEP;
  double ystep = (IMAGMAX-IMAGMIN) / IMAGSTEP;
  double iY = IMAGMIN + ystep * y;
  double rX = REALMIN + xstep * x;
  double rZ = rX;
  double iZ = iY;

  int n;
  for (n=0; n < MAX_ITERATIONS; n++) {
    double a = rZ * rZ;
    double b = iZ * iZ;
    if (a+b > 4.0) break;
    iZ = 2 * rZ * iZ + iY;
    rZ = a - b + rX;
  }

  if (n == MAX_ITERATIONS) {
    return 0;
  } else {
    return n+1;
  }
}

void mandel (void) {
  for (int y=0; y<64; y++) {
    for (int x=0; x<64; x++) {
      uint8_t mval = mandelcalc(x, y);
      if (mval == 0) {
        setPixel(x, y, led_black);
      } else {
        setPixel(x, y, cols[mval % COLSTEPS]);
      }
    }
  }
}

#define GOL_COLS 8
panelcolour golcols[GOL_COLS] = {led_green, led_red, led_magenta, led_cyan, led_yellow, led_white, led_blue, led_black};
#define GOL_COL golcols[0]
#define GOL_OFF golcols[GOL_COLS-1]
#define GOL_X 64
#define GOL_Y 52
#define GOL_ITERATIONS 1000
#define DENSITY 7

uint16_t gol[GOL_Y][GOL_X/16];

void golBitSet (int x, int y) {
  if ((x<0) || (x>=GOL_X) || (y<0) || (y>=GOL_Y)) {
    return;
  }

  // x =  0..15 -> gol[y][0]
  // x = 16..31 -> gol[y][1]
  // x = 32..47 -> gol[y][2]
  // x = 48..63 -> gol[y][3]
  //
  // So xbit = x & 0x000F
  //    xidx = x >> 4
  //
  gol[y][x>>4] |= (1<<(x&0xF));
}

void golBitClear (int x, int y) {
  if ((x<0) || (x>=GOL_X) || (y<0) || (y>=GOL_Y)) {
    return;
  }
  gol[y][x>>4] &= ~(1<<(x&0xF));
}

bool golBitRead (int x, int y) {
  if ((x<0) || (x>=GOL_X) || (y<0) || (y>=GOL_Y)) {
    return false;
  }

  if ((gol[y][x>>4] & (1<<(x&0xF))) != 0) {
    return true;
  } else {
    return false;
  }
}

int golPixelRead (int x, int y) {
  // Handle wrapping
  if (x<0) {x=GOL_X-1;};
  if (x>=GOL_X) {x=0;};
  if (y<0) {y=GOL_Y-1;};
  if (y>=GOL_Y) {y=0;};
  
  if (getPixel(x,y) == GOL_COL) {
    return 1;
  } else {
    return 0;
  }
}

void golTick (int x, int y) {
  int neigh = 0;

  // Row above
  neigh += golPixelRead (x+1,y-1);
  neigh += golPixelRead (x,y-1);
  neigh += golPixelRead (x-1,y-1);
  
  // Row itself
  neigh += golPixelRead (x+1,y);
  neigh += golPixelRead (x-1,y);
  
  // Row below
  neigh += golPixelRead (x+1,y+1);
  neigh += golPixelRead (x,y+1);
  neigh += golPixelRead (x-1,y+1);
  
  int me = golPixelRead (x,y);
  
  if (me != 0) {
    if (neigh < 2) {
      // Rule 1 - fewer than 2 neighbours die
      golBitClear (x, y);
    }
    else if (neigh > 3) {
      // Rule 3 - more than 3 neighbours die
      golBitClear (x, y);
    }
    else {
      // Rule 2 - 2 or 3 neighbours lives
    }
  }
  else {
    // Rule 4 for dead cells
    if (neigh == 3) {
      // 3 neighbours reproduce
      golBitSet (x, y);
    }
  }
}

void gameoflife (void) {
  randomSeed(analogRead(A7));
  for (int x=0; x<GOL_X; x++) {
    for (int y=0; y<GOL_Y; y++) {
      if (random(10) > DENSITY) {
        golBitSet(x, y);
        setPixel(x, y, GOL_COL);
      } else {
        golBitClear(x, y);
        setPixel(x, y, GOL_OFF);
      }
    }
  }

  for (int i=0; i<GOL_ITERATIONS; i++) {
    for (int y=0; y<GOL_Y; y++) {
      for (int x=0; x<GOL_X; x++) {
        golTick(x, y);
      }
    }

    // Update grid/display
    for (int y=0; y<GOL_Y; y++) {
      for (int x=0; x<GOL_X; x++) {
        if (golBitRead(x, y)) {
          setPixel (x, y, GOL_COL);
        } else {
          // Cycle through the colours until get to "OFF"
          panelcolour col = getPixel (x, y);
          for (int c=0; c<GOL_COLS-1; c++) {
            if (col == golcols[c]) {
              setPixel (x, y, golcols[c+1]);
            }
          }
        }
      }
    }
  }
}

void setup() {
  panelInit();
  panelClear();

  Timer1.initialize(5000);  // 200Hz
  Timer1.attachInterrupt(panelScan);
  delay(500);

#ifdef DO_BRIGHTNESS
  testBrightness();
  setBrightness (BRIGHTNESS_MIN);
  panelClear();
  delay(1000);
#else
  setBrightness (BRIGHTNESS_MIN);
  panelClear(true);
  delay(2000);
  panelClear();
  delay(1000);
#endif

#ifdef DO_MANDEL
  mandel();
  delay(5000);
  panelClear();
#endif
}

#define DEL 5
void loop() {
#ifdef DO_GOL
  gameoflife();
#endif

#ifdef DO_LINES
  for (int x=0; x<64; x++) {
    for (int y=0; y<64; y++) {
      setPixel(x, y, led_blue);
      delay(DEL);
      setPixel(x, y, led_green);
      delay(DEL);
      setPixel(x, y, led_red);
      delay(DEL);
      setPixel(x, y, led_white);
      delay(DEL);
    }
    for (int y=0; y<64; y++) {
      setPixel(x, y, led_black);
    }
  }
#endif
}
