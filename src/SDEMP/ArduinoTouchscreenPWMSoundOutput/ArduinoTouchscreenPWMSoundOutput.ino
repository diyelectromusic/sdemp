/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Touchscreen PWM Sound Output
//  https://diyelectromusic.wordpress.com/2021/08/09/arduino-touchscreen-pwm-sound-output/
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
    Secrets of Arduino PWM - https://www.arduino.cc/en/Tutorial/SecretsOfArduinoPWM
    Direct Digital Synthesis - https://www.electronics-notes.com/articles/radio/frequency-synthesizer/dds-direct-digital-synthesis-synthesizer-what-is-basics.php
    "Audio Output and Sound Synthesis" from "Arduino for Musicians" Brent Edstrom
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    ILI9488 Touchscreen - https://emalliab.wordpress.com/2021/08/06/cheap-ili9488-tft-lcd-displays/
*/
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Arduino_GFX_Library.h>
#include <MIDI.h>
#include "pitches.h"

//#define TEST 1
#define TESTNOTE 69 // A4 - "Concert A" @ 440Hz

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// Set up the MIDI channel to listen on
#define MIDI_CHANNEL 1

// Set up the MIDI codes to respond to by listing the lowest note
#define MIDI_NOTE_START 23   // B0

// Set the notes to be played by each key
int notes[] = {
  NOTE_B0,
  NOTE_C1, NOTE_CS1, NOTE_D1, NOTE_DS1, NOTE_E1, NOTE_F1, NOTE_FS1, NOTE_G1, NOTE_GS1, NOTE_A1, NOTE_AS1, NOTE_B1,
  NOTE_C2, NOTE_CS2, NOTE_D2, NOTE_DS2, NOTE_E2, NOTE_F2, NOTE_FS2, NOTE_G2, NOTE_GS2, NOTE_A2, NOTE_AS2, NOTE_B2,
  NOTE_C3, NOTE_CS3, NOTE_D3, NOTE_DS3, NOTE_E3, NOTE_F3, NOTE_FS3, NOTE_G3, NOTE_GS3, NOTE_A3, NOTE_AS3, NOTE_B3,
  NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
  NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,
  NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,
  NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7,
  NOTE_C8, NOTE_CS8, NOTE_D8, NOTE_DS8
};
int numnotes;

#define FREQ_MIN 55   // A1
#define FREQ_MAX 1760 // A6

// Direct Digital Synthesis relies on an "accumulator" to index
// into a wavetable to read out samples for writing to the audio
// output circuitry.
//
// The accumulator is a 16 bit value, but we only need
// to refer to one of 256 values in the wavetable.  However
// the calculations involving the frequency often turn up decimals
// which are awkward to handle in code like this (and you lose accuracy).
//
// One way round this is to use "fixed point maths" where
// we take, say, a 16 bit value and use the top 8 bits
// for the whole number part and the low 8 bits for the
// decimal part.
//
// This is what I do with the accumulator.  This means
// that when it comes to actually using the accumulator
// to access the wavetable, it is 256 times bigger than
// we need it to be, so will always use (accumulator>>8)
// to calculate the index into the wavetable.
//
// This needs to be taken into account when calculating
// accumulator increments which will be using the formulas
// discussed above, but with several pre-set constant values.
//
// As we are using constant values involving 256 and a sample
// rate of 32768 it is very convenient to pre-calculate the
// values to be used in the frequency to increment calculation,
// especially making use of the 8.8 fixed point maths we've already mentioned.
//
// Recall:
//     Inc = freq * wavetable size / Sample rate
//
// We also want this 256 times larger to support the 8.8 FP maths.
// So for a sample rate of 32768 and wavetable size of 256
//     Inc = 256 * freq * 256 / 32768
//     Inc = freq * 65535/32768
//     Inc = freq * 2
//
// This makes calculating the (256 times too large) increment
// from the frequency pretty simple.
//
#define FREQ2INC(freq) (freq*2)
uint16_t accumulator;
uint16_t frequency;
uint8_t  wave;
int      playnote;

//NOTE: If you change the size of the wavetable, then the setWavetable
//      function will need re-writing.
#define WTSIZE 256
unsigned char wavetable[WTSIZE];

