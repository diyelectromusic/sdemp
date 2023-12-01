/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Frequency Game
//    https://diyelectromusic.wordpress.com/2023/12/01/arduino-guess-the-frequency-game/
//
      MIT License
      
      Copyright (c) 2023 diyelectromusic (Kevin)
      
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
    Potentiometer - https://www.arduino.cc/en/Tutorial/Potentiometer
    toneFollower  - https://www.arduino.cc/en/Tutorial/TonePitchFollower
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//#define TEST 1

#define POTS 3
int SPEAKER = 3;
int TRIGGER = 4;

#define READINGS 16
int s_readings[POTS][READINGS];
int s_idx[POTS];
int s_total[POTS];
int s_avge[POTS];

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int trigger;

void setup() {
#ifdef TEST
  Serial.begin(9600);
#endif
  randomSeed(analogRead(A7));

  pinMode(TRIGGER, INPUT_PULLUP);
  trigger = LOW;
  

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
#ifdef TEST
    Serial.println(F("SSD1306 allocation failed"));
#endif
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.setRotation(2);
  display.println("Guess!");
  display.display();
}

int smoothAnalogRead (int pot) {
  if ((pot < A0) || (pot >= A0+POTS)) {
    return 0;
  }
  int potIdx = pot - A0;
  int readIdx = s_idx[potIdx];
  s_total[potIdx] = s_total[potIdx] - s_readings[potIdx][readIdx];
  s_readings[potIdx][readIdx] = analogRead(pot);
  s_total[potIdx] = s_total[potIdx] + s_readings[potIdx][readIdx];
  s_avge[potIdx] = s_total[potIdx] / READINGS;
  
  readIdx++;
  if (readIdx >= READINGS) readIdx = 0;
  s_idx[potIdx] = readIdx;

  return s_avge[potIdx];
}

void printDec (unsigned integer, unsigned decimal) {
  if (integer < 1000) {
    display.print(" ");
  }
  if (integer < 100) {
    display.print(" ");
  }
  if (integer < 10) {
    display.print(" ");
  }
  display.print(integer);

  display.print(".");
  if (decimal < 10) display.print("0");
  display.print(decimal);
}

void loop() {
  unsigned pitch=0;
  for (int i=0; i<POTS; i++) {
    pitch += smoothAnalogRead(A0+i);
  }
#ifdef TEST
  Serial.println(pitch);
#endif
  // if the reading is zero, turn it off
  if (pitch == 0) {
    noTone (SPEAKER);
  } else {
    tone (SPEAKER, pitch);
  }

  int trig = digitalRead(TRIGGER);
  if ((trig == LOW) && (trigger == HIGH)) {
    // LOW -> HIGH = pressed
    display.clearDisplay();
    display.setCursor(0,0);

    // Add a random decimal between 0.00 and 0.99 to add to the fun...
    int rdec = random(100);
    printDec(pitch, rdec);
    display.print(" Hz\n");

    // Now calculate the different from 440.00 and show that too...
    // NB: multiply by 100 to allow simple calculations without FP!
    long pitch100 = ((long)pitch)*100 + (long)rdec;
    long diff = 0;
    char sign='-';
    if (pitch100 > 44000) {
      diff = pitch100 - 44000;
      sign='+';
    } else {
      diff = 44000 - pitch100;
    }
    int diffdec = diff - 100*(diff/100);
    printDec(diff/100, diffdec);
    display.print(" ");
    display.print(sign);
    display.print("\n");
    display.display();
  } else if ((trig == HIGH) && (trigger == LOW)) {
    // HIGH -> LOW = released
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Guess!");
    display.display();
  }
  trigger = trig;

/*
  Serial.print(s_idx[0]);
  Serial.print("::\t");
  for (int i=0; i<READINGS; i++) {
    Serial.print(s_readings[0][i]);
    Serial.print("\t");
  }
  Serial.println(s_avge[0]);
*/
}
