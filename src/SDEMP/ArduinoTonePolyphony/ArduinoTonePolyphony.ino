/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Tone Polyphony
//  https://diyelectromusic.wordpress.com/2021/02/05/arduino-tone-polyphony/
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
    Arduino Direct Port Manipulation - https://www.arduino.cc/en/Reference/PortManipulation
    Arduino Timer1  - https://playground.arduino.cc/Code/Timer1/


*/
#include <TimerOne.h>
#include <MIDI.h>

// This is required to set up the MIDI library.
// This overrides the default MIDI setup which uses
// the Arduino built-in serial port (pin 0 and 1).
struct MySettings : public MIDI_NAMESPACE::DefaultSettings {
  static const bool Use1ByteParsing = false; // Allow MIDI.read to handle all received data in one go
  static const long BaudRate = 31250;        // Doesn't build without this...
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, MySettings);

// Set up the MIDI channel to listen on
#define MIDI_CHANNEL 1

// Set up the MIDI codes to respond to by listing the lowest note
#define MIDI_NOTE_START 48   // C3

// Test pin is hardcoded to A0 or PORTC PC0.
// Use this to check the timing of the interrupt routine.
// Comment this out to disable the test.
#define TESTPIN 1
#define TESTMASK 1 // PC0

#define NUM_TONES 12
#define PORTDMASK  0b11111100; // Pins 2-7
#define PORTBMASK  0b00111111; // Pins 8-13
int pins[NUM_TONES]={2,3,4,5, 6,7,8,9, 10,11,12,13};

// This project runs code on the command of one of the Arduino Timers.
// This determines the length of the "tick" of the timer.
//
// Using a TICK of 10 means our code runs every
// 10 microseconds, i.e. at a frequency of 100kHz.
//
// A TICK of 50 is 20kHz and so on.
//
// Sounds are created by sending HIGH or LOW pulses to the IO pins.
// The length of the pulses determines the eventual frequency of the note played.
//
// The same TICK can be used for a whole range of frequencies as long as
// the TICK period is a useful divisor of the required frequencies.
//
// Calculations using a spreadsheet, and analysing the errors in the
// output frequencies from "rounding" to multiples of the basic pulse
// suggest aiming for a 20uS pulse.
//
// This supports a frequency range of 130.81Hz (C3) up to 523.25Hz (C5)
// without too much error at the high end.
//
// Basically, the tuning is tolerable at this range!
//
// Faster (i.e. lower) is always better but if the time spent in the
// timer routing to update all the pulses ends up longer than the TICK
// itself, everything will fall apart.
//
#define TICK   40    // 40uS

// The frequency table is specified in terms of the number of pulses
// required to toggle the output pin at the right frequency.
//
// The counters are initialised to these values and decremented by 1
// each time the interrupt routine runs.  When they reach zero the
// output pin for that frequency is toggled and the counter is reset
// to these values to start again.
//
uint8_t freq[NUM_TONES]={
   95, 90, 85, 80, 75, 71, 67, 63, 60, 56, 53, 50 // C3 to B3
// 47, 45, 42, 40, 37, 35, 33, 31, 30, 28, 26, 25 // C4 to B4
};
#ifdef TICK20
uint8_t freq[NUM_TONES]={
  191,180,170,160,151,143,135,127,120,113,107,101, // C3 to B3
// 95, 90, 85, 80, 75, 71, 67, 63, 60, 56, 53, 50, // C4 to B4
};
#endif

uint8_t counters[NUM_TONES];
uint16_t toneenable;
uint16_t tonepulse;

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

  if ((pitch < MIDI_NOTE_START) || (pitch >= MIDI_NOTE_START+NUM_TONES)) {
    // The note is out of range for us so do nothing
    return;
  }

  toneenable |= (1<<(pitch-MIDI_NOTE_START));
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  if ((pitch < MIDI_NOTE_START) || (pitch >= MIDI_NOTE_START+NUM_TONES)) {
    // The note is out of range for us so do nothing
    return;
  }

  toneenable &= ~(1<<(pitch-MIDI_NOTE_START));
}

void setup() {
  // Set the pin directions using direct port accesses
  DDRD |= PORTDMASK;
  DDRB |= PORTBMASK;

#ifdef TESTPIN
  DDRC |= TESTMASK;
#endif
  
  // initialise the counters
  for (int i=0; i<NUM_TONES; i++) {
    counters[i] = freq[i];
  }

  // Start with all tones disabled
  toneenable = 0;
  tonepulse  = 0;

  // Use an Arduino Timer1 to generate a regular "tick"
  // to run our code every TICK microseconds.
  Timer1.initialize (TICK);
  Timer1.attachInterrupt (pulseCounter);

  // Set up the functions to be called on specific MIDI events
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // This listens to the specified MIDI channel
  MIDI.begin(MIDI_CHANNEL);
}

void loop() {
  // Handling the IO - which notes are on or off.
  // So all the loop has to do is call the MIDI read function.
  MIDI.read();
}

// This has to run quicker than the TICK is running!
// That should probably be checked with an oscilloscpe or logic analyser.
//
void pulseCounter() {
#ifdef TESTPIN
  PORTC |= TESTMASK;
#endif
  // For each tone pin, we decrement the counter associated with the pin.
  // When it reaches zero we then toggle the output pin and reset the counter.
  for (int i=0; i<NUM_TONES; i++) {
    counters[i]--;
    if (counters[i] == 0) {
      if (tonepulse & (1<<i)) {
        tonepulse &= ~(1<<i);
      } else {
        tonepulse |= (1<<i);
      }
      counters[i] = freq[i];
    }
  }

  // Write the outputs if enabled using following split:
  //   tonepulse = 0b0000 1111 1111 1111
  //      port D =             0b11 1111 00
  //      port B = 0b  00 1111 11
  uint8_t pd = ((uint16_t)(tonepulse & toneenable)<<2) & 0xFF;  // PORTD sets 0b11111100
  uint8_t pb = ((uint16_t)(tonepulse & toneenable)>>6) & 0xFF;  // PORTB sets 0b00111111

  // As PORTD pins 0 and 1 are the RX/TX pins, we don't
  // want to trash those while writing the other pins in case
  // that messes with the serial comms (i.e. the MIDI).
  //
  // I don't care about the rest of PORTB!
  //
  PORTD = (PORTD & 0x03) | pd;
  PORTB = pb;  
#ifdef TESTPIN
  PORTC &= ~TESTMASK;
#endif
}
