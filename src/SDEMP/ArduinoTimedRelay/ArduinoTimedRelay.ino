/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Timed Relay
//  https://diyelectromusic.wordpress.com/2022/03/28/arduino-timed-relay/
//
      MIT License
      
      Copyright (c) 2022 diyelectromusic (Kevin)
      
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
   Arduino Relay: https://arduinogetstarted.com/tutorials/arduino-relay
   HT16K33 7 Segment Display: https://github.com/RobTillaart/HT16K33
*/
#include <HT16K33.h>

// I2C 7 Segment Display
//   C/CLK = A5/SCL
//   D/DAT = A4/SDA
HT16K33 seg(0x70);  // Pass in I2C address of the display

#define TIMER_START 7
#define RELAY_RESET 6
#define RELAY       5

// Set the sense for your relay.
#define ACTIVE   LOW
#define INACTIVE HIGH

#define MINS 1
#define SECS 11
#define STOP_TIME_MS ((MINS*60UL+SECS)*1000UL)
unsigned long starttime;
unsigned long stoptime;
bool resettimer;
bool timerstopped;
bool relayenabled;

void relayOn () {
  digitalWrite(RELAY, ACTIVE);
}

void relayOff () {
  digitalWrite(RELAY, INACTIVE);
}

void setup() {
  Serial.begin(9600);

  pinMode(TIMER_START, INPUT_PULLUP);
  pinMode(RELAY_RESET, INPUT_PULLUP);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, INACTIVE);  // Relay starts disactivated

  // Initialise the I2C display
  seg.begin();
  Wire.setClock(100000);
  seg.displayOn();
  seg.brightness(2);
  seg.setDigits(4);
  seg.displayClear();
  seg.displayInt(1234);
  delay(1000);
  seg.displayClear();
  resettimer = false;
  timerstopped = true;
  relayenabled = false;
}

void loop() {
  // Check the buttons
  if (digitalRead(TIMER_START) == LOW) {
    resettimer = true;
    timerstopped = false;
    Serial.println("Timer starting...");
    delay(500); // crude debouncing
  }

  if (digitalRead(RELAY_RESET) == LOW) {
    if (!relayenabled) {
      relayOn();
      Serial.println("Relay enabled");
      delay(500); // crude debouncing
      relayenabled = true;
    }
  }
  
  if (resettimer) {
    starttime = millis();
    resettimer = false;
    Serial.print("Start time=");
    Serial.println(starttime);
  }

  if (!timerstopped) {
    unsigned long time_ms = millis() - starttime;
    seg.displaySeconds(time_ms/1000, true, false);
    if (time_ms >= STOP_TIME_MS) {
      Serial.println("Stopped!");
      relayOff();
      timerstopped = true;
      relayenabled = false;
    }
  }
}
