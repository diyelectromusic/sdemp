/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino AY-3-8910 MIDI
//  https://diyelectromusic.com/2025/07/11/arduino-and-ay-3-8910-part-2/
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
    Arduino AY-3-8910 - https://github.com/Andy4495/AY3891x
*/
#include <MIDI.h>
#include "AY3891x.h"
#include "AY3891x_sounds.h"  // Contains the divisor values for the musical notes

MIDI_CREATE_DEFAULT_INSTANCE();
#define MIDI_CHANNEL 1
#define POLYPHONY 3
byte playing[POLYPHONY];
int playidx;

// Comment this out for a fixed volume based on MIDI velocity
// Envelop values are defined in the AY-3-8910 datasheet
#define ENVELOPE 0b1000
#define ENVFREQ  10      // Envelope freq (Hz)
byte ayenv;

// Comment this out if don't want new notes
// to overwrite already playing notes.
#define POLYOVERWRITE

// Define the range covered by the AY-3-8910 library
#define MIDI_NOTE_START 12  // C0
#define MIDI_NOTE_END   119 // B8

// Comment out to ignore button
#define BUTTON A5
int lastbtn;

AY3891x psg( 17, 8, 7, 6, 5, 4, 3, 2, 16, 15, 14);

// Taken from AY3891x library examples...
//
// The following code generates a 1 MHz 50% duty cycle output to be used
// as the clock signal for the AY-3-891x chip.
#define AY_CLOCK 9 // D9
static void clockSetup()
{
  pinMode(AY_CLOCK, OUTPUT);
  digitalWrite(AY_CLOCK, LOW);

  // Timer 1
  // Use CTC mode: WGM13..0 = 0100
  // Toggle OC1A on Compare Match: COM1A1..0 = 01
  // Use ClkIO with no prescaling: CS12..0 = 001
  // Interrupts off: TIMSK0 = 0
  // OCR0A = interval value
  TCCR1A = (1 << COM1A0);
  TCCR1B = (1 << WGM12) | (1 << CS10);
  TCCR1C = 0;
  TIMSK1 = 0;
  OCR1AH = 0;
  OCR1AL = 7; // For 1MHz Clock
}

void aySetup () {
  psg.begin();
  // LOW = channel is in the mix.
  // Add channels A, B, C.
  psg.write(AY3891x::Enable_Reg, ~(MIXER_TONE_A_DISABLE|MIXER_TONE_B_DISABLE|MIXER_TONE_C_DISABLE));

#ifdef ENVELOPE
  ayenv = ENVELOPE;
  psg.write(AY3891x::Env_Shape_Cycle, ayenv);
  uint16_t envfreq = 3906 / ENVFREQ; // 1MHz/(256*freq)
  psg.write(AY3891x::Env_Period_Coarse_Reg, envfreq >> 8);
  psg.write(AY3891x::Env_Period_Fine_Reg, envfreq & 0xFF);
#endif
}

void ayNoteOn (byte ch, byte note, byte vel) {
  switch (ch) {
    case 0:
      psg.write(AY3891x::ChA_Tone_Period_Coarse_Reg, pgm_read_word(&Notes[note]) >> 8);
      psg.write(AY3891x::ChA_Tone_Period_Fine_Reg, pgm_read_word(&Notes[note]) & 0xFF);
#ifdef ENVELOPE
      psg.write(AY3891x::ChA_Amplitude, AMPLITUDE_CONTROL_MODE);
#else
      psg.write(AY3891x::ChA_Amplitude, vel);
#endif
      break;
    case 1:
      psg.write(AY3891x::ChB_Tone_Period_Coarse_Reg, pgm_read_word(&Notes[note]) >> 8);
      psg.write(AY3891x::ChB_Tone_Period_Fine_Reg, pgm_read_word(&Notes[note]) & 0xFF);
#ifdef ENVELOPE
      psg.write(AY3891x::ChB_Amplitude, AMPLITUDE_CONTROL_MODE);
#else
      psg.write(AY3891x::ChB_Amplitude, vel);
#endif
      break;
    case 2:
      psg.write(AY3891x::ChC_Tone_Period_Coarse_Reg, pgm_read_word(&Notes[note]) >> 8);
      psg.write(AY3891x::ChC_Tone_Period_Fine_Reg, pgm_read_word(&Notes[note]) & 0xFF);
#ifdef ENVELOPE
      psg.write(AY3891x::ChC_Amplitude, AMPLITUDE_CONTROL_MODE);
#else
      psg.write(AY3891x::ChC_Amplitude, vel);
#endif
      break;
    default:
      // Ignore
      break;
  }
}

void ayNoteOff (byte ch, byte note) {
  switch (ch) {
    case 0:
      psg.write(AY3891x::ChA_Amplitude, 0);
      break;
    case 1:
      psg.write(AY3891x::ChB_Amplitude, 0);
      break;
    case 2:
      psg.write(AY3891x::ChC_Amplitude, 0);
      break;
    default:
      // Ignore
      break;
  }
}

void ayEnvelope () {
  // increase the envelope, selecting between
  // last 8 as per datasheet.
  ayenv++;
  if (ayenv > 15) ayenv = 8;
  psg.write(AY3891x::Env_Shape_Cycle, ayenv);
}

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  if (velocity == 0) {
    handleNoteOff(channel, pitch, velocity);
    return;
  }

  if ((pitch < MIDI_NOTE_START) || (pitch > MIDI_NOTE_END)) {
    // The note is out of range for us so do nothing
    return;
  }

#ifdef POLYOVERWRITE
  if (playing[playidx] != 0) {
    // Find next slot free to play, overwriting oldest playing notes
    playidx++;
    if (playidx >= POLYPHONY) playidx = 0;
  }
  ayNoteOn(playidx, pitch-MIDI_NOTE_START, velocity>>4);
  playing[playidx] = pitch;
#else
  // Find a free slot.
  // Ignore if all notes already sounding.
  for (int i=0; i<POLYPHONY; i++) {
    if (playing[i] == 0) {
      ayNoteOn(i, pitch-MIDI_NOTE_START, velocity>>4);
      playing[i] = pitch;
      // Once a free slot is claimed, stop looking for more
      return;
    }
  }
#endif
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  if ((pitch < MIDI_NOTE_START) || (pitch > MIDI_NOTE_END)) {
    // The note is out of range for us so do nothing
    return;
  }

  // If we receive a note off event for the playing note, turn it off
  for (int i=0; i<POLYPHONY; i++) {
    if (pitch == playing[i]) {
      ayNoteOff(i, pitch-MIDI_NOTE_START);
      playing[i] = 0;
    }
  }
}

void setup() {
  clockSetup();
  aySetup();
  MIDI.begin(MIDI_CHANNEL);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

#ifdef BUTTON
  pinMode(BUTTON, INPUT_PULLUP);
  lastbtn = HIGH;
#endif
}

void loop() {
  MIDI.read();

  int btn = digitalRead(BUTTON);
  if (lastbtn == HIGH && btn == LOW) {
    // Button pressed
    ayEnvelope();
    delay(100);
  }
  lastbtn = btn;
}

