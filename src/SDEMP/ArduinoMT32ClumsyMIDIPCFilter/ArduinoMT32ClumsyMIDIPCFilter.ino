/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MT-32 Clumsy MIDI Program Changer Filter
//  https://diyelectromusic.wordpress.com/2021/05/01/mt32-clumsy-midi-program-changer-and-filter/
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
    Arduino MIDI Library    - https://github.com/FortySevenEffects/arduino_midi_library
    Arduino Analog Input    - https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogInput/
    Adafruit SSD1306        - https://learn.adafruit.com/monochrome-oled-breakouts/arduino-library-and-examples
    Adafruit GFX Library    - https://learn.adafruit.com/adafruit-gfx-graphics-library
*/
#include <MIDI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "mt32voices.h"

// This is required to set up the MIDI library.
// The default MIDI setup uses the built-in serial port, bit this
// tells the MIDI library to handle complete MIDI messages at a time
// rather than rely on multiple calls to MIDI.read to get a complete message.
struct MySettings : public MIDI_NAMESPACE::DefaultSettings {
  static const bool Use1ByteParsing = false; // Allow MIDI.read to handle all received data in one go
  static const long BaudRate = 31250;        // Doesn't build without this...
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, MySettings);

// Comment out if don't want/need MIDI indicator
#define MIDI_LED LED_BUILTIN

// Potentiometer definitions.
// NB: Can't use A4 and A5 as these are the I2C interface used for the display.
//
#define POT_G A0   // Sound Group POT
#define POT_S A1   // Sound POT

// Digital Input Pins for the MIDI channel switch
int midisw[4] = {3, 4, 5, 6};

// Set the screen direction, relative to use of the DIY shield.
//   0 = landscape (pins at top)
//   1 = portrait (pins on the left)
//   2 = landscape (pins at bottom)
//   3 = portrait (pins on the right)
#define OLED_ROT 0

// Dimensions of the OLED screen
#define OLED_W  128
#define OLED_H   32
#define OLED_RST -1 // If your screen has a RESET pin set that here

// Set the address on the I2C bus for the display.
// SSD1306 based displays can be set to one of two
// addresses 0x3C or 0x3D depending on a jumper
// on the back of the boards.  By default mine is set
// to 0x3C.
//
// Note that I2C addresses are 7-bit only and the final
// bit indicates a read or a write going on.
//
// 0x3C = b0111100
// 0x3D = b0111101
//
// On the bus these are shifted up 1 bit to make
//    0x3C -> 0x78 (b01111000)
//    0x3D -> 0x7A (b01111010)
//
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display = Adafruit_SSD1306(OLED_W, OLED_H, &Wire, OLED_RST);

int pot_group;
int pot_sound;
int mt32voice;
int midich,lastmidich;
int pot1, pot2;
bool dispen;

void midiLedOn(void) {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, HIGH);
#endif
}

void midiLedOff() {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, LOW);
#endif
}

int calcMT32Voice () {
  // Now calculate the MT32 Voice index
  // NB: This will return a zero-indexed voice number
  int voice = 0;
  for (int i=0; i<pot_group; i++) {
    voice += mt32VinG[i];
  }
  voice += pot_sound;

  return voice;
}

void updateDisplay (int group, int sound) {
  if (!dispen) return;

  display.clearDisplay();
  display.setCursor(0,0);

  printGroup(group);
  printSound(mt32voice);
}

void printGroup (int group) {
  for (int i=0; i<MT32SIZE; i++) {
    char c = pgm_read_byte_near(&mt32groups[group][i]);
    if (c==0) break;
    display.print(c);
  }
  display.println();
}

void printSound (int sound) {
  for (int i=0; i<MT32SIZE; i++) {
    char c = pgm_read_byte_near(&mt32voices[sound][i]);
    if (c==0) break;
    display.print(c);
  }
  display.println();  
}

void mt32ProgramChange (byte voice) {
  midiLedOn();
  // Recall midich is 0 to 15, but MIDI requires 1 to 16
  MIDI.sendProgramChange(voice, midich+1);
  midiLedOff();
}

