/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Auduino MIDI Slider Waveform
//  https://diyelectromusic.wordpress.com/2021/08/21/auduino-slider-waveform-granular-synthesis/
//
//  Dec 2020: Update to add an optional volume control on A5.
//  Jul 2021: Update to add MIDI.
//  Aug 2021: Update to build grain from a stored wavetable, built
//            up from multiplexed slider potentiometers.
*/
// Auduino, the Lo-Fi granular synthesiser
// https://diyelectromusic.wordpress.com/2021/08/18/auduino-wavetable-granular-synthesis/
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
#include <MIDI.h>

// Uncomment this line to test updating the wavetable
//#define TEST 1
//#define SERPLOT 1
//#define SERPRINT 1
//#define SERALG 1
#define TESTNOTE 69 // A4 - "Concert A" @ 440Hz

#define MIDI_CHANNEL 1
//#define MIDI_LED     13
MIDI_CREATE_DEFAULT_INSTANCE();
int midiNote;
int numMidiNotes;
#define MIDI_NOTE_START 24 // 21=A0; 24=C1

// This is the actual wavetable used. If this is changed
// then updateWavetable() will need re-writing.
#define WTSIZE 256
unsigned char wavetable[WTSIZE];
#define WAVETABLE 1  // Uncomment to revert to previous non-wavetable behaviour

// Need to set up the single analog pin to use for the MUX
#define MUXPIN A0
// And the digital lines to be used to control it.
// Four lines are required for 16 pots on the MUX.
// You can drop this down to three if only using 8 pots.
#define MUXLINES 4
byte muxpins[MUXLINES]={13,12,11,10};
// Uncomment this if you want to control the MUX EN pin
// (optional, only required if you have several MUXs or
// are using the IO pins for other things too).
// This is active LOW.
//#define MUXEN    3

// Using 16 pots to define the wave.  If this changes or
// the size of the wavetable changes, then updateWavetable()
// will need re-writing.
#define NUMPOTS 16
// Use this to define the order for reading the pots and
// to decide if any of them need "mirroring" (i.e. 1023-analogRead()).
//
// NB: bit 0 of MIRRORPOTS is for pots[0], so they are effectively
//     listed backwards here...
byte pots[NUMPOTS] = {7,6,5,4,3,2,1,0,8,9,10,11,12,13,14,15};
#define MIRRORPOTS ((uint16_t)0x00FF)  // First 8 are mirrored

byte pot;
int potReading[NUMPOTS];

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
uint8_t volume;

// Map Analogue channels
#define SYNC_CONTROL         (1)
#define GRAIN_FREQ_CONTROL   (5)
#define GRAIN_DECAY_CONTROL  (4)
#define GRAIN2_FREQ_CONTROL  (3)
#define GRAIN2_DECAY_CONTROL (2)
//#define VOL_CONTROL          (5) // Comment out to ignore volume
//#define CW_POTS   // Comment out for original "counter-clockwise = 5V" pot wiring

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
//  return (midiTable[(1023-input) >> 3]);
  return (midiTable[input]);
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
#ifdef MIDI_LED
  pinMode(MIDI_LED, OUTPUT);
#endif
#ifdef MIDI_CHANNEL
#ifndef TEST
  MIDI.begin(MIDI_CHANNEL);
  MIDI.setHandleNoteOn(midiNoteOn);
  MIDI.setHandleNoteOff(midiNoteOff);
#endif
  numMidiNotes = sizeof(midiTable)/sizeof(midiTable[0]);
#endif

  muxInit();
  wavetableSetup();

  pinMode(PWM_PIN,OUTPUT);
  audioOn();
  pinMode(LED_PIN,OUTPUT);
}

