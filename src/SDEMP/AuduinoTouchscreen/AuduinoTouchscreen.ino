/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Auduino Touchscreen
//  https://diyelectromusic.wordpress.com/2021/08/13/arduino-touchscreen-auduino/
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
// Auduino, the Lo-Fi granular synthesiser
//
// by Peter Knight, Tinker.it http://tinker.it
//
// Help:      http://code.google.com/p/tinkerit/wiki/Auduino
// More help: http://groups.google.com/group/auduino
//
// Analog in 0: Grain 1 pitch
// Analog in 1: Grain 2 decay
// Analog in 2: Grain 1 decay
// Analog in 3: Grain 2 pitch
// Analog in 4: Grain repetition frequency
//
// Digital 3: Audio out (Digital 11 on ATmega8)
//
// Changelog:
// 19 Nov 2008: Added support for ATmega8 boards
// 21 Mar 2009: Added support for ATmega328 boards
// 7 Apr 2009: Fixed interrupt vector for ATmega328 boards
// 8 Apr 2009: Added support for ATmega1280 boards (Arduino Mega)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Arduino_GFX_Library.h>

//#define TEST 1  // Prints out touchscreen values for calibration

uint16_t syncPhaseAcc;
uint16_t syncPhaseInc;
uint16_t grainPhaseAcc;
uint16_t grainPhaseInc;
uint16_t grainAmp;
uint8_t grainDecay;
uint16_t grain2PhaseAcc;
uint16_t grain2PhaseInc;
uint16_t grain2Amp;
uint8_t grain2Decay;

// Map Analogue channels
#define SYNC_CONTROL         (0)
#define GRAIN_FREQ_CONTROL   (1)
#define GRAIN_DECAY_CONTROL  (2)
#define GRAIN2_FREQ_CONTROL  (3)
#define GRAIN2_DECAY_CONTROL (4)


// Changing these will also requires rewriting audioOn()

#if defined(__AVR_ATmega8__)
//
// On old ATmega8 boards.
//    Output is on pin 11
//
#define LED_PIN       13
#define LED_PORT      PORTB
#define LED_BIT       5
#define PWM_PIN       11
#define PWM_VALUE     OCR2
#define PWM_INTERRUPT TIMER2_OVF_vect
#elif defined(__AVR_ATmega1280__)
//
// On the Arduino Mega
//    Output is on pin 3
//
#define LED_PIN       13
#define LED_PORT      PORTB
#define LED_BIT       7
#define PWM_PIN       3
#define PWM_VALUE     OCR3C
#define PWM_INTERRUPT TIMER3_OVF_vect
#else
//
// For modern ATmega168 and ATmega328 boards
//    Output is on pin 3
//
#define PWM_PIN       3
#define PWM_VALUE     OCR2B
#define LED_PIN       13
#define LED_PORT      PORTB
#define LED_BIT       5
#define PWM_INTERRUPT TIMER2_OVF_vect
#endif

// Smooth logarithmic mapping
//
uint16_t antilogTable[] = {
  64830,64132,63441,62757,62081,61413,60751,60097,59449,58809,58176,57549,56929,56316,55709,55109,
  54515,53928,53347,52773,52204,51642,51085,50535,49991,49452,48920,48393,47871,47356,46846,46341,
  45842,45348,44859,44376,43898,43425,42958,42495,42037,41584,41136,40693,40255,39821,39392,38968,
  38548,38133,37722,37316,36914,36516,36123,35734,35349,34968,34591,34219,33850,33486,33125,32768
};
uint16_t mapPhaseInc(uint16_t input) {
  return (antilogTable[input & 0x3f]) >> (input >> 6);
}