void setup() {
#ifdef MIDI_LED
  pinMode (MIDI_LED, OUTPUT);
  digitalWrite (MIDI_LED, LOW);
#endif

  // MIDI Switch inputs
  for (int i=0; i<4; i++) {
    pinMode(midisw[i], INPUT_PULLUP);
  }

  midiLedOn();
  dispen = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  midiLedOff();

  if (dispen) {
    display.setRotation(OLED_ROT);
    display.clearDisplay();
    display.display();

    // Initialise the display for text
    display.setTextColor(SSD1306_WHITE,SSD1306_BLACK);
    display.setTextWrap(true);
    display.setTextSize(2);
    updateDisplay(0,0);
  }

  // Listen to all MIDI channels in case the channel changes after power up
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();  // Disable automatic MIDI Thru function
}

int state;

void loop() {
  state++;

  // To keep the MIDI as responsive as can be, we don't read the IO
  // every time through the loop and when we do read it, we split the
  // reads over different scans.
  if (state == 10) {
    // Read the MIDI channel switches.  Recall these are configured
    // in PULLUP mode, so off = high.
    //
    // This will give is a channel number between 0 and 15
    // encoded as a binary number by combining the four switches.
    //
    // Note: The MT-32 itself only actually listens on channels
    //       1-8 (or 2-9) and 10.
    //
    midich = 0;
    for (int i=0; i<4; i++) {
      if (!digitalRead(midisw[i])) {
        midich = midich + (1<<i);
      }
    }
  }
  
  // Convert the potentiometers from 0 to 1023 down to
  // represent the number of MT-32 Groups and the max
  // number of MT-32 voices per group.
  //
  // Note that one of the pots is "wired backwards" so
  // need to reverse the value here.
  //
  if (state == 20) {
    pot1 = analogRead(POT_G) / (1024/MT32GROUPS);
  }
  
  if (state == 30) {
    pot2 = (1023-analogRead(POT_S)) / (1024/MT32MAXVING);
  }

  if (state > 30) {
    state = 0;
  }
  
  // Check that the sound is in range for the group
  // NB: Need 0-indexed group and sound
  // NB: Calc before testing pot_group/pot_sound otherwise
  //     when the pot readings are out of range they will
  //     continually be calculated as being different to
  //     pot_group/pot_sound and so continuously trigger updates.
  //
  if (pot1 >= MT32GROUPS) pot1 = MT32GROUPS-1;
  if (pot2 >= mt32VinG[pot1]) pot2 = mt32VinG[pot1]-1;
  
  if ((pot_group != pot1) || (pot_sound != pot2) || (midich != lastmidich)) {
    // Update the group and sound
    pot_group = pot1;
    pot_sound = pot2;
    mt32voice = calcMT32Voice();
    updateDisplay(pot_group, pot_sound);
    mt32ProgramChange(mt32voice);
  }
  lastmidich = midich;

  // If we have MIDI data then forward it on.
  // Based on the DualMerger.ino example from the MIDI library.
  //
  if (MIDI.read()) {
    // Extract the channel and type to build the command to pass on
    // Recall MIDI channels are in the range 1 to 16, but will be
    // encoded as 0 to 15.
    //
    // All commands in the range 0x80 to 0xE0 support a channel.
    // Any command 0xF0 or above do not (they are system messages).
    // 
    // Need to check it against the channel for the filter which
    // is stored in 0-15 format.
    byte ch = MIDI.getChannel();
    if (ch == midich+1) {
      // Now we've filtered on channel, filter on command
      byte cmd = MIDI.getType();
      switch (cmd) {
        case midi::NoteOn:
          midiLedOn();
          MIDI.send(MIDI.getType(), MIDI.getData1(), MIDI.getData2(), MIDI.getChannel());
          break;
        case midi::NoteOff:
          midiLedOff();
          MIDI.send(MIDI.getType(), MIDI.getData1(), MIDI.getData2(), MIDI.getChannel());
          break;
        // Add in any additional messages to pass through here
        // Alternatively, put the "passthrough" in a default statement here
        // and add specific cases for any messages to filter out.
      }
    }
  }

  // Finally update the display every other scan
  if (state & 0x01) {
    if (dispen) display.display();
  }
}
