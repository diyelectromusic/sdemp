/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Multi Mode MIDI Step Sequencer - CDR Format
//  https://diyelectromusic.wordpress.com/2021/06/06/multi-mode-midi-step-sequencer-cdr-format-part-2/
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
    Analog Input  - https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogInput
    Digital Pins  - https://www.arduino.cc/en/Tutorial/Foundations/DigitalPins
    Random        - https://www.arduino.cc/reference/en/language/functions/random-numbers/random/
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library

    Note: This is design to be used with a Pro Mega 2560 which is recognised
          within the Arduino environment as an Arduino Mega 2560.
*/
#include <MIDI.h>

#define NUM_STEPS 8

int seqpot[NUM_STEPS] = {A14,A15,A12,A13,A10,A11,A8,A9};
int seqsw[NUM_STEPS]  = {38,39,36,37,34,35,32,33};
int seqled[NUM_STEPS] = {46,47,44,45,42,43,40,41};

#define RND_POT A5        // Set to an unused pot to seed the random number generator
int tempopot  = A0;
int octsw[2]  = {31,30};  // Up/Down switch positions
int trigsw[2] = {29,28};
int runsw[2]  = {27,26};
int swsw[2]   = {25,24};
int rndsw[2]  = {23,22};
int gatesig   = 8;
int gateout   = 6;
int gatebtn   = 10;
int midiled   = 12;

// The system supports various trigger modes, running
// modes, ranges, and operation of the switches.
//
// By default they will all start in "mode 0"
//
// Trigger Modes
#define T_FREE 0  // Free running
#define T_SEQ  1  // Gate input triggers a complete sequence
#define T_STEP 2  // Gate input triggers a single step
int trigmode;

// Run modes
#define R_NORM 0  // Normal step 1 to 8 and repeat
#define R_REV  1  // Reverse step 8 to 1 and repeat
#define R_CYCL 2  // Normal and reverse step 1 to 8 to 1 again
int runmode;

// Switch Modes
#define S_SIL  0  // Switch silences a step
#define S_SKIP 1  // Switch skips a step
#define S_RPT  2  // Switch repeats a step
#define S_RPT2 3  // Second state for the "switch repeats" option
int swmode;

//  Octave Range Modes
#define O_FULL 0  // Full octave range
#define O_FOUR 1  // Four octave range
#define O_ONE  2  // One octave range
int octmode;

//  Random Modes
#define RN_NORM 0  // Normal
#define RN_NOTE 1  // Choose a random step
#define RN_STEP 2  // Play a random note
int rndmode;

#define MIN_POT_READING 5     // Any below this is considered "zero"

// This is required to set up the MIDI library.
// Tell the MIDI library to use serial port 3,
// which is TX/D14, RX/D15 on the Mega 2560.
MIDI_CREATE_INSTANCE(HardwareSerial, Serial3, MIDI);

int midiChannel = 1;  // Define which MIDI channel to transmit on (1 to 16).

int numNotes;
int playingNote;
int lastNote;
int runDir=1;
int lastgatesig;
int lastgatebtn;
int gatebtndbcnt;
int triggered;

// Set the MIDI codes for the notes to be played
// The "Octave Range" mode will determine where in this list we start
#define O_FOUR_START  36  // Note: there is no checking that there are enough notes after this...
#define O_ONE_START   48
int notes[] = {
9,10,11,12,  13,14,15,16, 17,18,19,20,  // A- to G#0
21,22,23,24, 25,26,27,28, 29,30,31,32,  // A0 to G#1
33,34,35,36, 37,38,39,40, 41,42,43,44,  // A1 to G#2
45,46,47,48, 49,50,51,52, 53,54,55,56,  // A2 to G#3
57,58,59,60, 61,62,63,64, 65,66,67,68,  // A3 to G#4
69,70,71,72, 73,74,75,76, 77,78,79,80,  // A4 to G#5
81,82,83,84, 85,86,87,88, 89,90,91,92,  // A5 to G#6
93,94,95,96, 97,98,99,100, 101,102,103,104, // A6 to G#7
105,106,107,108, 109,110,111,112, 113,114,115,116, // A7 to G#8
};

