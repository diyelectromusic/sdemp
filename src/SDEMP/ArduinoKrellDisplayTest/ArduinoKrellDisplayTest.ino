/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Forbidden Planet Krell Display
//  https://diyelectromusic.com/2025/01/25/forbidden-planet-krell-display/
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
#include <Adafruit_NeoPixel.h>

#define ALG_PIN   A7  // Comment out if not using a pot
#define LED_PIN   6
#define LED_BLOCK 5
#define LED_RINGS 8
#define LED_COUNT (LED_BLOCK*LED_RINGS)
#define STRIP_BLOCK 7
#define STRIP_COUNT (LED_RINGS*STRIP_BLOCK)

int ledpattern[LED_BLOCK] = {1,0,5,4,3};
int leds[LED_COUNT];

Adafruit_NeoPixel strip(STRIP_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void clearStrip () {
  for(int i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
}

void setup() {
  for (int i=0; i<LED_RINGS; i++) {
    for (int j=0; j<LED_BLOCK; j++) {
      leds[LED_BLOCK*i + j] = ledpattern[j] + STRIP_BLOCK*i;
    }
  }
  strip.begin();
  strip.show();
  strip.setBrightness(50);
}

int lastled;
void loop() {
#ifdef ALG_PIN
  int algval = analogRead(ALG_PIN);
  int ledend = map(algval, 0,1020,0,LED_COUNT);
  if (lastled != ledend) {
    lastled = ledend;
    clearStrip();
    for(int i=0; i<lastled; i++) {
      strip.setPixelColor(leds[i], strip.Color(80, 35, 0));
    }
    strip.show();
  }
#else
  for(int i=0; i<LED_COUNT; i++) {
    clearStrip();
    strip.setPixelColor(leds[i], strip.Color(80, 35, 0));
    strip.show();
    delay(200);
  }
#endif
}
