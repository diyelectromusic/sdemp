/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Relay Bolero
//  https://diyelectromusic.wordpress.com/2020/06/02/arduino-relay-bolero/
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
const int relayPin = 5;

// Set up the durations for the range of notes.
//
// These values are passed to the Arduino delay() function
// which delays the running of the code for the requested number
// of milliseconds before continuing the programme.
//
// As each note is a single click, there is no concept of
// a rest required.
//
#define n_SQ 112     // semiquaver
#define n_Q  225     // quaver
#define n_TQ 150     // triplet quaver
#define n_C  450     // crotchet
#define n_M  900     // minim
#define n_SB 1800    // semibreve


int notes[] = {
/* Edit this array below to change the rhythm */

  n_C,
  n_TQ, n_TQ, n_TQ,
  n_C,
  n_TQ, n_TQ, n_TQ,
  n_C,
  n_C,
  n_C,
  n_TQ, n_TQ, n_TQ,
  n_C,
  n_TQ, n_TQ, n_TQ,
  n_TQ, n_TQ, n_TQ,
  n_TQ, n_TQ, n_TQ,

/* Stop editing here - make sure each note is followed by a comma */
};

int relayValue;

void setup() {
  // Set up our I/O pin as an output
  relayValue = LOW;
  pinMode (relayPin, OUTPUT);
  digitalWrite (relayPin, relayValue);

  // A short 5 second pause before starting playing rhythm
  delay (5000);
}

void loop() {
  int i;
  int numNotes = sizeof(notes)/sizeof(notes[0]); // A programming trick to work out how many notes have been listed

  // This cycles through each note in our array.
  for (i=0; i<numNotes; i++) {
 
    // We don't mind if the relay is opening or closing - 
    // we just want a change to make a sound.  In fact, if we
    // have to open then close for each note, we'd end up with
    // two clicks in quick successfion, which is messy.
    //
    // So for this code, we just toggle the state of
    // of the relay, which happens here.
    relayValue = !relayValue;
    
    // Trigger the relay to make our "click".
    digitalWrite (relayPin, relayValue);
  
    // Then wait for the required delay before moving on to the next note.
    delay (notes[i]);
  }
}