// XPT2046 Touchscreen Definitions
//
#define T_CS  8
// MOSI=11, MISO=12, SCK=13
#define T_IRQ 2  //  Set to 255 to disable interrupts and rely on software polling
XPT2046_Touchscreen ts(T_CS, T_IRQ);

// ILI9488 Display Definitions
//
#define ILI_CS  10
#define ILI_DC  7
#define ILI_RST 9
Arduino_DataBus *bus = new Arduino_HWSPI(ILI_DC, ILI_CS);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, ILI_RST, 0 /* rotation */, false /* IPS */);

// If the resolution or rotation changes, then the gfxBtnXX
// routines will need re-writing to cope.
unsigned w, h;
#define ROTATION 1  // Want display in landscape mode, USB port on right

void gfxSetup() {
  // Pre-initialise the "chip select" pins to "deselected" (HIGH)
  pinMode(ILI_CS, OUTPUT);
  pinMode(T_CS, OUTPUT);
  digitalWrite(ILI_CS, HIGH);
  digitalWrite(T_CS, HIGH);
  delay(10);
  
  ts.begin();
  ts.setRotation(ROTATION);

  gfx->begin();

  gfx->setRotation(ROTATION);
  w = gfx->width();
  h = gfx->height();
  gfx->fillScreen(BLACK);

#ifdef TEST
  Serial.print("ILI9844 Touchscreen ");
  Serial.print(w);
  Serial.print("x");
  Serial.println(h);
#endif
}

void gfxLoop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    int xc = xt2xdsp(p.x);
    int yc = yt2ydsp(p.y);
    setWaveTable(xc, yc);
  }
}

unsigned xt2xdsp(unsigned touch_x) {
  return (unsigned)((((unsigned long)w)*touch_x)/4096UL);
}

unsigned yt2ydsp(unsigned touch_y) {
  return (unsigned)((((unsigned long)h)*touch_y)/4096UL);
}

// Uses fixed definitions relating to a region
// in the centre of the display.
//
// As there is 320x480 display and want to represent
// a wavetable of 256 entries, with range 0-255, I'm using:
//    Y-scaling is 1:1
//    X-scaling is 1.5:1
//
// All values here in fixed point 12.4 format, i.e.
// all will be *16 when set and /16 on use (or >>4).
//
// WTn_SCALE = ratio of WT to Display.
//   1(*16) = No scaling, it is pixel for pixel mapped to the wavetable.
//   2(*16) = 1 WT entry is worth 2 pixels on the display.
//
// WTn_MIN and WTn_SIZE are in display units (*16).
//
// WTX_SIZE should be big enough to support WTSIZE entries.
//
#define fp2d(x) ((uint16_t)((x)>>4))  // div by 16
#define d2fp(x) ((uint16_t)((x)<<4))  // mul by 16
#define WTX_MIN    (48*16UL)
#define WTX_SCALE      24UL   // (1.5*16UL)
#define WTX_SIZE  (384*16UL)
#define WTY_MIN    (32*16UL)
#define WTY_SCALE   (1*16UL)
#define WTY_SIZE  (256*16UL)
#define BORDER           4
void displayWaveTable () {
  gfx->fillRect(fp2d(WTX_MIN)-BORDER, fp2d(WTY_MIN)-BORDER, fp2d(WTX_SIZE)+BORDER*2, fp2d(WTY_SIZE)+BORDER*2, RED);
  gfx->fillRect(fp2d(WTX_MIN), fp2d(WTY_MIN), fp2d(WTX_SIZE), fp2d(WTY_SIZE), BLACK);
  for (int i=0; i<WTSIZE; i++) {
    updateWaveTable (i);
  }
}

void updateWaveTable (int idx) {
  int drawval = 255-wavetable[idx];
  gfx->drawFastVLine(fp2d(WTX_MIN+idx*WTX_SCALE), fp2d(WTY_MIN), fp2d(WTY_SIZE), BLACK);
  gfx->drawPixel(fp2d(WTX_MIN+idx*WTX_SCALE), fp2d(WTY_MIN+drawval*WTY_SCALE), GREEN);
}