// Stepped chromatic mapping
//
uint16_t midiTable[] = {
  17,18,19,20,22,23,24,26,27,29,31,32,34,36,38,41,43,46,48,51,54,58,61,65,69,73,
  77,82,86,92,97,103,109,115,122,129,137,145,154,163,173,183,194,206,218,231,
  244,259,274,291,308,326,346,366,388,411,435,461,489,518,549,581,616,652,691,
  732,776,822,871,923,978,1036,1097,1163,1232,1305,1383,1465,1552,1644,1742,
  1845,1955,2071,2195,2325,2463,2610,2765,2930,3104,3288,3484,3691,3910,4143,
  4389,4650,4927,5220,5530,5859,6207,6577,6968,7382,7821,8286,8779,9301,9854,
  10440,11060,11718,12415,13153,13935,14764,15642,16572,17557,18601,19708,20879,
  22121,23436,24830,26306
};
uint16_t mapMidi(uint16_t input) {
  return (midiTable[(1023-input) >> 3]);
}

// Stepped Pentatonic mapping
//
uint16_t pentatonicTable[54] = {
  0,19,22,26,29,32,38,43,51,58,65,77,86,103,115,129,154,173,206,231,259,308,346,
  411,461,518,616,691,822,923,1036,1232,1383,1644,1845,2071,2463,2765,3288,
  3691,4143,4927,5530,6577,7382,8286,9854,11060,13153,14764,16572,19708,22121,26306
};

uint16_t mapPentatonic(uint16_t input) {
  uint8_t value = (1023-input) / (1024/53);
  return (pentatonicTable[value]);
}


void audioOn() {
  // Configure for FastPWM mode:
  //    FastPWM
  //    No prescalar
  //    Clear OC2B on match; Set on BOTTOM
  //    Match at TOP
  // WGM2[2:0] = 011
  // COM2B[1:0] = 10
  // CS2[2:0] = 001
  // TOIE2 = 1
  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  TIMSK2 = _BV(TOIE2);
  return;
  
#if defined(__AVR_ATmega8__)
  // ATmega8 has different registers
  TCCR2 = _BV(WGM20) | _BV(COM21) | _BV(CS20);
  TIMSK = _BV(TOIE2);
#elif defined(__AVR_ATmega1280__)
  TCCR3A = _BV(COM3C1) | _BV(WGM30);
  TCCR3B = _BV(CS30);
  TIMSK3 = _BV(TOIE3);
#else
  // Set up PWM to 31.25kHz, phase accurate
  TCCR2A = _BV(COM2B1) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  TIMSK2 = _BV(TOIE2);
#endif
}


void setup() {
#ifdef TEST
  Serial.begin(9600);
#endif
  gfxSetup();
  pinMode(PWM_PIN,OUTPUT);
  audioOn();
  pinMode(LED_PIN,OUTPUT);
}

void loop() {
  // The loop is pretty simple - it just updates the parameters for the oscillators.
  //
  // Avoid using any functions that make extensive use of interrupts, or turn interrupts off.
  // They will cause clicks and poops in the audio.

  gfxLoop();

  // Smooth frequency mapping
  syncPhaseInc = mapPhaseInc(gfxAnalogRead(SYNC_CONTROL)) / 4;
  
  // Stepped mapping to MIDI notes: C, Db, D, Eb, E, F...
  //syncPhaseInc = mapMidi(gfxAnalogRead(SYNC_CONTROL));
  
  // Stepped pentatonic mapping: D, E, G, A, B
  //syncPhaseInc = mapPentatonic(gfxAnalogRead(SYNC_CONTROL));

  grainPhaseInc  = mapPhaseInc(gfxAnalogRead(GRAIN_FREQ_CONTROL)) / 2;
  grainDecay     = gfxAnalogRead(GRAIN_DECAY_CONTROL) / 8;
  grain2PhaseInc = mapPhaseInc(gfxAnalogRead(GRAIN2_FREQ_CONTROL)) / 2;
  grain2Decay    = gfxAnalogRead(GRAIN2_DECAY_CONTROL) / 4;
#ifdef TEST
  Serial.print(syncPhaseInc);
  Serial.print("\t");
  Serial.print(grainPhaseInc);
  Serial.print("\t");
  Serial.print(grainDecay);
  Serial.print("\t");
  Serial.print(grain2PhaseInc);
  Serial.print("\t");
  Serial.print(grain2Decay);
  Serial.print("\n");
#endif
}

