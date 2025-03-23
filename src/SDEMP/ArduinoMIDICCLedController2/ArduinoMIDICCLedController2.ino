/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI LED CC Controller 2
//  https://diyelectromusic.com/2025/03/18/duppa-i2c-midi-controller-part-3/
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
/*
  Using principles from the following tutorials:
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Duppa Library - https://github.com/Fattoresaimon/ArduinoDuPPaLib
*/
#include <MIDI.h>
#include <Wire.h>
#include "LEDRingSmall.h"

//#define TEST

// Uncomment the following for one LED per CC.
// This requires 5+ laps around the LED ring to cover the 128 values
#define LED_PER_CC

// When the above is enabled the following scales
// the number of LEDs to a multiple of the ring size.
#define LED_SCALE_TO_RING

// Choose one of these for thr input method
//#define CC_ENCODER
#define CC_POTENTIOMETER
//#define CC_I2CENCODER
//#define CC_ENDLESSPOT

// Uncomment the following to make the CC value
// wrap around from 127 back to 0.
#define CC_WRAP

// Interrupt pin for the I2C Duppa encoder
#define PIN_ENC_IRQ A3

// Analog pin for the potentiometer
#define PIN_ALG A0

// Pins for the regular encoder
#define PIN_ENC_A 11
#define PIN_ENC_B 10

// Pins for the endless potentiometer
#define PIN_EALG_1 A1
#define PIN_EALG_2 A2

// Everything acts on and is driven by these values
byte midiCC;
byte lastMidiCC;

#ifdef CC_I2CENCODER
#include <i2cEncoderMiniLib.h>
#endif
#ifdef CC_ENCODER
#include <RotaryEncoder.h>
#endif
#ifdef CC_ENDLESSPOT
#include "EndlessPotentiometer.h"
#endif

#ifdef TEST
#define dbegin(...) Serial.begin(__VA_ARGS__)
#define dprint(...) Serial.print(__VA_ARGS__)
#define dprintln(...) Serial.println(__VA_ARGS__)
#else
#define dbegin(...)
#define dprint(...)
#define dprintln(...)
#endif

//--------------------------------------------------
// MIDI
//--------------------------------------------------
MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_CHANNEL 1  // MIDI Channel number (1 to 16, or MIDI_CHANNEL_OMNI)
//#define MIDI_THRU    1  // Include software MIDI THRU
#define MIDI_CC      1  // 1=Modulation Wheel
#define MIDI_LED     LED_BUILTIN // Optional MIDI IN activity
#define MIDI_LED_CNT 300

int midiLedCnt;
void midiLedOn () {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, HIGH);
#endif
}
void midiLedOff () {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, LOW);
#endif
}
void midiLedSetup () {
#ifdef MIDI_LED
  pinMode(MIDI_LED, OUTPUT);
  midiLedOff();
#endif
  midiLedCnt=0;
}
void midiLedLoop() {
  if (midiLedCnt > 0) {
    midiLedCnt--;
  } else {
    midiLedOff();
  }
}
void midiLedReset() {
   midiLedCnt = MIDI_LED_CNT;
}

void midiControlChange(byte channel, byte control, byte value) { 
  if ((channel == MIDI_CHANNEL) && (control == MIDI_CC)) {
    midiLedOn();
    midiLedReset();
    midiCC = value;
  }
}

#ifndef TEST
void midiSetup() {
  midiLedSetup();
  MIDI.begin(MIDI_CHANNEL);

#ifdef MIDI_THRU
  MIDI.turnThruOn();
#else
  MIDI.turnThruOff();
#endif

  MIDI.setHandleControlChange(midiControlChange);
}

void midiLoop() {
  // Process MIDI read first in case it changes the value of midiCC
  // due to receiving a MIDI message.
  MIDI.read();
  midiLedLoop();

  if (lastMidiCC != midiCC) {
    // This will result in sending a MIDI CC as soon as midiCC changes
    // which could come from the encoder or from reciving a MIDI CC message.
    //
    // If MIDI THRU is enabled, then the received CC message will also
    // have already been sent via the THRU mechanism by this point too
    // so this is a duplicate, but I'm not worrying about that...
    MIDI.sendControlChange(MIDI_CC, midiCC, MIDI_CHANNEL);
  }
}
#else
void midiSetup() {}
void midiLoop() {}
#endif

void midiCCIncrement(int inc) {
#ifdef CC_WRAP
  midiCC += inc;
  midiCC = midiCC % 128;
#else
  if (midiCC <= 127-inc) {
    midiCC += inc;
  } else {
    midiCC = 127;
  }
#endif
}

void midiCCDecrement(int dec) {
#ifdef CC_WRAP
  midiCC -= dec;
  midiCC = midiCC % 128;
#else
  if (midiCC >= dec) {
     midiCC -= dec;
  } else {
    midiCC = 0;
  }
#endif
}

