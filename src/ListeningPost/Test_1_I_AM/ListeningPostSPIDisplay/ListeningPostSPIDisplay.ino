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
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>
#include "lcd2004.h"

// For ESP32-S3 Multi-display Board
#define SPI_SS     10
#define SPI_MOSI    8
#define SPI_SCLK    9
#define SPI_MISO    1 // Not used
SPIClass MySPI(FSPI);

// Max SPI speed is 80MHz for ESP32S3.
// But that might be unreliable.
#define SPI_SPEED  60000000

// How many pixels to scroll at a time
// 1 is smoothest, but slowest
// 3 is ok
// higher will be pretty jerky
#define DISPLAY_SCROLL 20

#define MAX_TFTS  8
#define NUM_TFTS  8
#define TFT_CS1  SPI_SS
#define TFT_CS2   6
#define TFT_CS3   5
#define TFT_CS4   4
#define TFT_CS5   3
#define TFT_CS6   2
#define TFT_CS7   1
#define TFT_CS8  12
#define TFT_RST   7
#define TFT_DC   11

int csPins[MAX_TFTS] = {TFT_CS1, TFT_CS2, TFT_CS3, TFT_CS4, TFT_CS5, TFT_CS6, TFT_CS7, TFT_CS8};

#define TFT_TYPE INITR_MINI160x80
//#define TFT_TYPE INITR_MINI160x80_PLUGIN   // Inverted display

Adafruit_ST7735 tft1 = Adafruit_ST7735(&MySPI, TFT_CS1, TFT_DC, TFT_RST);
Adafruit_ST7735 tft2 = Adafruit_ST7735(&MySPI, TFT_CS2, TFT_DC, -1);
Adafruit_ST7735 tft3 = Adafruit_ST7735(&MySPI, TFT_CS3, TFT_DC, -1);
Adafruit_ST7735 tft4 = Adafruit_ST7735(&MySPI, TFT_CS4, TFT_DC, -1);
Adafruit_ST7735 tft5 = Adafruit_ST7735(&MySPI, TFT_CS5, TFT_DC, -1);
Adafruit_ST7735 tft6 = Adafruit_ST7735(&MySPI, TFT_CS6, TFT_DC, -1);
Adafruit_ST7735 tft7 = Adafruit_ST7735(&MySPI, TFT_CS7, TFT_DC, -1);
Adafruit_ST7735 tft8 = Adafruit_ST7735(&MySPI, TFT_CS8, TFT_DC, -1);
Adafruit_ST7735 *tfts[MAX_TFTS] = {&tft1, &tft2, &tft3, &tft4, &tft5, &tft6, &tft7, &tft8};

// Format is 16-bit 5-6-5 B-G-R
// Allow 0..255 in component values, by only taking
// most significant bits (5 or 6) from each value.
//   bbbbbggggggrrrrr
#define ST_COL(r,g,b) (((r&0xF8)>>3)|((g&0xFC)<<3)|((b&0xF8)<<8))
#define ST_BLACK   ST_COL(0,0,0)
#define ST_GREY    ST_COL(64,64,64)
#define ST_WHITE   ST_COL(255,255,255)
#define ST_BLUE    ST_COL(0,0,255)
#define ST_GREEN   ST_COL(0,255,0)
#define ST_RED     ST_COL(255,0,0)
#define ST_YELLOW  ST_COL(255,255,0)
#define ST_MAGENTA ST_COL(255,0,255)
#define ST_CYAN    ST_COL(0,255,255)

// Create an instance of the text LCD on the tft
CLCD2004 lcd((Adafruit_ST77xx*)&tft1, ST_WHITE, ST_BLACK);
// Create a scrolling "lcd" display on the TFT
CLCD2004 lcd1((Adafruit_ST77xx*)&tft1, ST_WHITE, ST_BLACK, true);
CLCD2004 lcd2((Adafruit_ST77xx*)&tft2, ST_WHITE, ST_BLACK, true);
CLCD2004 lcd3((Adafruit_ST77xx*)&tft3, ST_WHITE, ST_BLACK, true);
CLCD2004 lcd4((Adafruit_ST77xx*)&tft4, ST_WHITE, ST_BLACK, true);
CLCD2004 lcd5((Adafruit_ST77xx*)&tft5, ST_WHITE, ST_BLACK, true);
CLCD2004 lcd6((Adafruit_ST77xx*)&tft6, ST_WHITE, ST_BLACK, true);
CLCD2004 lcd7((Adafruit_ST77xx*)&tft7, ST_WHITE, ST_BLACK, true);
CLCD2004 lcd8((Adafruit_ST77xx*)&tft8, ST_WHITE, ST_BLACK, true);
CLCD2004 *lcds[MAX_TFTS] = {&lcd1, &lcd2, &lcd3, &lcd4, &lcd5, &lcd6, &lcd7, &lcd8};