int irqtoggle;
SIGNAL(PWM_INTERRUPT)
{
  uint8_t value;
  uint16_t output;

  if (irqtoggle) {
    irqtoggle = 0;
    return;
  } else {
    irqtoggle = 1;
  }

  syncPhaseAcc += syncPhaseInc;
  if (syncPhaseAcc < syncPhaseInc) {
    // Time to start the next grain
    grainPhaseAcc = 0;
    grainAmp = 0x7fff;
    grain2PhaseAcc = 0;
    grain2Amp = 0x7fff;
    LED_PORT ^= 1 << LED_BIT; // Faster than using digitalWrite
  }
  
  // Increment the phase of the grain oscillators
  grainPhaseAcc += grainPhaseInc;
  grain2PhaseAcc += grain2PhaseInc;

  // Convert phase into a triangle wave
  value = (grainPhaseAcc >> 7) & 0xff;
  if (grainPhaseAcc & 0x8000) value = ~value;
  // Multiply by current grain amplitude to get sample
  output = value * (grainAmp >> 8);

  // Repeat for second grain
  value = (grain2PhaseAcc >> 7) & 0xff;
  if (grain2PhaseAcc & 0x8000) value = ~value;
  output += value * (grain2Amp >> 8);

  // Make the grain amplitudes decay by a factor every sample (exponential decay)
  grainAmp -= (grainAmp >> 8) * grainDecay;
  grain2Amp -= (grain2Amp >> 8) * grain2Decay;

  // Scale output to the available range, clipping if necessary
  output >>= 9;
  if (output > 255) output = 255;

  // Output to PWM (this is faster than using analogWrite)  
  PWM_VALUE = output;
}

//////////////////////////////////////////////////////////////////////////////////
//
// Start of additional code for Touchscreen and Display
//
//////////////////////////////////////////////////////////////////////////////////

#define NUMPOTS 5
int potval[NUMPOTS];
int potlast[NUMPOTS];

// XPT2046 Touchscreen Definitions
//
#define T_CS  8
// MOSI=11, MISO=12, SCK=13
#define T_IRQ  255  //  Set to 255 to disable interrupts and rely on software polling
XPT2046_Touchscreen ts(T_CS, T_IRQ);
#define T_MAX 3700UL  // Should be 4096, but by experiment, working to this value
#define T_MIN  300UL  // Should be 0, but by experiment, working to this value

// ILI9488 Display Definitions
//
#define ILI_CS  10
#define ILI_DC  7
#define ILI_RST 9
Arduino_DataBus *bus = new Arduino_HWSPI(ILI_DC, ILI_CS);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, ILI_RST, 0 /* rotation */, false /* IPS */);

// If the resolution or rotation changes, then the gfx
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

  gfxPotInit();
}

void gfxLoop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    int pot = x2pot(xt2xdsp(p.x));
    int pval = y2val(p.y);
    if ((pot != -1) && (pval != -1)) {
      potval[pot] = pval;
      gfxPotValue(pot);
      potlast[pot] = potval[pot];
    }
  }
#ifdef TEST
  for (int i=0; i<NUMPOTS; i++) {
    Serial.print(1023-potval[i]);
    Serial.print("\t");
  }
  Serial.print("\n");
#endif
}

// Translate pot number into a set of on-screen slider pots.
//
// Assuming a 320x480 display in landscape mode.
// For 5 sliders the following "design" is being used:
//   Width of pots 80px, so total 400px is pots.
//   Height (length) of pots is 300px.
//   Gap between pots is 10px, so total of 40px between buttons.
//   This leaves top/bottom gap of (320-300)/2 = 10
//   And left/right gap of (480-400-40)/2 = 20
//
#define POT_W     80
#define POT_X_MIN 10
#define POT_X_GAP 10
#define POT_Y_MIN 5   // Must be > POT_BAR/2
#define POT_H     300
#define POT_BAR   6