int ctrlscan;
void controlScan () {
  // Scan the switches
  ctrlscan++;
  if (ctrlscan == 1) {
    // Trigger switches    
    int sw1 = digitalRead(trigsw[0]);
    int sw2 = digitalRead(trigsw[1]);
    if (!sw1) {
      trigmode = T_SEQ;
    } else if (!sw2) {
      trigmode = T_STEP;
    } else {
      trigmode = T_FREE;
    }
  } else if (ctrlscan == 2) {
    // Run mode switches
    int sw1 = digitalRead(runsw[0]);
    int sw2 = digitalRead(runsw[1]);
    if (!sw1) {
      runmode = R_REV;
    } else if (!sw2) {
      runmode = R_CYCL;
    } else {
      runmode = R_NORM;
    }
  } else if (ctrlscan == 3) {
    // Switch mode switches
    int sw1 = digitalRead(swsw[0]);
    int sw2 = digitalRead(swsw[1]);
    if (!sw1) {
      swmode = S_SKIP;
    } else if (!sw2) {
      // If not already in "playing second repeat" mode
      // then set to repeat mode
      if (swmode != S_RPT2) {
        swmode = S_RPT;
      }
    } else {
      swmode = S_SIL;
    }
  } else if (ctrlscan == 4) {
    // Octave range mode switches
    int sw1 = digitalRead(octsw[0]);
    int sw2 = digitalRead(octsw[1]);
    if (!sw1) {
      octmode = O_FOUR;
    } else if (!sw2) {
      octmode = O_ONE;
    } else {
      octmode = O_FULL;
    }
  } else if (ctrlscan == 5) {
    // RND mode switches
    int sw1 = digitalRead(rndsw[0]);
    int sw2 = digitalRead(rndsw[1]);
    if (!sw1) {
      rndmode = RN_NOTE;
    } else if (!sw2) {
      rndmode = RN_STEP;
    } else {
      rndmode = RN_NORM;
    }
  } else {
    ctrlscan = 0;
  }
}

// Define a function to control the tempo.
// We want more control of the faster tempos, so
// split the ranges of the potentiometer up and
// return different scalings depending on the readings.
//
// Note that we are returning a delay value (in milliseconds),
// so 5000 means every 5 seconds, i.e. 12 bpm
// and 50 means every 50 mS or 20 times a second, i.e. 1200 bpm.
//
int tempomap (int i) {
  if (i<256) {
    // Slowest range of tempos: 5000 to 2000
    return map (i, 0, 256, 5000, 2000);
  } else if (i<512) {
    // Mid range of tempos: 2000 to 800
    return map (i, 256, 512, 2000, 800);
  } else if (i<768) {
    // Next mid range of tempos: 800 to 300
    return map (i, 512, 768, 800, 300);
  } else {
    // Fastest range 300 to 50
    return map (i, 768, 1023, 300, 50);
  }
}

void midiLedOn () {
  digitalWrite(midiled, HIGH);
}

void midiLedOff () {
  digitalWrite(midiled, LOW);
}

void setup() {
  // This is a programming trick to work out the number of notes we've listed
  numNotes = sizeof(notes)/sizeof(notes[0]);

  randomSeed (analogRead(RND_POT));

  for (int i=0; i<NUM_STEPS; i++) {
    pinMode(seqsw[i],INPUT_PULLUP);
    pinMode(seqled[i],OUTPUT);
    digitalWrite(seqled[i],LOW);
  }

  pinMode(swsw[0], INPUT_PULLUP);
  pinMode(swsw[1], INPUT_PULLUP);
  pinMode(runsw[0], INPUT_PULLUP);
  pinMode(runsw[1], INPUT_PULLUP);
  pinMode(trigsw[0], INPUT_PULLUP);
  pinMode(trigsw[1], INPUT_PULLUP);
  pinMode(octsw[0], INPUT_PULLUP);
  pinMode(octsw[1], INPUT_PULLUP);
  pinMode(rndsw[0], INPUT_PULLUP);
  pinMode(rndsw[1], INPUT_PULLUP);
  pinMode(gatebtn, INPUT_PULLUP);
  pinMode(gatesig, INPUT);
  pinMode(gateout, OUTPUT);
  digitalWrite(gateout, LOW);
  pinMode(midiled, OUTPUT);
  midiLedOff();

  playingNote = 0;

  MIDI.begin(MIDI_CHANNEL_OFF);
}

