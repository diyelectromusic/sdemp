/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Keyboard MIDI Matrix PCB Decode
//  
//
      MIT License
      
      Copyright (c) 2023 diyelectromusic (Kevin)
      
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
    Arduino MIDI Library  - https://github.com/FortySevenEffects/arduino_midi_library
    Keypad Tutorial       - https://playground.arduino.cc/Main/KeypadTutorial/
*/
#include <MIDI.h>
#include <Keypad.h>

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// Set up the MIDI channel to send on
#define MIDI_CHANNEL 1

#define MIDI_LED LED_BUILTIN

const byte ROWS = 8;
const byte COLS = 4;
char keys[ROWS][COLS] = {
{48,54,60,66},
{49,55,61,67},
{50,56,62,68},
{51,57,63,69},
{52,58,64,70},
{53,59,65,72},
{ 0,60, 0,72},
{ 0, 0, 0, 0}
};
byte rowPins[ROWS] = {2,3,4,5,6,7,8,9}; //connect to the row pinouts of the kpd
byte colPins[COLS] = {10,11,12,13}; //connect to the column pinouts of the kpd

Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void setup() {
  MIDI.begin(MIDI_CHANNEL_OFF);
  //Serial.begin(9600);
  pinMode(MIDI_LED, OUTPUT);
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
                    digitalWrite(MIDI_LED, HIGH);
                    //Serial.print(kpd.key[i].kchar,DEC);
                    //Serial.println(" Pressed");
                    MIDI.sendNoteOn(kpd.key[i].kchar, 127, MIDI_CHANNEL);
                break;
                    case HOLD:
                break;
                    case RELEASED:
                    MIDI.sendNoteOff(kpd.key[i].kchar, 0, MIDI_CHANNEL);
                    digitalWrite(MIDI_LED, LOW);
                break;
                    case IDLE:
                break;
                }
            }
        }
    }
}  // End loop