void gfxPotInit () {
  for (int i=0; i<NUMPOTS; i++) {
    potval[i] = 1023;  // Prefix to "high"
    potlast[i] = 1023;
    int xc = pot2xc(i);

    // Background to the pot goes "full height"
    gfx->fillRect(xc, 0, POT_W, h, LIGHTGREY);
    gfxPotValue(i);

#ifdef TEST
    Serial.print("gfxPotInit ");
    Serial.print(i);
    Serial.print("\t");
    Serial.println(xc);
#endif
  }
}

int gfxAnalogRead(int pot) {
  if (pot < NUMPOTS) {
    // All pots are "wired backwards" due to screen coordinates, etc.
    // so to replicate analogRead, invert them here.
    return 1023-potval[pot];
  }
}

void gfxPotValue (int pot) {
  int xc = pot2xc(pot);
  int oldyc = pot2yc(potlast[pot]);
  int yc = pot2yc(potval[pot]);
#ifdef TEST
  Serial.print("gfxPotValue ");
  Serial.print(pot);
  Serial.print("\t");
  Serial.print(xc);  
  Serial.print("\t");
  Serial.println(yc);
#endif

  gfx->fillRect(xc, oldyc-POT_BAR/2, POT_W, POT_BAR, LIGHTGREY);
  gfx->fillRect(xc, yc-POT_BAR/2, POT_W, POT_BAR, GREEN);
}

int pot2xc (int pot) {
  // Need remainder after dividing by the number of pots,
  // which will give us the position within the row.
  return POT_X_MIN+(POT_X_GAP+POT_W)*(pot % NUMPOTS);
}

int pot2yc (int val) {
  // Use standard pot values of 0 to 1023 for scaling.
  return (int)(POT_Y_MIN+((long)POT_H*(long)val)/1024L);
}

int x2pot (int xc) {
  // Find the pot number corresponding to a specific X coord
  //    XC MIN < pot xc < XC MIN + POT width
  //
  // If outside the range, returns -1.
  // If inside the gap, returns -1.
  //
  // Ensure pots are in the range.
  if ((xc < POT_X_MIN) || (xc > POT_X_MIN+(POT_W+POT_X_GAP)*NUMPOTS-POT_X_GAP)) return -1;

  xc = xc - POT_X_MIN;

  int xbtn = -1;
  for (int i=0; i<NUMPOTS && xc>0; i++) {
    if (xc < POT_W) {
      // We're in the space for this pot
      xbtn = i;
      break;
    }
    xc = xc - (POT_W+POT_X_GAP);
  }

  return xbtn;
}

int y2val (int yc) {
  // Translated Y coord into a 0 to 1023 range similar to an analogRead().
  // NOTE: This is NOT the display Y coord (0 to 319), but the TOUCH
  //       Y coord, which goes from 0 to T_MAX.  This means we don't lose
  //       resolution, but does mean some translating is required...
  //
  // If outside the range, returns -1.
  //
  // Ensure pots are in the range.
  //
  // Actually, I cheat here and tweak the "touch" range manually to 
  // match what seems to come back for better control/accuracy.
  //
  int t_min = T_MIN;
  int t_max = T_MAX;
//  int t_min = (int)((T_MAX*(long)POT_Y_MIN)/(long)h);
//  int t_max = (int)((T_MAX*((long)POT_Y_MIN+(long)POT_H))/(long)h);
  if (yc < t_min) yc = t_min;
  if (yc > t_max) yc = t_max;

  long t_h = t_max-t_min;
  int retval = (int)(((long)yc - (long)t_min)*1023L/t_h);
#ifdef TEST
  Serial.print("y2val: ");
  Serial.print(yc);
  Serial.print("\t");
  Serial.print(t_min);
  Serial.print(" < ");
  Serial.print(yc);
  Serial.print(" < ");
  Serial.print(t_max);
  Serial.print("\t(");
  Serial.print(t_h);
  Serial.print(")\tRet=");
  Serial.println(retval);
#endif
  return retval;
}


unsigned xt2xdsp(unsigned touch_x) {
  return (unsigned)((((unsigned long)w)*touch_x)/4096UL);
}

unsigned yt2ydsp(unsigned touch_y) {
  return (unsigned)((((unsigned long)h)*touch_y)/4096UL);
}
