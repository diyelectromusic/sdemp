/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino USB/Serial MIDI PWM CV
//  https://diyelectromusic.com/2025/05/24/usb-midi-to-serial-and-cv-gate/
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
  Using principles from the following Arduino tutorials:
    Secrets of Arduino PWM - https://www.arduino.cc/en/Tutorial/SecretsOfArduinoPWM
    Arduino 101 Timers and Interrupts - https://www.robotshop.com/community/forum/t/arduino-101-timers-and-interrupts/13072
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    USB Host 2.0 Library - https://github.com/felis/USB_Host_Shield_2.0
    Arduino USB Transport - https://github.com/lathoub/Arduino-USBMIDI
*/
#include <MIDI.h>
#include <UHS2-MIDI.h>

//#define PWMTEST

USB Usb;
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, MIDI_U);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI_S);
#define MIDI_CHANNEL 1

#define MIDI_LED LED_BUILTIN

#define PWM_CV_PIN 3 // Cannot change this without changing all the PWM code
#define GATE_PIN   2

// Uncomment if GATE should be a PULSE (so a TRIGGER) not a LEVEL (GATE)
//#define GATE_PULSE 10  // Pulse width (mS)

void pwmSetup() {
  // Initialise Timer 2 as follows:
  //    Output on pin 3 (OC2B).
  //    Run at CPU clock speed (8MHz for 3V3 board).
  //    Use Fast PWM mode (count up, then reset).
  //    Use TOP value of OCR2A, set to 239.
  //    PWM value is updated by writing to OCR2B
  //    This gives PWM Freq = 8000000/240 = 33.3kHz
  //
  // So set up PWM for Timer 2, Output B:
  //   WGM2[2:0] = 111 = Fast PWM; TOP=OCR2A
  //   COM2A[1:0] = 00 = OC2A disconnected
  //   COM2B[1:0] = 10  Clear OC2B on cmp match up; Set OC2B at BOTTOM
  //   CS2[2:0] = 001 = No prescalar
  //   TOIE2 = 0 = No interrupts
  //
  pinMode(PWM_CV_PIN, OUTPUT);
  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS20);
  OCR2A = 239;
}

void pwmSet (uint8_t pwmval) {
  OCR2B = pwmval;
}

#define MIDI_LOWEST_NOTE  36 // C2
#define MIDI_HIGHEST_NOTE 96 // C7

// There is a choice in how to handle out of range notes.
// This function will clamp to the highest/lowest notes.
uint8_t midi2pwm (uint8_t note) {
  if (note < MIDI_LOWEST_NOTE) {
    note = MIDI_LOWEST_NOTE;
  }
  if (note > MIDI_HIGHEST_NOTE) {
    note = MIDI_HIGHEST_NOTE;
  }

  return (note - MIDI_LOWEST_NOTE) * 4;
}

unsigned long gatePulseMillis;
void gateSetup () {
  pinMode(GATE_PIN, OUTPUT);
  gateOff();
  gatePulseMillis = 0;
}

void gateOn () {
  digitalWrite(GATE_PIN, HIGH);
#ifdef GATE_PULSE
  gatePulseMillis = millis() + GATE_PULSE;
#endif
}

void gateOff () {
  digitalWrite(GATE_PIN, LOW);
}

void gateScan () {
#ifdef GATE_PULSE
  if (gatePulseMillis < millis()) {
    gateOff();
  }
#endif
}

#ifdef MIDI_LED
unsigned long ledtimer;
#define LED_TIMEON 200 // mS
void ledSetup() {
  pinMode(MIDI_LED, OUTPUT);
  ledOff();
  ledtimer = 0;
}

void ledOn() {
  digitalWrite(MIDI_LED, HIGH);
//  digitalWrite(MIDI_LED, LOW);
  ledtimer = millis() + LED_TIMEON;
}

void ledOff() {
  digitalWrite(MIDI_LED, LOW);
//  digitalWrite(MIDI_LED, HIGH);
}

