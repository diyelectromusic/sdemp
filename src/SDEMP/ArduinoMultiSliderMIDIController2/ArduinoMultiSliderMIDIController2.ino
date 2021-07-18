/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Multi Slider MIDI Controller - Part 2
//  https://diyelectromusic.wordpress.com/2021/07/18/arduino-multi-slider-midi-controller-part-2/
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
    Potentiometer        - https://www.arduino.cc/en/Tutorial/Potentiometer
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
*/
#include <MIDI.h>

// Uncomment this to use the serial port as a test rather than real MIDI
//#define TEST 1

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_CHANNEL 1

// Uncomment this to use for the "slider board" with an Arduino Nano
//#define SLIDER_BOARD_NANO 1

// Uncomment to use a multiplexer
#define ALGMUX 1
// Need to set up the single analog pin to use for the MUX
#define MUXPIN A0
// And the digital lines to be used to control it.
// Four lines are required for 16 pots on the MUX.
// You can drop this down to three if only using 8 pots.
#define MUXLINES 3
int muxpins[MUXLINES]={4,5,6};
//int muxpins[MUXLINES]={4,5,6,7};
// Uncomment this if you want to control the MUX EN pin
// (optional, only required if you have several MUXs or
// are using the IO pins for other things too).
// This is active LOW.
//#define MUXEN    3

#define NUMPOTS 8
// In MUX mode this is the pot number for the MUX.
// In non-MUX mode these are Arduino analog pins.
#ifdef ALGMUX
int pots[NUMPOTS] = {0,1,2,3,4,5,6,7};
#else
// (actually, you could get away with using 0,1,2,3... here too)
int pots[NUMPOTS] = {A0,A1,A2,A3,A4,A5,A6,A7};
#endif
int cc[NUMPOTS] = {
  0x01,  // Modulation wheel
//  0x07,  // Channel volume
//  0x08,  // Balance
//  0x0A,  // Pan
  0x0B,  // Expression control
  0x0C,  // Effect control 1
  0x0D,  // Effect control 2
  0x10,  // General purpose control 1
  0x11,  // General purpose control 2
  0x12,  // General purpose control 3
  0x13,  // General purpose control 4
//  0x14,  // General purpose control 5
//  0x15,  // General purpose control 6
//  0x16,  // General purpose control 7
//  0x17,  // General purpose control 8
};
int last[NUMPOTS];

#ifdef ALGMUX
#define ALGREAD muxAnalogRead
#else
#ifdef SLIDER_BOARD_NANO
#define ALGREAD altAnalogRead
#else
#define ALGREAD analogRead
#endif
#endif

int altAnalogRead(int potpin) {
  // There are some inaccuracies in the readings here when using
  // my Nano slider board, I re-map the analog range onto 20-1000.
  //
  // Also note the additional use of constrain() to ensure the values
  // are within the range passed to the map() function.
  //
  int algval = analogRead(potpin);
  return map(constrain(algval, 20, 1000), 20,1000,0,1023);
}

int muxAnalogRead (int mux) {
  if (mux >= NUMPOTS) {
    return 0;
  }

#ifdef MUXEN
  digitalWrite(MUXEN, LOW);  // Enable MUX
#endif

  // Translate mux pot number into the digital signal
  // lines required to choose the pot.
  for (int i=0; i<MUXLINES; i++) {
    if (mux & (1<<i)) {
      digitalWrite(muxpins[i], HIGH);
    } else {
      digitalWrite(muxpins[i], LOW);
    }
  }

  // And then read the analog value from the MUX signal pin
  int algval = analogRead(MUXPIN);

#ifdef MUXEN
  digitalWrite(MUXEN, HIGH);  // Disable MUX
#endif
  
  return algval;
}

void muxInit () {
  for (int i=0; i<MUXLINES; i++) {
    pinMode(muxpins[i], OUTPUT);
    digitalWrite(muxpins[i], LOW);
  }

#ifdef MUXEN
  pinMode(MUXEN, OUTPUT);
  digitalWrite(MUXEN, HIGH); // start disabled
#endif
}

void setup() {
#ifdef TEST
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL_OMNI);
#endif

  // Preset the "last" values to something invalid to
  // force an update on the first time through the loop.
  for (int i=0; i<NUMPOTS; i++) {
    last[i] = -1;
  }

#ifdef SLIDER_BOARD_NANO
  // This is a 'hack' to use two of the Arduino's IO
  // pins as VCC and GND for the potentiometers.
  //
  // This is specific to my "slider" board, so you
  // probably don't need to worry about doing this.
  pinMode(6,OUTPUT);
  pinMode(7,OUTPUT);
  digitalWrite(6,LOW);  // GND
  digitalWrite(7,HIGH); // VCC
#endif

#ifdef ALGMUX
  muxInit();
#endif
}

int pot;
void loop() {
  // As this is a controller, we process any MIDI messages
  // we receive and allow the MIDI library to enact it's THRU
  // functionality to pass them on.
  MIDI.read();

  pot++;
  if (pot >= NUMPOTS) pot = 0;

  // We want to turn it into a number between 0 and 127
  // from the analog range of 0 to 1023. The simplest
  // way to do that is to divide it by two, three times,
  // which can be trivially done with a bit-shift of three places.
  //
  int algval = ALGREAD(pots[pot])>>3;
  if (algval != last[pot]) {
    // The value for this pot has changed so send a new MIDI message
#ifdef TEST
    Serial.print(pot);
    Serial.print(" CC 0x");
    if (cc[pot]<16) Serial.print("0");
    Serial.print(cc[pot],HEX);
    Serial.print(" -> ");
    Serial.println(algval);
#else
    MIDI.sendControlChange (cc[pot], algval, MIDI_CHANNEL);
#endif
    last[pot] = algval;
  }
}
