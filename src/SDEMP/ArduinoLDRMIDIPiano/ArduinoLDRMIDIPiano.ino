/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino LDR MIDI Piano
//  https://diyelectromusic.wordpress.com/2021/09/11/arduino-ldr-midi-piano/
//
      MIT License
      
      Copyright (c) 2020 diyelectromusic (Kevin)
      
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
    Arduino LDR          - https://www.tweaking4all.com/hardware/arduino/arduino-light-sensitive-resistor/
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
*/
#include <MIDI.h>

//#define TEST 1

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

int midiChannel = 1;  // Define which MIDI channel to transmit on (1 to 16).

#define MUXALG  A0
#define MUXPINS 3
int muxpins[MUXPINS] = {
  4,3,2
};

#define NUMKEYS 8
byte playing[NUMKEYS];
int  cal[NUMKEYS];

// Set the MIDI codes for the notes to be played by each key
byte notes[NUMKEYS] = {
  60, 62, 64, 65, 67, 69, 71, 72,
};

int muxAnalogRead (int mux) {
  for (int i=0; i<3; i++) {
    if (mux & (1<<i)) {
      digitalWrite(muxpins[i], HIGH);
    } else {
      digitalWrite(muxpins[i], LOW);
    }
  }
  return analogRead(MUXALG);
}

bool ldrDigitalRead (int ldr) {
  // This reads the LDR and compares it
  // to the stored threshold value to see
  // if it returns HIGH (covered) or LOW (not).
  int ldrval = muxAnalogRead(ldr);
  if (ldrval < cal[ldr]) {
    return HIGH;
  } else {
    return LOW;
  }
}

void setup() {
#ifdef TEST
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL_OFF);
#endif

  for (int i=0; i<MUXPINS ; i++) {
    pinMode (muxpins[i], OUTPUT);
    digitalWrite(muxpins[i], LOW);
  }

  // Perform several loops for calibration on startup
  //
  // Note cal[] is in int so can take up to +/-32767.
  // So add 16 reads (each of which can be up to 1023)
  // together, giving a maximum in cal[] of 16*1023 = 16388
  // then divide the final figure by 16 to get the
  // mean value over the 16 readings.
  //
  // What I actually need though is the threshold figure
  // to use to indicate a change from the LDR, so I actually
  // store the mean value less 20%. If the reading is above
  // this value, then it is considered "not covered".  If the
  // value is less than this, it is considered "covered".
  //
#ifdef TEST
  Serial.print("\n\nCalibrating...\n\n");
#endif
  for (int j=0; j<16; j++) {
    for (int i=0; i<NUMKEYS; i++) {
      int ldrval = muxAnalogRead(i);
      cal[i] += ldrval;
#ifdef TEST
      Serial.print(ldrval);
      Serial.print("\t");
#endif
    }
#ifdef TEST
    Serial.print("\n");
#endif
    delay(200);
  }
  for (int i=0; i<NUMKEYS; i++) {
    cal[i] = cal[i]/16;
    cal[i] = cal[i] - cal[i]/5;
  }  
#ifdef TEST
  Serial.println("\nCalculated Thresholds:\n");
  for (int i=0; i<NUMKEYS; i++) {
    Serial.print(cal[i]);
    Serial.print("\t");
  }
  Serial.print("\n\n");
#endif
}

void loop() {
  for (int k=0; k<NUMKEYS; k++) {
    // If covered and not already playing, sound the note
    bool ldrState = ldrDigitalRead(k);
    if (!playing[k] && ldrState) {
#ifdef TEST
      Serial.print(k);
      Serial.print("\tNote On:\t");
      Serial.println(notes[k]);
#else
      MIDI.sendNoteOn(notes[k], 127, midiChannel);
#endif
      playing[k] = true;
    } else if (playing[k] && !ldrState) {
#ifdef TEST
      Serial.print(k);
      Serial.print("\tNote Off:\t");
      Serial.println(notes[k]);
#else
      MIDI.sendNoteOff(notes[k], 0, midiChannel);
#endif
      playing[k] = false;
    } else {
      // Nothing to do - either still high or still low
    }
  }
}