void loop() {
#ifdef MIDI_CHANNEL
#ifdef TEST
  midiNote = TESTNOTE;
#else
  MIDI.read();
#endif
#endif

  wavetableLoop();

  // The loop is pretty simple - it just updates the parameters for the oscillators.
  //
  // Avoid using any functions that make extensive use of interrupts, or turn interrupts off.
  // They will cause clicks and poops in the audio.
#ifdef SYNC_CONTROL
#ifdef CW_POTS
  syncPhaseInc = mapPhaseInc(1023-analogRead(SYNC_CONTROL)) / 4;
#else
  syncPhaseInc = mapPhaseInc(analogRead(SYNC_CONTROL)) / 4;
#endif
#else
  // MIDI Based frequency mapping
  if (midiNote == 0) {
    // No note is playing
    syncPhaseInc = 0;
  } else {
     syncPhaseInc = mapMidi (midiNote-MIDI_NOTE_START);
  }
#endif
  // Smooth frequency mapping
  //syncPhaseInc = mapPhaseInc(analogRead(SYNC_CONTROL)) / 4;
  
  // Stepped mapping to MIDI notes: C, Db, D, Eb, E, F...
  //syncPhaseInc = mapMidi(analogRead(SYNC_CONTROL));
  
  // Stepped pentatonic mapping: D, E, G, A, B
  //syncPhaseInc = mapPentatonic(analogRead(SYNC_CONTROL));

#ifdef CW_POTS
  grainPhaseInc  = mapPhaseInc(1023-analogRead(GRAIN_FREQ_CONTROL)) / 2;
  grainDecay     = (1023-analogRead(GRAIN_DECAY_CONTROL)) / 8;
  grain2PhaseInc = mapPhaseInc(1023-analogRead(GRAIN2_FREQ_CONTROL)) / 2;
  grain2Decay    = (1023-analogRead(GRAIN2_DECAY_CONTROL)) / 4;
#else
  grainPhaseInc  = mapPhaseInc(analogRead(GRAIN_FREQ_CONTROL)) / 2;
  grainDecay     = analogRead(GRAIN_DECAY_CONTROL) / 8;
  grain2PhaseInc = mapPhaseInc(analogRead(GRAIN2_FREQ_CONTROL)) / 2;
  grain2Decay    = analogRead(GRAIN2_DECAY_CONTROL) / 4;
#endif

#ifdef VOL_CONTROL
  volume = analogRead(VOL_CONTROL) >> 3;  // 0 to 127 scaling
#endif

#ifdef WAVE_CONTROL
  int newwave = analogRead(WAVE_CONTROL) >> 8; // 0 to 3
  if (newwave != wave) {
    wave = newwave;
    setWavetable(wave);
  }
#endif
}

SIGNAL(PWM_INTERRUPT)
{
  uint8_t value;
  uint16_t output;

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

#ifdef WAVETABLE
  // Use top 16 bits as the index into the wavetable
  value = wavetable[grainPhaseAcc >> 8];
#else
  // Convert phase into a triangle wave
  value = (grainPhaseAcc >> 7) & 0xff;
  if (grainPhaseAcc & 0x8000) value = ~value;
#endif
  // Multiply by current grain amplitude to get sample
  output = value * (grainAmp >> 8);

#ifdef WAVETABLE
  // Use top 16 bits as the index into the wavetable
  value = wavetable[grain2PhaseAcc >> 8];
#else
  // Repeat for second grain
  value = (grain2PhaseAcc >> 7) & 0xff;
  if (grain2PhaseAcc & 0x8000) value = ~value;
#endif
  output += value * (grain2Amp >> 8);    

  // Make the grain amplitudes decay by a factor every sample (exponential decay)
  grainAmp -= (grainAmp >> 8) * grainDecay;
  grain2Amp -= (grain2Amp >> 8) * grain2Decay;

  // Scale output to the available range, clipping if necessary
  output >>= 9;
  if (output > 255) output = 255;

#ifdef VOL_CONTROL
  output = output * volume;
  output >>= 7;
#endif

  // Output to PWM (this is faster than using analogWrite)  
  PWM_VALUE = output;
}

void midiLedOff () {
#ifdef MIDI_LED
   digitalWrite(MIDI_LED, LOW);
#endif
}

void midiLedOn () {
#ifdef MIDI_LED
   digitalWrite(MIDI_LED, HIGH);
#endif
}