void setWaveTable (int xc, int yc) {
  // Convert coords into 12.4 fixed point format
  xc = d2fp(xc);
  yc = d2fp(yc);
  if ((xc < WTX_MIN) || (xc >= WTX_MIN+WTX_SIZE) || (yc < WTY_MIN) || (yc >= WTY_MIN+WTY_SIZE)) {
    return;
  }

  // Set the new value in the wavetable
  // NB: Both top and bottom of fraction all all in 12.4 arithmetic, so no
  //     extra converting back to non 12.4 values is required.  Both are *16
  //     too big already, so they cancel out.
  //
  wavetable[(xc-WTX_MIN)/WTX_SCALE] = 255-((yc-WTY_MIN)/WTY_SCALE);
  updateWaveTable((xc-WTX_MIN)/WTX_SCALE);
}

void initPwm() {
  // Initialise Timer 2 as follows:
  //    Output on pin 3 (OC2B).
  //    Run at CPU clock speed (16MHz).
  //    Use FastPWM mode (count up, then reset).
  //    Use OC2A as the TOP value.
  //    TOP value = (clock / (prescaler * 65536)) - 1 = 243
  //    PWM value is updated by writing to OCR2B
  //    Interrupt enabled for overflow: TIMER2_OVF_vect
  //
  // NB: Need 65536Hz PWM frequency to keep the counter
  //     value in the range of an 8-bit timer.
  //     However, we only do the sample every two ticks of the timer.
  //
  // So set up PWM for Timer 2, Output B:
  //   WGM2[2:0] = 111 = FastPWM; OCR2A as TOP; TOV1 set at BOTTOM
  //   COM2A[1:0] = 00 = OC2A disconnected
  //   COM2B[1:0] = 10  Clear OC2B on cmp match up; Set OC2B at cmp match down
  //   CS2[2:0] = 001 = No prescalar
  //   TOIE2 = Timer/Counter 2 Overflow Interrupt Enable
  //
  pinMode(3, OUTPUT);
  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS20);
  OCR2A = 243;

  TIMSK2 = _BV(TOIE2);
}

void setPwm (uint8_t pwmval) {
  OCR2B = pwmval;
}

int irqtoggle;
ISR(TIMER2_OVF_vect) {
  if (irqtoggle) {
    ddsOutput();
    irqtoggle = 0;
  } else {
    irqtoggle = 1;
  }
}

// These are the functions to be called on recieving certain MIDI events.
// Full documentation of how to do this is provided here:
// https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-Callbacks

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  if (velocity == 0) {
    // Handle this as a "note off" event
    handleNoteOff(channel, pitch, velocity);
    return;
  }

  if ((pitch < MIDI_NOTE_START) || (pitch >= MIDI_NOTE_START+numnotes)) {
    // The note is out of range for us so do nothing
    return;
  }

  playnote = pitch;
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  if (playnote == pitch) {
    // This is our note, so turn it off
    playnote = 0;
  }
}

void setup() {
#ifdef TEST
  Serial.begin(9600);
#endif

  for (int i=0; i<WTSIZE; i++) {
    wavetable[i] = i;
  }

  gfxSetup();

  initPwm();

  wave = 0;
  accumulator = 0;
  frequency = 440;

  displayWaveTable();

#ifndef TEST
  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // This listens to the specified MIDI channel
  MIDI.begin(MIDI_CHANNEL);
#endif

  numnotes = sizeof(notes)/sizeof(notes[0]);
}

void loop() {
#ifdef TEST
  playnote = TESTNOTE;
#else
  MIDI.read();
#endif

  gfxLoop();

  // Convert the playing note to a frequency
  if (playnote != 0) {
    frequency = notes[playnote-MIDI_NOTE_START];
  } else {
    // turn the sound generation off
    frequency = 0;
  }
}

// This is the code that is run on every "tick" of the timer.
void ddsOutput () {
  // Output the last calculated value first to reduce "jitter"
  // then go on to calculate the next value to be used.
  setPwm(wave);

  // Recall that the accumulator is as 16 bit value, but
  // representing an 8.8 fixed point maths value, so we
  // only need the top 8 bits here.
  wave = wavetable[accumulator>>8];
  accumulator += (FREQ2INC(frequency));
}