//--------------------------------------------------
//   Duppa LED Ring
//--------------------------------------------------
LEDRingSmall LEDRingSmall(ISSI3746_SJ1 | ISSI3746_SJ5);

#define NUM_LEDS 24
uint32_t ledMap[NUM_LEDS];
int nextLed;

int ledIndex (int led) {
  // Maps the required LED index onto the physical
  // LED arrangements.
  // LEDs are arranged anti-clockwise, with L1
  // at "12 o'clock", which we want to keep.
  if (led == 0) {
    return 0;
  }
  // Otherwise 1 = L24 (index 23)
  //           2 = L23 (index 22)
  //           ...
  //          23 = L2 (index 1)
  return NUM_LEDS - led;
}

void ledSetup() {
  LEDRingSmall.LEDRingSmall_Reset();
  delay(20);

  LEDRingSmall.LEDRingSmall_Configuration(0x01); //Normal operation
  LEDRingSmall.LEDRingSmall_PWMFrequencyEnable(1);
  LEDRingSmall.LEDRingSmall_SpreadSpectrum(0b0010110);
  LEDRingSmall.LEDRingSmall_GlobalCurrent(0x10);
  LEDRingSmall.LEDRingSmall_SetScaling(0xFF);
  LEDRingSmall.LEDRingSmall_PWM_MODE();

  for (int i=0; i<NUM_LEDS; i++) {
    ledMap[i] = 0;
  }
  nextLed = 0;
}

#ifdef LED_PER_CC
#define LED_CNT 5000
#else
#define LED_CNT 6000
#endif

int ledCnt;
void ledLoop() {
  ledCnt++;
  if (ledCnt > LED_CNT) {
    ledCnt=0;
    for (int i=0; i<NUM_LEDS; i++) {
      if (i != nextLed && ledMap[i] > 0) {
        uint32_t newcol = ((ledMap[i] & 0xFE0000) >> 1) + ((ledMap[i] & 0x00FE00) >> 1) + ((ledMap[i] & 0x0000FE) >> 1);
        LEDRingSmall.LEDRingSmall_Set_RGB(i, newcol);
        ledMap[i] = newcol;
      }
    }
  }
  if (lastMidiCC != midiCC) {
    // Need to update the current LED...
    int midiCC_scaled = midiCC;
#ifdef LED_PER_CC
#ifdef LED_SCALE_TO_RING
    // Map to a multiple of NUM_LEDS
    int scalemax = 128 - 128 % NUM_LEDS;
    midiCC_scaled = (midiCC * scalemax / 128);
#endif
    // Show a 1:1 mapping between LEDs and CC values
    nextLed = ledIndex(midiCC_scaled % NUM_LEDS);
    uint32_t col=0;
    if (midiCC_scaled < NUM_LEDS) {
      col = 0x00003F;
    } else if (midiCC_scaled < NUM_LEDS*2) {
      col = 0x003F3F;
    } else if (midiCC_scaled < NUM_LEDS*3) {
      col = 0x003F00;
    } else if (midiCC_scaled < NUM_LEDS*4) {
      col = 0x3F3F00;
    } else if (midiCC_scaled < NUM_LEDS*5) {
      col = 0x3F0000;
    } else {
      col = 0x3F003F;
    }
    LEDRingSmall.LEDRingSmall_Set_RGB(nextLed, col);
    ledMap[nextLed] = col;
#else
    // Scale the LEDs to fit the CC range
    nextLed = ledIndex(midiCC * NUM_LEDS / 128);
    LEDRingSmall.LEDRingSmall_Set_RGB(nextLed, 0x00003F);
    ledMap[nextLed] = 0x00003F;
#endif
  }
}

//--------------------------------------------------
// Basic  Rotary Encoder
//--------------------------------------------------
#ifdef CC_ENCODER

RotaryEncoder encoder(PIN_ENC_A, PIN_ENC_B, RotaryEncoder::LatchMode::FOUR3);
int re_pos;

void encSetup() {
  re_pos = 0;
}

void encLoop() {
  encoder.tick();
  int new_pos = encoder.getPosition();
  if (new_pos != re_pos) {
    int re_dir = (int)encoder.getDirection();
    if (re_dir < 0) {
      midiCCDecrement(1);
    } else if (re_dir > 0) {
      midiCCIncrement(1);
    } else {
      // if re_dir == 0; do nothing
    }
    re_pos = new_pos;
  }
}
#else
void encSetup() {}
void encLoop() {}
#endif

//--------------------------------------------------
// Analog Averaging
//--------------------------------------------------
#define AVGEREADINGS 32

class AvgePot {
public:
  AvgePot(int pot) {
    potpin = pot;
    for (int i=0; i<AVGEREADINGS; i++) {
      avgepotvals[i] = 0;
    }
    avgepotidx = 0;
    avgepottotal = 0;
  }

  int getPin () {
    return potpin;
  }

