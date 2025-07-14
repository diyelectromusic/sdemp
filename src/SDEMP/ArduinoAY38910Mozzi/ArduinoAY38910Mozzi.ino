/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino AY-3-8910 Mozzi
//  https://diyelectromusic.com/2025/07/14/arduino-and-ay-3-8910-part-4/
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
    Arduino AY-3-8910 - https://github.com/Andy4495/AY3891x
    Mozzi External Output - https://github.com/sensorium/Mozzi/tree/master/examples/13.External_Audio_Output
*/
#include "AY3891x.h"

#define TIMING_PIN 11

#include "MozziConfigValues.h"
#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_EXTERNAL_CUSTOM
#define MOZZI_AUDIO_BITS 8
#define MOZZI_CONTROL_RATE 64
#define MOZZI_AUDIO_RATE 16384
#define MOZZI_ANALOG_READ MOZZI_ANALOG_READ_NONE
#include <Mozzi.h>
#include <Oscil.h>
#include <tables/cos2048_int8.h>
#include <mozzi_midi.h>
#include <mozzi_fixmath.h>

// use: Oscil <table_size, update_rate> oscilName (wavetable), look in .h file of table #included above
Oscil<COS2048_NUM_CELLS, MOZZI_AUDIO_RATE> aCarrier(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, MOZZI_AUDIO_RATE> aModulator(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, MOZZI_CONTROL_RATE> kModIndex(COS2048_DATA);

Q8n8 mod_index;// = float_to_Q8n8(2.0f);
Q16n16 deviation;
Q16n16 carrier_freq, mod_freq;
Q8n8 mod_to_carrier_ratio = float_to_Q8n8(1.f);

// Comment out to ignore button
#define BUTTON A5
int lastbtn;
bool fmmode;

int timingPinValue;
void timingHigh () {
#ifdef TIMING_PIN
  timingPinValue = HIGH;
  digitalWrite(TIMING_PIN, timingPinValue);
#endif
}

void timingLow () {
#ifdef TIMING_PIN
  timingPinValue = LOW;
  digitalWrite(TIMING_PIN, timingPinValue);
#endif
}

void timingToggle () {
#ifdef TIMING_PIN
  timingPinValue = !timingPinValue;
  digitalWrite(TIMING_PIN, timingPinValue);
#endif
}

//-------------------------------------------------------------------
//
//   AY-3-8910 Driver
//
//-------------------------------------------------------------------

#define BC1  14  // A0
#define BC2  15  // A1
#define BDIR 16  // A2

// DA7-DA0, BDIR, BC1, BC2
AY3891x psg( 17, 8, 7, 6, 5, 4, 3, 2, BDIR, BC2, BC1);
// NOTE: D2:D7 = PORTD[2:7]
//       D8 = PORTB[0]
//       D17/A3 = PORTC[3]

#define BC1LOW  {PORTC &= 0xFE;} // A0 LOW
#define BC1HIGH {PORTC |= 0x01;} // A0 HIGH
#define BC2LOW  {PORTC &= 0xFD;} // A1 LOW
#define BC2HIGH {PORTC |= 0x02;} // A1 HIGH
#define BDIRLOW  {PORTC &= 0xF7;} // A2 LOW
#define BDIRHIGH {PORTC |= 0x04;} // A2 HIGH

void ayFastWriteSetup () {
  // Fix output modes for all pins too
  DDRD |= 0xFC;  // DA0-DA5
  DDRC |= 0x0F;  // DA7, BC1, BC2, BDIR
  DDRB |= 0x01;  // DA6

  // Fix BC2 high for later fast writes
  BC2HIGH;
}

void ayFastWrite (byte reg, byte val) {
  // Mode=Addr Latch
  BC1HIGH;
  BDIRHIGH;

  // Latch address
  // NB: Addresses are all in range 0..15 so don't need to
  //     worry about writing out bits 6,7 - just ensure set to zero
  PORTD = (PORTD & 0x03) | ((reg & 0xCF)<<2); // Bits 0-5 map to PORTD[2:7]
  PORTB = (PORTB & 0xFE); // Bit 6 maps to PORTB[0]
  PORTC = (PORTC & 0xF7); // Bit 7 maps to PORTC[3]

  // Need 400nS Min
  delayMicroseconds(1);

  // Mode = Inactive
  BC1LOW;
  BDIRLOW;

  // Need 100nS settle then 50nS preamble
  delayMicroseconds(1);

  // Mode = Write
  BC1LOW;
  BDIRHIGH;

  // Write data
  PORTD = (PORTD & 0x03) | ((val & 0xCF)<<2); // Shift bits 0:5 to 2:7
  PORTB = (PORTB & 0xFE) | ((val & 0x40)>>6); // Shift bit 6 to 0
  PORTC = (PORTC & 0xF7) | ((val & 0x80)>>4); // Shift bit 7 to 3

  // Need 500nS min
  delayMicroseconds(1);

  // Mode = Inactive
  BC1LOW;
  BDIRLOW;

  // Need 100nS min
  // Assume the normal machine of running other code will give this
}

// Taken from AY3891x library examples...
//
// The following code generates a 1 MHz 50% duty cycle output to be used
// as the clock signal for the AY-3-891x chip.
#define AY_CLOCK 9 // D9
void aySetup () {
  pinMode(AY_CLOCK, OUTPUT);
  digitalWrite(AY_CLOCK, LOW);

  // Timer 1
  // Use CTC mode: WGM13..0 = 0100
  // Toggle OC1A on Compare Match: COM1A1..0 = 01
  // Use ClkIO with no prescaling: CS12..0 = 001
  // Interrupts off: TIMSK1 = 0
  // OCR1A = interval value
  //
  // NB: As OC1A toggles on match, need counter 
  //     running at 2MHz for a 1MHz clock signal.
  TCCR1A = (1 << COM1A0);
  TCCR1B = (1 << WGM12) | (1 << CS10);
  TCCR1C = 0;
  TIMSK1 = 0;
  OCR1AH = 0;
  OCR1AL = 7; // 16MHz / 8 = 2MHz Counter

  psg.begin();

  // Output highest frequency on each channel, but set level to 0
  // Highest freq = 1000000 / (16 * 1) = 62500
  psg.write(AY3891x::ChA_Amplitude, 0);
  psg.write(AY3891x::ChA_Tone_Period_Coarse_Reg, 0);
  psg.write(AY3891x::ChA_Tone_Period_Fine_Reg,   0);
  psg.write(AY3891x::ChB_Amplitude, 0);
  psg.write(AY3891x::ChB_Tone_Period_Coarse_Reg, 0);
  psg.write(AY3891x::ChB_Tone_Period_Fine_Reg,   0);
  psg.write(AY3891x::ChC_Amplitude, 0);
  psg.write(AY3891x::ChC_Tone_Period_Coarse_Reg, 0);
  psg.write(AY3891x::ChC_Tone_Period_Fine_Reg,   0);

  // LOW = channel is in the mix.
  // Turn everything off..
  psg.write(AY3891x::Enable_Reg, 0xFF);

  ayFastWriteSetup();
}

void ayOutput (byte ch, byte level) {
  // Convert from 8-bit value to a 4-bit level
  switch (ch) {
    case 0:
      ayFastWrite(AY3891x::ChA_Amplitude, level>>4);
      break;
    case 1:
      ayFastWrite(AY3891x::ChB_Amplitude, level>>4);
      break;
    case 2:
      ayFastWrite(AY3891x::ChC_Amplitude, level>>4);
      break;
    default:
      // ignore
      break;
  }
}

//-------------------------------------------------------------------
//
//   Mozzi Support
//
//-------------------------------------------------------------------

void audioOutput(const AudioOutput f)
{
  timingToggle();
  int out = MOZZI_AUDIO_BIAS + f.l();
  ayOutput(0,out);
}

// With a sample rate of 16385Hz we need to
// push out a sample every 60uS.
// Using +60 is a little flat however, so
// I'm using 58 to allow for processing time.
unsigned long lastmicros;
bool canBufferAudioOutput() {
  unsigned long nowmicros = micros();
  if (nowmicros > lastmicros+58) {
    // time for a new sample
    lastmicros=nowmicros;
    return true;
  }
  // Not time for a new sample yet
  return false;
}

void setFreqs(Q8n8 midi_note){
  carrier_freq = Q16n16_mtof(Q8n8_to_Q16n16(midi_note)); // convert midi note to fractional frequency
  mod_freq = ((carrier_freq>>8) * mod_to_carrier_ratio); // (Q16n16>>8)   Q8n8 = Q16n16, beware of overflow
  deviation = ((mod_freq>>16) * mod_index); // (Q16n16>>16)   Q8n8 = Q24n8, beware of overflow
  aCarrier.setFreq_Q16n16(carrier_freq);
  aModulator.setFreq_Q16n16(mod_freq);
}

//-------------------------------------------------------------------
//
//   Arduino setup/loop
//
//-------------------------------------------------------------------

void setup() {
#ifdef TIMING_PIN
  pinMode(TIMING_PIN, OUTPUT);
  timingLow();
#endif
  aySetup();

#ifdef BUTTON
  pinMode(BUTTON, INPUT_PULLUP);
  lastbtn = HIGH;
#endif
  fmmode = true;

  kModIndex.setFreq(.4f);
  setFreqs(Q7n0_to_Q7n8(69));  // A4 = 440Hz
  startMozzi();
}

void updateControl() {
  int btn = digitalRead(BUTTON);
  if (lastbtn == HIGH && btn == LOW) {
    // Button pressed
    if (fmmode) {
      fmmode = false;
    } else {
      fmmode = true;
    }
  }
  lastbtn = btn;

  // vary the modulation index
  mod_index = (Q8n8)350+kModIndex.next();
  setFreqs(Q7n0_to_Q7n8(69));  // A4 = 440Hz
}

AudioOutput updateAudio() {
  if (fmmode) {
    Q15n16 modulation = deviation * aModulator.next() >> 8;
    return MonoOutput::from8Bit(aCarrier.phMod(modulation));
  } else {
    return MonoOutput::from8Bit(aCarrier.next());
  }
}

void loop() {
  audioHook();
}
