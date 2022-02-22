/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Toy Keyboard Tone Piano
//  https://diyelectromusic.wordpress.com/2022/02/22/toy-keyboard-tone-piano/
//
      MIT License
      
      Copyright (c) 2022 diyelectromusic (Kevin)
      
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
    Tone Melody     - https://docs.arduino.cc/built-in-examples/digital/toneMelody
    Keypad Tutorial - https://playground.arduino.cc/Main/KeypadTutorial/
*/
#include <Keypad.h>
#include "pitches.h"

//#define DBG 1  // Uncomment for debug, but don't connect RX/TX to IO if using

// This is mapped onto the keyboards LEDs
#define PLAY_LED 13
#define TONE_PIN  9

#define NONE        0

#define B_TEMPP    80
#define B_TEMPN    81
#define B_REC      82
#define B_STOP     83
#define B_PLAY     84
#define B_VOLP     85
#define B_VOLN     86
#define B_DEMOSEL  87
#define B_DEMO     88
#define B_SW2      89

#define B_DOG      90
#define B_FROG     91
#define B_DUCK     30
#define B_BIRD     31

#define B_SLOWROCK 40
#define B_ROCK     41
#define B_NEWAGE   42
#define B_DISCO    43
#define B_MARCH    44
#define B_WALTZ    45
#define B_SAMBA    46
#define B_BLUES    47

#define B_PIANO    32
#define B_ORGAN    33
#define B_VIOLIN   34
#define B_TRUMPET  35
#define B_BELL     36
#define B_MANDOLIN 37
#define B_GUITAR   38
#define B_MUSICBOX 39

#define NOTE_LOW   48
#define NOTE_HIGH  79

const byte ROWS = 8;
const byte COLS = 10;
// Set the values to be returned by the keys.
// These correspond to MIDI note numbers, but not all notes
// map onto actual keys, and any "notes" in the first few rows
// actually correspond to buttons, not keys.
// 00 = ignored
char keys[ROWS][COLS] = {
/*          BP00    BP02      BP03     BP04     BP05        BP06      BP07 11  12  13 */
/* BP20 */ {0,      B_ROCK,   B_VOLP,  0,       B_SLOWROCK, B_DEMOSEL, 79, 71, 56, 48},
/* BP21 */ {0,      B_MARCH,  B_VOLN,  B_STOP,  0,          B_GUITAR,  78, 70, 57, 49},
/* BP22 */ {0,      B_NEWAGE, 0,       B_PLAY,  B_DISCO,    B_TRUMPET, 77, 69, 58, 50},
/* BP23 */ {B_SW2,  B_SAMBA,  0,       B_REC,   B_BLUES,    0,         76, 68, 59, 51},
/* BP24 */ {B_DOG,  0,        B_TEMPP, B_DEMO,  B_VIOLIN,   0,         75, 67, 60, 52},
/* BP25 */ {B_BIRD, 0,        B_TEMPN, B_PIANO, B_BELL,     0,         74, 66, 61, 53},
/* BP26 */ {B_FROG, 0,        0,       B_ORGAN, B_MUSICBOX, 0,         73, 65, 62, 54},
/* BP27 */ {B_DUCK, 0,        0,       B_WALTZ, B_MANDOLIN, 0,         72, 64, 63, 55},
};
// ROWs = OUTPUTS; HIGH by default, set LOW individually to scan
//                  BP20,21,22,23,24,25,26,27
byte rowPins[ROWS] = { 4, 5, 6, 7, 8,10,11,12 }; // {12,11,10, 8, 7, 6, 5, 4};
// COLs = INPUTS with INPUT_PULLUP.  Switches connect them to the COLs.
//                  BP00,02,03,04,05,06,07,11,12,13
byte colPins[COLS] = {A0,A1,A2,A3,A4,A5, 1, 0, 2, 3};
Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// Set up the codes to respond to by listing the lowest note
#define NOTE_START 23   // B0
#define NUM_NOTES  89

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

int playing;

void toyNoteOn (int note) {
  if ((note >= NOTE_START) && (note < NOTE_START+NUM_NOTES)) {
    ledOn();
    tone (TONE_PIN, notes[note-NOTE_START]);
    playing = note;
  }
}

void toyNoteOff (int note) {
  if (note == playing) {
    noTone (TONE_PIN);
    ledOff();
  }
}

void ledSetup() {
  pinMode(PLAY_LED, OUTPUT);
}

void ledOn() {
  digitalWrite(PLAY_LED, LOW);
}

void ledOff() {
  digitalWrite(PLAY_LED, HIGH);
}

void setup() {
#ifdef DBG
  Serial.begin(9600);
#endif
  ledSetup();
  ledOff();
}

void loop() {
    // Fills kpd.key[ ] array with up-to 10 active keys.
    // Returns true if there are ANY active keys.
    if (kpd.getKeys())
    {
        for (int i=0; i<LIST_MAX; i++)   // Scan the whole key list.
        {
            if ( kpd.key[i].stateChanged )   // Only find keys that have changed state.
            {
                switch (kpd.key[i].kstate) {
                    case PRESSED:
#ifdef DBG
                    Serial.print(kpd.key[i].kchar,DEC);
                    Serial.println(" Pressed");
#endif
                    toyNoteOn(kpd.key[i].kchar);
                break;
                    case HOLD:
                break;
                    case RELEASED:
                    toyNoteOff(kpd.key[i].kchar);
                break;
                    case IDLE:
                break;
                }
            }
        }
    }
}  // End loop