void midiNoteOn(byte channel, byte pitch, byte velocity)
{
  if (velocity == 0) {
    // Handle this as a "note off" event
    midiNoteOff(channel, pitch, velocity);
    return;
  }

  if ((pitch < MIDI_NOTE_START) || (pitch >= MIDI_NOTE_START+numMidiNotes)) {
    // The note is out of range for us so do nothing
    return;
  }

  midiLedOn();
  midiNote = pitch;
}

void midiNoteOff(byte channel, byte pitch, byte velocity)
{
  // If we get a noteOff for the currently playing note turn it off
  if (pitch == midiNote) {
    midiNote = 0;
    midiLedOff();
  }
}

int muxAnalogRead (int mux) {
  if (mux >= NUMPOTS) {
    return 0;
  }

#ifdef MUXEN
  digitalWrite(MUXEN, LOW);  // Enable MUX
#endif

#ifdef TEST
#ifdef SERALG
  Serial.print(mux);
  Serial.print("\t");
#endif
#endif

  // Translate mux pot number into the digital signal
  // lines required to choose the pot.
  for (int i=0; i<MUXLINES; i++) {
    if (mux & (1<<i)) {
      digitalWrite(muxpins[i], HIGH);
#ifdef TEST
#ifdef SERALG
      Serial.print(1);
#endif
#endif
    } else {
      digitalWrite(muxpins[i], LOW);
#ifdef TEST
#ifdef SERALG
      Serial.print(0);
#endif
#endif
    }
  }

  // And then read the analog value from the MUX signal pin
  int algval = analogRead(MUXPIN);
#ifdef TEST
#ifdef SERALG
  Serial.print("\t");
  Serial.print(algval);
  Serial.print("\n");
#endif
#endif

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

void wavetableSetup() {
  // Build the initial wavetable
  for (int i=0; i<NUMPOTS; i++) {
    if (MIRRORPOTS & (1<<i)) {
      potReading[i] = 1023-muxAnalogRead(pots[i]);
    } else {
      potReading[i] = muxAnalogRead(pots[i]);
    }
    updateWavetable(i, potReading[i]);
  }
#ifdef TEST
#ifdef SERPLOT
  // Dump the values from the wavetable out to the serial port
  // for use with the Arduino serial plotter
  if (pot == 0) {
    for (int i=0; i<WTSIZE; i++) {
      Serial.println(wavetable[i]);
    }
    delay(1000);
  }
#endif
#endif
}

void wavetableLoop() {
  // Need to read the potentiometers and update the wavetable
  int algval;
  if (MIRRORPOTS & (1<<pot)) {
    algval = 1023-muxAnalogRead(pots[pot]);
  } else {
    algval = muxAnalogRead(pots[pot]);
  }
  if (algval != potReading[pot]) {
    // Need to update the reading and rebuild the wavetable
    potReading[pot] = algval;
    updateWavetable(pot, algval);
  }
  pot++;
  if (pot >= NUMPOTS) pot = 0;

#ifdef TEST
  // Dump the values from the wavetable out to the serial port
  // for use with the Arduino serial plotter
  if (pot == 0) {
#ifdef SERPLOT
    for (int i=0; i<WTSIZE; i++) {
      Serial.println(wavetable[i]);
    }
#endif
#ifdef SERALG
    for (int i=0; i<NUMPOTS; i++) {
      Serial.print(potReading[i]);
      Serial.print("\t");  
    }
    Serial.print("\n");
#endif
    delay(1000);
  }
#endif
}

// This function rebuilds the wavetable from potentiometer readings.
// It is designed for incremental updates, so will just adjust one
// value and the interpolated values between it and its neighbours.
//
// As written it assumes a wavetable of 256 values and 16 step updates.
// If the size of the wavetable or the number of potentiometers changes
// then this function will need re-writing.
//
uint16_t tempwt[33];
void updateWavetable (int idx, int newval) {
  if ((idx < 0) || (idx >= NUMPOTS) || (NUMPOTS != 16) || (WTSIZE != 256)) {
    // Values are out of range
    return;
  }

  // There are 256 values in the wavetable, but only 16 potentiometers
  // to define them, so there is one real value for every 16 values in the table.
  //
  // This means that we need to interpolate between values to fill in the gaps.
  //
  // We need to take care that the start and end of the table link up too, so
  // need to do something special for idx=0 and idx=15.
  //
  // I am treating each idx as being the first value in the table, then will work
  // on the following 15 values up until the next idx.
  //
  // I'm just using basic linear interpolation here.  More accurate algorithms
  // are available that would perform actual curve fitting, but I'm keeping it simple
  // for now.
  //
  // Note that although the actual wavetable has only 8-bit resolution, I'm using
  // 16-bit integers in the calculation, relying on the full 1023 bit range from
  // the analog readings, and only dropping back to 8-bits at the end.
  //
  // I'm using a 8-bit.8-bit fixed point numerical value for the calculation.
  // This means each value is 256 times bigger than it needs to be but anything
  // in the last 8 bits is treated as a fractional component.
  //
  // This also goes for the 10 bit analog reading, so the 0-1023 is turned into
  // and 8.2 fixed point value - i.e. 8 significant bits and 2 "decimal points".

  // Calculate the indexes into the wavetable to use:
  //    (idx-1)*16 .... idx*16 .... (idx+1)*16
  // As idx=0 is the start of the wavetable, we using count up from the lower
  // index to get the right part of the table.
  //
  int lastidx = (idx-1)*16;
  if (idx == 0) lastidx = 15*16;
  int nextidx = (idx+1)*16;
  if (idx == 15) nextidx = 0;
  int curidx = idx*16;

  // Turn the 10-bit analog value into a fixed point 8.2 value and
  // store it in our temporary wavetable
  tempwt[16] = newval << 6;

  // Now do the same for the last and first values from the real
  // wavetable, this time converting from 8-bit to 8.8 fixed point.
  tempwt[0] = wavetable[lastidx] << 8;
  tempwt[32] = wavetable[nextidx] << 8;

#ifdef TEST
#ifdef SERPRINT
  Serial.print(idx);
  Serial.print("\t");
  for (int i=0; i<33; i++) {
    Serial.print(tempwt[i]);
    Serial.print("\t");
  }
  Serial.print("\n");
#endif
#endif
  // Now calculate the 15 new values for the wavetable for the range
  //    lastidx to idx, represented by 0 to 16
  //    idx to nextidx, represented by 16 to 32
  //
  // Linear interpolation gives the equation for a point on a line
  // between two other points (x1,y1) and (x2,y2) as:
  //
  //   y = y1 + (x - x1) * gradient of the line
  //
  // Where the gradient of the line  = (y2 - y1) / (x2 - x1)
  //
  // In our case, x1 = 0 and x2 = 16 for each section of the wavetable.
  //
  // NB: Don't need to do the calculation for i=0 as that is defined
  //     by the existing value each time.
  //
  long lastgrad = ((long)tempwt[16] - (long)tempwt[0]) / 16;
  long nextgrad = ((long)tempwt[32] - (long)tempwt[16]) / 16;
  for (long i=1L; i<16; i++) {
    // lastidx to idx
    tempwt[i] = (uint16_t)((long)tempwt[0] + i*lastgrad);

    // idx to nextidx
    tempwt[16+i] = (uint16_t)((long)tempwt[16] + i*nextgrad);
  }

#ifdef TEST
#ifdef SERPRINT
  Serial.print(idx);
  Serial.print("\t");
  for (int i=0; i<33; i++) {
    Serial.print(tempwt[i]);
    Serial.print("\t");
  }
  Serial.print("\n");
#endif
#endif
  // Now write this section of the wavetable back, converting it
  // back from 8.8 fixed point into an 8-bit only value.
  for (int i=0; i<16; i++) {
    wavetable[lastidx+i] = tempwt[i]>>8;
    wavetable[curidx+i] = tempwt[16+i]>>8;
  }
}