void setup() {
  MySPI.begin(SPI_SCLK, SPI_MISO, SPI_MOSI, SPI_SS);
  
  // Turn off any unused displays
  for (int i=NUM_TFTS; i<MAX_TFTS; i++) {
    pinMode(csPins[i], OUTPUT);
    digitalWrite(csPins[i], HIGH);
  }

  for (int i=0; i<NUM_TFTS; i++) {
    tfts[i]->initR(TFT_TYPE);
    tfts[i]->setSPISpeed(SPI_SPEED); // Must be done adfter initR() apparently
    tfts[i]->setRotation(3);
    tfts[i]->fillScreen(ST_BLACK);
  }
}

void loop() {
  iam();
  return;

  test1();
  test2();
  test3();
  test4();
  test5();
}

unsigned startcnt=0;
unsigned starts[MAX_TFTS];
unsigned scrolls[MAX_TFTS];
void iam() {
  if (startcnt == 0) {
    randomSeed(analogRead(14));
    for (int i=0; i<NUM_TFTS; i++) {
      lcds[i]->setColour(ST_CYAN, ST_BLACK);
      lcds[i]->printclr();
      lcds[i]->print("    I AM");
      starts[i] = random(200);
      scrolls[i] = 2+random(12);
    }
  }
  else {
    for (int i=0; i<NUM_TFTS; i++) {
      if (startcnt > starts[i]) {
        lcds[i]->scroll(scrolls[i], true);
      } else {
        // "null" scroll to keep timings consistent
        lcds[i]->scroll(0);
      }
    }
  }
  if (startcnt < 1000) {
    // no need to keep counting once all displays
    // are active and don't want to wrap around...
    startcnt++;
  }
}

void test1() {
  lcd1.setColour(ST_WHITE, ST_BLACK);
  char *test = "0123456789";
  lcd1.printclr();
  lcd1.print(test);
  lcd1.update();
  for (int i=0; i<=BIGCHAR_W*(3+2*strlen(test)); i+=1) {
    lcd1.scroll(1, true);
  }
  delay(500);
  lcd1.setColour(ST_CYAN, ST_BLACK);
  char *test2 = "    I AM GETTING CLOSER";
  lcd1.printclr();
  lcd1.print(test2);
  lcd1.update();
  for (int i=0; i<=BIGCHAR_W*(3+strlen(test2)); i+=2) {
    lcd1.scroll(2, true);
  }
  delay(500);
  lcd1.setColour(ST_BLUE, ST_BLACK);
  char *test3 = "    ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  lcd1.printclr();
  lcd1.print(test3);
  lcd1.update();
  for (int i=0; i<=BIGCHAR_W*(3+strlen(test3)); i+=3) {
    lcd1.scroll(3, true);
  }
  delay(500);
}

void test2() {
  lcd1.setColour(ST_CYAN, ST_BLACK);
  for (int i=0; i<7; i++) {
    char test[5] = {'A'+i*4, 'B'+i*4, 'C'+i*4, 'D'+i*4, 0};
    lcd1.printclr();
    lcd1.print(test);
    lcd1.update();
    delay(1000);
  }
}

void test3() {
  lcd.clear(ST_BLACK);
  for (int j=0; j<4; j++) {
    lcd.printglyph(0x0123, 0, j, ST_MAGENTA);
    delay(500);
    lcd.printglyph(0x4567, 4, j, ST_MAGENTA);
    delay(500);
    lcd.printglyph(0x8900, 8, j, ST_MAGENTA);
    delay(1000);
  }
  lcd.printline("012345678900", 0, 4, ST_MAGENTA);
  delay(2000);
}

