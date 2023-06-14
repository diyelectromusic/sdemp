/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino PCF8574/8575 MIDI Program and Control Messenger
//  https://diyelectromusic.wordpress.com/2023/06/04/arduino-pcf8574-midi-controller/
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
  Using principles from the following tutorials:
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    PCF8574 Datasheet and Libraries
*/
#include <MIDI.h>
#include <Wire.h>

//#define TEST 1

MIDI_CREATE_DEFAULT_INSTANCE();

// The following define the MIDI parameters to use.
//
#define MIDI_CHANNEL 1  // MIDI Channel number (1 to 16, or MIDI_CHANNEL_OMNI)

// Comment out to remove the LED indicator or change if you want your own LED
#define MIDI_LED     LED_BUILTIN

// PCF8574 I2C IO Expander
#define PCF857x_ADDR  0x20
#define PCF857x_CLOCK 100000
#define PCF857x_DEVICES 3  // Assumed to be on consecutive addresses
#define PCF857x_BITS  8  // 8 for PCF8574 or 16 for PCF8575

// MIDI Actions to take on each button
#define NUM_BTNS (PCF857x_DEVICES*PCF857x_BITS)
#define MB_CC 0
#define MB_PC 1
#define MB_N  2
int midiBtns[NUM_BTNS][2] = {
// T, Value T=0 (CC), T=1 (PC), T=2 (NoteOn/Off)
MB_PC, 0,
MB_PC, 1,
MB_PC, 2,
MB_PC, 3,
MB_PC, 4,
MB_PC, 5,
MB_PC, 6,
MB_PC, 7,
MB_N,  60,
MB_N,  61,
MB_N,  62,
MB_N,  63,
MB_N,  64,
MB_N,  65,
MB_N,  66,
MB_N,  67,
MB_N,  68,
MB_N,  69,
MB_N,  70,
MB_N,  71,
MB_N,  72,
MB_CC, 80,  // Generic On/Off
MB_CC, 81,  // Generic On/Off
MB_CC, 82,  // Generic On/Off
};

uint16_t midibtns[PCF857x_DEVICES];
uint16_t midibtns_last[PCF857x_DEVICES];

void midiBtnOn (int btn) {
  if (btn >= NUM_BTNS) return;

#ifdef TEST
  Serial.print("Btn On: ");
  Serial.print(midiBtns[btn][0]);
  Serial.print(",");
  Serial.print(midiBtns[btn][1]);
  Serial.print("\n");
  return;
#endif

  if (midiBtns[btn][0] == MB_CC) {
    // MIDI CC action required
    MIDI.sendControlChange(midiBtns[btn][1], 127, MIDI_CHANNEL);
  } else if (midiBtns[btn][0] == MB_PC) {
    // MIDI PC action required
    MIDI.sendProgramChange(midiBtns[btn][1], MIDI_CHANNEL);
  } else if (midiBtns[btn][0] == MB_N) {
    // MIDI Note action required
    MIDI.sendNoteOn(midiBtns[btn][1], 127, MIDI_CHANNEL);
  } else {
    // Unknown action - ignore!
  }
}

void midiBtnOff (int btn) {
  if (btn >= NUM_BTNS) return;

#ifdef TEST
  Serial.print("Btn Off: ");
  Serial.print(midiBtns[btn][0]);
  Serial.print(",");
  Serial.print(midiBtns[btn][1]);
  Serial.print("\n");
  return;
#endif

  if (midiBtns[btn][0] == MB_CC) {
    // MIDI CC action required
    MIDI.sendControlChange(midiBtns[btn][1], 0, MIDI_CHANNEL);
  } else if (midiBtns[btn][0] == MB_PC) {
    // MIDI PC action required
    // Nothing to do for "off" though...
  } else if (midiBtns[btn][0] == MB_N) {
    // MIDI Note action required
    MIDI.sendNoteOff(midiBtns[btn][1], 0, MIDI_CHANNEL);
  } else {
    // Unknown action - ignore!
  }
}
void ledInit() {
#ifdef MIDI_LED
  pinMode(MIDI_LED, OUTPUT);
  digitalWrite(MIDI_LED, LOW);
#endif
}
void ledOn() {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, HIGH);
#endif
}
void ledOff() {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, LOW);
#endif
}

void setup() {
  ledInit();

#ifdef TEST
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL);
#endif
  pcf857xSetup();
}

void loop() {
#ifdef TEST
  pcf857xTest();
#else
  MIDI.read();  // Allows MIDI THRU processing
#endif
  for (int d=0; d<PCF857x_DEVICES; d++) {
    midibtns[d] = pcf857xRead(d);

    for (int i=0; i<PCF857x_BITS; i++) {
      if ((midibtns[d] & (1<<i)) && !(midibtns_last[d] & (1<<i))) {
        // LOW->HIGH transition
        midiBtnOff(d*PCF857x_BITS+i);
      } else if (!(midibtns[d] & (1<<i)) && (midibtns_last[d] & (1<<i))) {
        // HIGH->LOW transition
        midiBtnOn(d*PCF857x_BITS+i);
      }
    }
    midibtns_last[d] = midibtns[d];
  }
}

void pcf857xSetup() {
  Wire.begin();
  Wire.setClock(PCF857x_CLOCK);

  byte error;

  for (int d=0; d<PCF857x_DEVICES; d++) {
    Wire.beginTransmission(PCF857x_ADDR+d);
    error = Wire.endTransmission();
    if (error == 0) {
#ifdef TEST
       Serial.print("I2C Device found at 0x");
       Serial.println(PCF857x_ADDR+d, HEX);
#endif
    }
  }
}

uint16_t pcf857xRead(int device) {
#if (PCF857x_BITS==8)
  if (Wire.requestFrom(PCF857x_ADDR+device, 1) != 0) {
    return (uint16_t)Wire.read();
  }
#elif (PCF857x_BITS==16)
  if (Wire.requestFrom(PCF857x_ADDR+device, 2) != 0) {
    uint16_t retval = (uint16_t)Wire.read();
    return retval + (((uint16_t)Wire.read())<<8);
  }
#else
#error PCF857x_BITS must be 8 or 16
#endif  
  return -1;
}

void pcf857xTest () {
#ifdef TEST
  for (int d=0; d<PCF857x_DEVICES; d++) {
    uint8_t val = midibtns[d];
    for (int i=0; i<PCF857x_BITS; i++) {
      if (val & (1<<(PCF857x_BITS-1-i))) {
        Serial.print("1");
      } else {
        Serial.print("0");
      }
    }
    Serial.print(" ");
  }
  Serial.print("\n");
#endif
}