void loop() {

  // Scan the general controls
  controlScan();

  // General principle of operation:
  //   Loop through each note in the sequence in turn,
  //   playing the note defined by the potentiomer for
  //   that point in the sequence.

  // Check for triggering conditions.
  // Gate signal is active high; gate button is active low.
  // Trigger happens on transition from inactive to active.
  //
  // If we are not in the right place of the sequence for
  // the trigger, then I just count them up so it will
  // trigger straight away when required.
  //
  // As each step is associted with a delay related to the tempo,
  // I also don't worry about debouncing the button.
  //
  int gate_b = digitalRead(gatebtn);
  int gate_s = digitalRead(gatesig);
  if (trigmode != T_FREE) {
    // Check the gate button with some debouncing
    if (gate_b == HIGH) {
      // No switch pressed yet
      gatebtndbcnt = 0;
      lastgatebtn = HIGH;
    } else {
      gatebtndbcnt++;
    }
    // Note: If want repeated triggering if the button is held, then
    //       remove lastgatebtn test here and only check gatebtndbcnt.
    if ((lastgatebtn == HIGH) && (gatebtndbcnt >= 1000)) {
      gatebtndbcnt=0;
      triggered++;
      lastgatebtn = LOW;
    }

    // Not doing anything with the gate signal input yet
    lastgatesig = gate_s;
  }

  // Turn off any playing notes
  if (lastNote != -1) {
    midiLedOff();
    MIDI.sendNoteOff(notes[lastNote], 0, midiChannel);
    lastNote = -1;
  }

  if ((trigmode == T_STEP) && !triggered) {
    // Do not proceed to the next step if there is no trigger.
    return;
  }

  if ((trigmode == T_SEQ) && !triggered && (
         ((playingNote == 0) && (runmode != R_REV) && (swmode != S_RPT2)) ||
         ((playingNote == NUM_STEPS-1) && (runmode == R_REV) && (swmode != S_RPT2))
       )) {
    // Do not run if we are back to note 0 (or the last note if in reverse mode),
    // and have no trigger - unless we are on "repeat two" of repeated playing!
    return;
  }

  // Reset the trigger condition
  triggered = 0;

  // Now process the current step in the sequence

  // Check enable switches.
  //
  // What we do depends on the switch mode!
  // Although if the switch is not active, then we play every time anyway.
  // NB: stepsw = True if not switched.
  //     stepsw = False if switched.
  // So if (!stepsw) test for the switch being activated.
  //
  int stepsw = digitalRead(seqsw[playingNote]);
  if (stepsw || (swmode != S_SKIP)) {
    // We have a step to play, but "what" we play
    // depends on the switch setting.

    // Indicate the step in the sequence
    digitalWrite(seqled[playingNote], HIGH);
  
    // Take the reading for the pot related to this step
    int potReading = analogRead (seqpot[playingNote]);

    if (rndmode == RN_NOTE) {
      // Add a random element to the reading to vary the note.
      // But ensure it is still within the 0-1023 range
      potReading = potReading - 100 + random (200);
      if (potReading > 1023) potReading = 1023;
      if (potReading < 0) potReading = 0;
    }
  
    // if the reading is zero (or almost zero), or the
    // mode switch is switched and we are in "silent mode",
    // then indicate no notes are playing and skip the "playing" bit.
    if ((potReading < MIN_POT_READING) || (!stepsw && (swmode == S_SIL))) {
      lastNote = -1;
    } else {
      // This is a special function that will take one range of values,
      // in this case the analogue input reading between 0 and 1023, and
      // produce an equivalent point in another range of numbers, in this
      // case choose one of the notes in our list.
      int playing;
      if (octmode == O_ONE) {
        playing = map(potReading, MIN_POT_READING, 1023, O_ONE_START, O_ONE_START+12);
      } else if (octmode == O_FOUR) {
        playing = map(potReading, MIN_POT_READING, 1023, O_FOUR_START, O_FOUR_START+48);
      } else {
        // full range
        playing = map(potReading, MIN_POT_READING, 1023, 0, numNotes-1);
      }
  
      // If we are already playing a note then we need to turn it off first
      if (lastNote != -1) {
          midiLedOff();
          MIDI.sendNoteOff(notes[lastNote], 0, midiChannel);
      }
      midiLedOn();
      MIDI.sendNoteOn(notes[playing], 127, midiChannel);
      lastNote = playing;
    }
  }


  // If the step switch is active and we are in skip mode
  // then no delay - go straight onto the next step.
  if (!stepsw && (swmode == S_SKIP)) {
    // Skip this step
  } else {
    // Move on to the next step after a short delay determined by
    // the tempo potentiometer.
    //
    // Note: 250 = 1/4 of a second which is same as 240 beats per minute.
    //
    // UNLESS we are in "step trigger mode" in which case, there is just
    // a short delay as everything is controlled by the trigger.
    //
    if (trigmode == T_STEP) {
      // short delay to allow the LED to show
      delay(100);
    } else {
      int tempodelay = tempomap(analogRead(tempopot));
      delay(tempodelay);
    }
  }
  digitalWrite(seqled[playingNote], LOW);

  // If we are in "switch mode repeat" and the switch is switched,
  // then don't increment the step, instead play it twice.
  // Unless this is the second time through, in which case, we do
  // increment it and reset the swmode.
  if ((swmode == S_RPT) && !stepsw) {
    // Set things up for the second pass, but don't increment the step.
    swmode = S_RPT2;
  } else {
    // Clear the repeat indicator if necessary, and then move onto the next step
    if (swmode == S_RPT2) {
      swmode = S_RPT; 
    }

    // Now update the running note, depending on our running mode.
    if (runmode == R_NORM) {
      runDir = 1;
    } else if (runmode == R_REV) {
      runDir = -1;
    } else if (runmode == R_CYCL) {
      if (playingNote == 0) {
        // Count up again
        runDir = 1;
      } else if (playingNote == (NUM_STEPS-1)) {
        // Count down again
        runDir = -1;
      } else {
        // nothing to change - leave runDir as it is
      }
    } else {
      // Invalid mode...
    }
    if (rndmode == RN_STEP) {
      // Pick the next step at random
      playingNote = random (0,NUM_STEPS);
    } else {
      playingNote += runDir;
    }
    if (playingNote >= NUM_STEPS) {
      playingNote = 0;
    } else if (playingNote < 0) {
      playingNote = NUM_STEPS-1;
    } else {
      // All good.  Carry on.
    }
  }
}