void test4() {
  lcd.clear(ST_BLACK);
  // Test each shape with some bounding boxes
  int del = 1000;
  int col1 = ST_RED;
  int col2 = ST_YELLOW;
  tft1.drawRect(2*CHAR_X*PXL_X-1, 2*CHAR_Y*PXL_Y-1, CHAR_X*PXL_X+2, CHAR_Y*PXL_Y+2, ST_WHITE);
  lcd.clear(2,2,ST_BLACK);
  tft1.drawRect(2*CHAR_X*PXL_X, 2*CHAR_Y*PXL_Y, CHAR_X*PXL_X, CHAR_Y*PXL_Y, col1);
  delay(del);

  lcd.clear(2,2,ST_BLACK);
  tft1.drawRect(2*CHAR_X*PXL_X, 2*CHAR_Y*PXL_Y, CHAR_X*PXL_X, CHAR_Y*PXL_Y, col1);
  lcd.topRTri(2,2,col2);
  delay(del);
 
  lcd.clear(2,2,ST_BLACK);
  tft1.drawRect(2*CHAR_X*PXL_X, 2*CHAR_Y*PXL_Y, CHAR_X*PXL_X, CHAR_Y*PXL_Y, col1);
  lcd.topLTri(2,2,col2);
  delay(del);
 
  lcd.clear(2,2,ST_BLACK);
  tft1.drawRect(2*CHAR_X*PXL_X, 2*CHAR_Y*PXL_Y, CHAR_X*PXL_X, CHAR_Y*PXL_Y, col1);
  lcd.botLTri(2,2,col2);
  delay(del);
 
  lcd.clear(2,2,ST_BLACK);
  tft1.drawRect(2*CHAR_X*PXL_X, 2*CHAR_Y*PXL_Y, CHAR_X*PXL_X, CHAR_Y*PXL_Y, col1);
  lcd.botRTri(2,2,col2);
  delay(del);
 
  lcd.clear(2,2,ST_BLACK);
  tft1.drawRect(2*CHAR_X*PXL_X, 2*CHAR_Y*PXL_Y, CHAR_X*PXL_X, CHAR_Y*PXL_Y, col1);
  lcd.fullBlock(2,2,col2);
  delay(del);
 
  lcd.clear(2,2,ST_BLACK);
  tft1.drawRect(2*CHAR_X*PXL_X, 2*CHAR_Y*PXL_Y, CHAR_X*PXL_X, CHAR_Y*PXL_Y, col1);
  lcd.topBlock(2,2,col2);
  delay(del);
 
  lcd.clear(2,2,ST_BLACK);
  tft1.drawRect(2*CHAR_X*PXL_X, 2*CHAR_Y*PXL_Y, CHAR_X*PXL_X, CHAR_Y*PXL_Y, col1);
  lcd.botBlock(2,2,col2);
  delay(del);
 
  lcd.clear(2,2,ST_BLACK);
  tft1.drawRect(2*CHAR_X*PXL_X, 2*CHAR_Y*PXL_Y, CHAR_X*PXL_X, CHAR_Y*PXL_Y, col1);
  lcd.leftBlock(2,2,col2);
  delay(del);
 
  lcd.clear(2,2,ST_BLACK);
  tft1.drawRect(2*CHAR_X*PXL_X, 2*CHAR_Y*PXL_Y, CHAR_X*PXL_X, CHAR_Y*PXL_Y, col1);
  lcd.rightBlock(2,2,col2);
  delay(del);
 
  lcd.clear(2,2,col2);
  tft1.drawRect(2*CHAR_X*PXL_X, 2*CHAR_Y*PXL_Y, CHAR_X*PXL_X, CHAR_Y*PXL_Y, col1);
  lcd.clear(2,2,col2);
  delay(del);
}

void test5() {
  // Fill display with each shape
  for (int x=0; x<MAX_X; x++) {
    for (int y=0; y<MAX_Y; y++) {
      lcd.clear(x, y, ST_BLACK);
      lcd.topRTri(x, y, ST_YELLOW);
      delay(10);
    }
  }

  for (int x=0; x<MAX_X; x++) {
    for (int y=0; y<MAX_Y; y++) {
      lcd.clear(x, y, ST_BLACK);
      lcd.topLTri(x, y, ST_YELLOW);
      delay(10);
    }
  }

  for (int x=0; x<MAX_X; x++) {
    for (int y=0; y<MAX_Y; y++) {
      lcd.clear(x, y, ST_BLACK);
      lcd.botLTri(x, y, ST_YELLOW);
      delay(10);
    }
  }

  for (int x=0; x<MAX_X; x++) {
    for (int y=0; y<MAX_Y; y++) {
      lcd.clear(x, y, ST_BLACK);
      lcd.botRTri(x, y, ST_YELLOW);
      delay(10);
    }
  }

  for (int x=0; x<MAX_X; x++) {
    for (int y=0; y<MAX_Y; y++) {
      lcd.clear(x, y, ST_BLACK);
      lcd.fullBlock(x, y, ST_YELLOW);
      delay(10);
    }
  }

  for (int x=0; x<MAX_X; x++) {
    for (int y=0; y<MAX_Y; y++) {
      lcd.clear(x, y, ST_BLACK);
      lcd.topBlock(x, y, ST_YELLOW);
      delay(10);
    }
  }

  for (int x=0; x<MAX_X; x++) {
    for (int y=0; y<MAX_Y; y++) {
      lcd.clear(x, y, ST_BLACK);
      lcd.botBlock(x, y, ST_YELLOW);
      delay(10);
    }
  }

  for (int x=0; x<MAX_X; x++) {
    for (int y=0; y<MAX_Y; y++) {
      lcd.clear(x, y, ST_BLACK);
      lcd.leftBlock(x, y, ST_YELLOW);
      delay(10);
    }
  }

  for (int x=0; x<MAX_X; x++) {
    for (int y=0; y<MAX_Y; y++) {
      lcd.clear(x, y, ST_BLACK);
      lcd.rightBlock(x, y, ST_YELLOW);
      delay(10);
    }
  }

  for (int x=0; x<MAX_X; x++) {
    for (int y=0; y<MAX_Y; y++) {
      lcd.clear(x, y, ST_BLACK);
      delay(10);
    }
  }
}
