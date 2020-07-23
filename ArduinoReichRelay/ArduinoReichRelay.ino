/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Reich Relay
//  https://diyelectromusic.wordpress.com/2020/06/03/arduino-reich-relay/
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

#define TEST 1

// Code uses consecutive digital pins in PORTD
// Arduino pins D0 to D7 correspond to PD0 to PD7
// We use digital pins 2 to 6 for our five relays.
// Which correspond to bits 2 to 6 in PORTD
#define CHANNELS 5
#define RLY1     0b00000100
#define RLY2     0b00001000
#define RLY3     0b00010000
#define RLY4     0b00100000
#define RLY5     0b01000000
#define ALLRLYS  (RLY1|RLY2|RLY3|RLY4|RLY5)

// Set the tempo for the piece.  This needs to be the most fundamental "beat"
// of the music in milliseconds.
// So, for a tempo of 120 crotchets per minute, but with music containing
// a granularity of quavers, we need a fundamental beat of 240 quavers a minute
// which equates to one quaver every 60000/240 milliseconds or every 250 milliseconds.
//
// Steve Reich Music for Pieces of Wood
// Defined in quavers, for crotchet = 192-216.
// Go with 1 quaver = 1/400 of a minute or approx
// every 150 mS
#define TEMPO_MILLIS 150

// Store the patterns for the relays to be used
// on each tick.  If the bit is set the relay needs
// to be toggled to make it "play".
// This uses the lowest 5 bits to represent each relay.
#ifdef TEST
const PROGMEM uint8_t score[] = {
  0b00001, 0b00010, 0b00100, 0b01000, 0b10000,
};
#else
#include "Reich.h"
#endif

int note;
int numNotes;

void setup() {
  // put your setup code here, to run once:

  // Set the relay pins as outputs
  DDRD |= ALLRLYS;

  // Set the outputs LOW
  PORTD &= ~ALLRLYS;

  numNotes = sizeof (score)/sizeof(score[0]);
  note = 0;

  delay (2000);
}

void loop() {
  // put your main code here, to run repeatedly:

  // This is the basic "beat" of the music
  delay (TEMPO_MILLIS);

  // Work out which relays to "play" on this beat.
  // Start with the current settings of all relays.
  uint8_t portdvalue = PIND & ALLRLYS;
  uint8_t nextnote = pgm_read_byte_near(score + note);
  if (nextnote & 0x1) {
    // Play relay 1 by toggling its value
    if (portdvalue & RLY1) {
      portdvalue &= ~RLY1;
    } else {
      portdvalue |= RLY1;
    }
  }
  if (nextnote & 0x2) {
    // Play relay 2
    if (portdvalue & RLY2) {
      portdvalue &= ~RLY2;
    } else {
      portdvalue |= RLY2;
    }
  }
  if (nextnote & 0x4) {
    // Play relay 3
    if (portdvalue & RLY3) {
      portdvalue &= ~RLY3;
    } else {
      portdvalue |= RLY3;
    }
  }
  if (nextnote & 0x8) {
    // Play relay 4
    if (portdvalue & RLY4) {
      portdvalue &= ~RLY4;
    } else {
      portdvalue |= RLY4;
    }
  }
  if (nextnote & 0x10) {
    // Play relay 5
    if (portdvalue & RLY5) {
      portdvalue &= ~RLY5;
    } else {
      portdvalue |= RLY5;
    }
  }

  // Now output the value to "play" the required relays.
  // First add in the state of the pins we arne't using.
  portdvalue |= (PIND & ~ALLRLYS);
  PORTD = portdvalue;

  note++;
  if (note >= numNotes) {
    // We've reached the end of the piece
    delay (30000);
    note = 0;
  }
}