  // Implement an averaging routine.
  // NB: All values are x10 to allow for 1
  //     decimal place of accuracy to allow
  //     "round to nearest" rather the default
  //     "round down" calculations, which gives
  //     a much more stable result.
  int avgeAnalogRead () {
    int reading = 10*analogRead(potpin);
    avgepottotal = avgepottotal - avgepotvals[avgepotidx];
    avgepotvals[avgepotidx] = reading;
    avgepottotal = avgepottotal + reading;
    avgepotidx++;
    if (avgepotidx >= AVGEREADINGS) avgepotidx = 0;
    return (((avgepottotal / AVGEREADINGS) + 5) / 10);
  }

  int avgeAnalogRead (int pot) {
    if (pot == potpin) {
      return avgeAnalogRead();
    }
    return 0;
  }

private:
  int potpin;
  int avgepotvals[AVGEREADINGS];
  int avgepotidx;
  long avgepottotal;
};

int testAnalogRead(int pin) {
  return 35;
}

//--------------------------------------------------
// Plain Potentiometer
//--------------------------------------------------
#ifdef CC_POTENTIOMETER

AvgePot algpot(PIN_ALG);
int lastpot;
void potSetup() {
  lastpot = -1;
}

int potcnt;
void potLoop() {
  potcnt++;
  if (potcnt > 100) {
    potcnt = 0;
    // Convert 10-bit reading down to 7-bits
    int newpot = algpot.avgeAnalogRead(PIN_ALG)>>3;
    // NB: Can't test against midiCC directly as
    //     it might get changed over MIDI and we
    //     don't want it to be immediatedly overwritten again.
    if (newpot != lastpot) {
      midiCC = newpot;
      lastpot = newpot;
    }
  }
}
#else
void potSetup() {}
void potLoop() {}
#endif

//--------------------------------------------------
// Duppa I2C Rotary Encoder
//--------------------------------------------------
#ifdef CC_I2CENCODER

i2cEncoderMiniLib Encoder(0x20);

void i2cEncIncrement(i2cEncoderMiniLib* obj) {
  midiCCIncrement(1);
}

void i2cEncDecrement(i2cEncoderMiniLib* obj) {
  midiCCDecrement(1);
}

void i2cEncSetup() {
  pinMode(PIN_ENC_IRQ, INPUT);

  Encoder.reset();
  Encoder.begin(i2cEncoderMiniLib::WRAP_DISABLE
                | i2cEncoderMiniLib::DIRE_LEFT
                | i2cEncoderMiniLib::IPUP_ENABLE
                | i2cEncoderMiniLib::RMOD_X1 );

  Encoder.writeCounter((int32_t) 0); /* Reset the counter value */
  Encoder.writeStep((int32_t) 1); /* Set the step to 1*/

  Encoder.onIncrement = i2cEncIncrement;
  Encoder.onDecrement = i2cEncDecrement;

  Encoder.autoconfigInterrupt();
}

void i2cEncLoop() {
  if (digitalRead(PIN_ENC_IRQ) == LOW) {
    Encoder.updateStatus();
  }
}
#else
void i2cEncSetup() {}
void i2cEncLoop() {}
#endif

//--------------------------------------------------
// Endless potentiometer
//--------------------------------------------------
#ifdef CC_ENDLESSPOT

AvgePot epot1(PIN_EALG_1);
AvgePot epot2(PIN_EALG_2);
EndlessPotentiometer endpot;
int lastendpot;
void endpotSetup() {
  lastendpot = 0;
  // For MIDI CC values
  endpot.maxValue = 127;
  endpot.minValue = 0;
  endpot.sensitivity = 1;
  endpot.threshold = 0;
}

int endpotcnt;
void endpotLoop() {
  endpotcnt++;
  if (endpotcnt > 100) {
    endpotcnt = 0;
    endpot.updateValues(epot1.avgeAnalogRead(PIN_EALG_1), epot2.avgeAnalogRead(PIN_EALG_2));
    if (endpot.isMoving) {
      if (endpot.direction == endpot.CLOCKWISE) {
        midiCCIncrement(endpot.valueChanged);
      } else if (endpot.direction == endpot.COUNTER_CLOCKWISE) {
        midiCCDecrement(endpot.valueChanged);
      }
    }
  }
}
#else
void endpotSetup() {}
void endpotLoop() {}
#endif

//--------------------------------------------------
// Main Arduino functions
//--------------------------------------------------

void setup() {
  dbegin(9600);

  Wire.begin();
  Wire.setClock(400000);
  midiCC = 0;
  lastMidiCC = -1;

  encSetup();
  potSetup();
  i2cEncSetup();
  endpotSetup();
  midiSetup();
  ledSetup();
}

void loop() {
  // Encoder/pot has to be scanned first to see if midiCC changes.
  // Then MIDI is scanned which might also update midiCC before acting upon it.
  // Finally the LEDs are scanned last to reflect any changes in midiCC.
  encLoop();
  potLoop();
  i2cEncLoop();
  endpotLoop();
  midiLoop();
  ledLoop();

  lastMidiCC = midiCC;
}
