/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Universal Synthesizer Panel Test Application
//  https://diyelectromusic.wordpress.com/2021/05/31/universal-synthesizer-panel-introduction/
//
      MIT License
      
      Copyright (c) 2021 diyelectromusic (Kevin)
      
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
/*
  Using principles from the following Arduino tutorials:
    Analog Input          - https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogInput
    Digital Input Pullup  - https://www.arduino.cc/en/Tutorial/BuiltInExamples/InputPullupSerial
    Tone Melody           - https://www.arduino.cc/en/Tutorial/BuiltInExamples/toneMelody
    Adafruit SSD1306 Library - https://learn.adafruit.com/monochrome-oled-breakouts/arduino-library-and-examples
    Adafruit GFX Library     - https://learn.adafruit.com/adafruit-gfx-graphics-library
    Arduino MIDI Library  - https://github.com/FortySevenEffects/arduino_midi_library
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// These are the pinouts for the Arduino Nano version
//
int algPins[] = {A0, A1, A2, A3, A6, A7};
int digPins[] = {2, 3, 4, 5, 6, 7};
int audioPin  = 9;
int midiLed   = 8;

int vals[6];

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void drawRectangle (int n, int h, int onoff) {
  // scale value to screen height
  int rect_h = h>>4;
  int rect_x = 4 + n*20;

  // Blank first
  display.fillRect(rect_x, 0, 18, 63, SSD1306_BLACK);

  // Then draw
  if (onoff) {
     display.fillRect(rect_x, 63-rect_h, 18, rect_h, SSD1306_WHITE);
  } else {
     display.drawRect(rect_x, 63-rect_h, 18, rect_h, SSD1306_WHITE);    
  }
}

void setup() {
  Serial.begin(9600);

  // Initialise the digital inputs to PULLUP mode
  for (int i=0; i<6; i++) {
    pinMode (digPins[i], INPUT_PULLUP);
  }
  pinMode (midiLed, OUTPUT);
  digitalWrite(midiLed, LOW);

  // Initialise the display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("ERROR: Can't initliase the display");
  }
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();
}

void loop() {
  for (int i=0; i<6; i++) {
    int aval = analogRead(algPins[i]);
    vals[i] = aval;

    Serial.print(aval);
    Serial.print("\t");
  }
  for (int i=0; i<6; i++) {
    int dval = digitalRead(digPins[i]);
    drawRectangle(i, vals[i], dval);

    // Use first switch to turn tone/led on or off too
    if (i==0) {
      if (dval) {
        digitalWrite(midiLed, HIGH);
        tone(audioPin, 440);
      } else {
        digitalWrite(midiLed, LOW);
        noTone(audioPin);
      }
    }

    Serial.print(dval);
    Serial.print("\t");
  }
  Serial.print("\n");
  delay(100);

  display.display();
}
