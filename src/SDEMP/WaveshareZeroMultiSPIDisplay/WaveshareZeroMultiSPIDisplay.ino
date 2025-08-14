/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Waveshare Zero Multi SPI Display
//  https://diyelectromusic.com/2025/08/14/arduino-with-multiple-displays-part-3/
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
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

// Defines the SPI routines such that up to eight SPI displays can
// be connected to a Waveshare Zero format board, using the same
// phyiscal pins as follows:
//          5V   1  1   TX
//         GND   2  2   RX
//         3V3   3  3
//         CS7   4  4   CS8
//         CS6   5  5   DC
//         CS5   6  6   CS1
//         CS4   7  7   SCL (SCLK)
//         CS3   8  8   SDA (MOSI)
//         CS2   9  9   RES

//#define WS_RP2040_ZERO
//#define WS_ESP32C3_ZERO
#define WS_ESP32S3_ZERO

#define NUM_TFTS 4
//#define NUM_TFTS 8
int tftTypes[NUM_TFTS] = {
// INITR_MINI160x80
// INITR_MINI160x80_PLUGIN   // Inverted display
   INITR_MINI160x80, INITR_MINI160x80,
   INITR_MINI160x80, INITR_MINI160x80,
#if (NUM_TFTS==8)
   INITR_MINI160x80, INITR_MINI160x80,
   INITR_MINI160x80, INITR_MINI160x80,
#endif
};

#ifdef WS_RP2040_ZERO
#define SPI_SS      5
#define SPI_MOSI    7
#define SPI_SCLK    6
#define SPI_MISO    4  // Not used
#define SPI_BUS     SPI

int tftCS[NUM_TFTS] = {
  SPI_SS, 14, 15, 26,
#if (NUM_TFTS==8)
  27, 28, 29, 3
#endif
};
#define TFT_RST     8
#define TFT_DC      4
#endif

#ifdef WS_ESP32C3_ZERO
#define SPI_SS      9
#define SPI_MOSI    7
#define SPI_SCLK    8
#define SPI_MISO    0  // Not used
SPIClass MySPI(FSPI);

int tftCS[NUM_TFTS] = {
  SPI_SS, 5, 4, 3,
#if (NUM_TFTS==8)
  2, 1, 0, 18
#endif
};
#define TFT_RST     6
#define TFT_DC     10
#endif

#ifdef WS_ESP32S3_ZERO
#define SPI_SS     10
#define SPI_MOSI    8
#define SPI_SCLK    9
#define SPI_MISO    1  // Not used
SPIClass MySPI(FSPI);

int tftCS[NUM_TFTS] = {
  SPI_SS, 6, 5, 4,
#if (NUM_TFTS==8)
  3, 2, 1, 12
#endif
};
#define TFT_RST     7
#define TFT_DC     11
#endif

Adafruit_ST7735 *tft[NUM_TFTS];

// Format is 16-bit 5-6-5 B-G-R
#define BBV 11
#define GBV  5
#define RBV  0
#define ST_COL(r,g,b) (((r&0x1F)<<RBV) | ((g&0x3F)<<GBV) | ((b&0x1F)<<BBV))
#define ST_BLACK   ST_COL(0,0,0)
#define ST_GREY    ST_COL(10,20,10)
#define ST_WHITE   ST_COL(31,63,31)
#define ST_BLUE    ST_COL(0,0,31)
#define ST_GREEN   ST_COL(0,63,0)
#define ST_RED     ST_COL(31,0,0)
#define ST_YELLOW  ST_COL(31,63,0)
#define ST_MAGENTA ST_COL(31,0,31)
#define ST_CYAN    ST_COL(0,63,31)

#define NUM_COLS 8
int colours[NUM_COLS] = {ST_GREY, ST_WHITE, ST_RED, ST_GREEN, ST_BLUE, ST_CYAN, ST_MAGENTA, ST_YELLOW};
char *colstr[NUM_COLS] = {"Grey  ", "White ", "Red   ", "Green ", "Blue  ", "Cyan  ", "Magnta", "Yellow"};
int colidx;

void setup() {
#ifdef WS_RP2040_ZERO
  SPI_BUS.setRX(SPI_MISO);
  SPI_BUS.setCS(SPI_SS);
  SPI_BUS.setSCK(SPI_SCLK);
  SPI_BUS.setTX(SPI_MOSI);
  SPI_BUS.begin(true);
#else
  MySPI.begin(SPI_SCLK, SPI_MISO, SPI_MOSI, SPI_SS);
  pinMode(SPI_SS, OUTPUT);
#endif

  int rstPin = TFT_RST;
  for (int i=0; i<NUM_TFTS; i++) {
#ifdef WS_RP2040_ZERO
    tft[i] = new Adafruit_ST7735(tftCS[i], TFT_DC, rstPin);
#else
    tft[i] = new Adafruit_ST7735(&MySPI, tftCS[i], TFT_DC, rstPin);
#endif
    rstPin = -1;  // Can only have one reset pin
    tft[i]->initR(tftTypes[i]);
    tft[i]->setRotation(3);
    tft[i]->fillScreen(ST_BLACK);
  }

  colidx = 0;
}

int colour;
void loop() {
  for (int i=0; i<NUM_TFTS; i++) {
    tft[i]->fillScreen(ST_BLACK);
    tft[i]->setCursor(10,10);
    tft[i]->setTextSize(4);
    tft[i]->setTextColor(colours[i]);
    tft[i]->print(i+1);
    delay(500);
  }
  delay(500);

  for (int i=0; i<NUM_TFTS; i++) {
    testfastlines(i, ST_RED, ST_BLUE);
    delay(500);
  }

  for (int i=0; i<NUM_TFTS; i++) {
    tft[i]->fillScreen(ST_BLACK);
  }

  for (int c=0; c<NUM_COLS; c++) {
    for (int i=0; i<NUM_TFTS; i++) {
      unsigned long time = millis();
      tft[i]->fillRect(10, 20, tft[i]->width(), 20, ST_BLACK);
      tft[i]->setTextColor(colours[c]);
      tft[i]->setCursor(10, 20);
      tft[i]->setTextSize(1);
      tft[i]->print(colstr[c]);
      tft[i]->print(" ");
      tft[i]->print(i);
      tft[i]->print(":");
      tft[i]->print(time, DEC);
    }
    delay(1000);
  }
}

void testfastlines(int t, uint16_t color1, uint16_t color2) {
  tft[t]->fillScreen(ST_BLACK);
  for (int16_t y=0; y < tft[t]->height(); y+=10) {
    tft[t]->drawFastHLine(0, y, tft[t]->width(), color1);
  }
  tft[t]->drawFastHLine(0, tft[t]->height()-1, tft[t]->width(), color1);
  for (int16_t x=0; x < tft[t]->width(); x+=10) {
    tft[t]->drawFastVLine(x, 0, tft[t]->height(), color2);
  }
  tft[t]->drawFastVLine(tft[t]->width()-1, 0, tft[t]->height(), color2);
}
 