void ledScan() {
  if (ledtimer < millis()) {
//    ledOff();
  }
}
#else
void ledSetup() {}
void ledOn() {}
void ledOff() {}
void ledScan() {}
#endif

void midiSetup() {
  ledSetup();
  Usb.Init();
  MIDI_U.begin(MIDI_CHANNEL_OMNI);
  MIDI_U.turnThruOff();
  MIDI_S.begin(MIDI_CHANNEL_OMNI);
  MIDI_S.turnThruOff();
}

void midiScan() {
  ledScan();
  Usb.Task();

  if (MIDI_U.read()) {
    midi::MidiType mt  = MIDI_U.getType();
    uint8_t d1 = MIDI_U.getData1();
    uint8_t d2 = MIDI_U.getData2();
    uint8_t ch = MIDI_U.getChannel();
    if (mt == midi::NoteOn) {
      handleNoteOn(d1, d2, ch);
      MIDI_S.send(mt, d1, d2, ch);
    } else if (mt == midi::NoteOff) {
      handleNoteOff(d1, d2, ch);
      MIDI_S.send(mt, d1, d2, ch);
    } else if (mt != midi::SystemExclusive) {
      MIDI_S.send(mt, d1, d2, ch);
    } else {
      int mSxLen = d1 + 256*d2;
      MIDI_S.sendSysEx(mSxLen, MIDI_U.getSysExArray(), true);
    }
  }

  if (MIDI_S.read()) {
    midi::MidiType mt  = MIDI_S.getType();
    uint8_t d1 = MIDI_S.getData1();
    uint8_t d2 = MIDI_S.getData2();
    uint8_t ch = MIDI_S.getChannel();
    if (mt == midi::NoteOn) {
      handleNoteOn(d1, d2, ch);
      MIDI_U.send(mt, d1, d2, ch);
    } else if (mt == midi::NoteOff) {
      handleNoteOff(d1, d2, ch);
      MIDI_U.send(mt, d1, d2, ch);
    } else if (mt != midi::SystemExclusive) {
      MIDI_U.send(mt, d1, d2, ch);
    } else {
      int mSxLen = d1 + 256*d2;
      MIDI_U.sendSysEx(mSxLen, MIDI_S.getSysExArray(), true);
    }
  }
}

uint8_t playing;
void handleNoteOn (uint8_t note, uint8_t vel, uint8_t channel) {
  if (channel != MIDI_CHANNEL) {
    // Not our channel so do nothing
    return;
  }

  if ((note < MIDI_LOWEST_NOTE) || (note > MIDI_HIGHEST_NOTE)) {
    // Out of range, so ignore
    return;
  }

  if (vel == 0) {
    handleNoteOff(note, vel, channel);
    return;
  }

  ledOn();
  pwmSet(midi2pwm(note));
  gateOn();

  playing = note;
}

void handleNoteOff (uint8_t note, uint8_t vel, uint8_t channel) {
  if (channel != MIDI_CHANNEL) {
    // Not our channel so do nothing
    return;
  }

  if ((note < MIDI_LOWEST_NOTE) || (note > MIDI_HIGHEST_NOTE)) {
    // Out of range, so ignore
    return;
  }

  if (note == playing) {
    ledOff();
    gateOff();
    playing = 0;

    // Option: Set CV to zero?
    //pwmSet(0);
    // Option: Set CV to provided note
    //pwmSet(midi2pwm(note));
    // Or just leave CV as it was...
    // (this allows envelopes to complete without changing pitch!)
  }
}

void setup() {
  gateSetup();
  pwmSetup();
  midiSetup();
}

uint8_t note=MIDI_LOWEST_NOTE;
void loop() {
  gateScan();
  midiScan();

#ifdef PWMTEST
  pwmSet(midi2pwm(note));
  note+=12;
  if (note>MIDI_HIGHEST_NOTE) note = MIDI_LOWEST_NOTE;
  delay(10000);
#endif
}
