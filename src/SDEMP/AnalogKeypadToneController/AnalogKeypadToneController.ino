/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Analog Keypad Tone Controller
//  https://diyelectromusic.wordpress.com/2021/06/16/analog-keypad-tone-controller/
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
    Arduino Tone Melody  - https://www.arduino.cc/en/Tutorial/BuiltInExamples/toneMelody
    Arduino Analog Input - https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogInput
    Read Multiple Switches using ADC - https://www.edn.com/read-multiple-switches-using-adc/
*/

#define TONEPIN 9

// The following analog values are the suggested values
// for each switch (as printed on the boards).
//
// These are given assuming a 5V ALG swing and a 1024
// full range reading.
//
// Note: On my matrix, the switches were numbered
//       1,2,3,
//       4,5,6,
//       7,8,9,
//      10,0,11
//
//       But I'm keeping it simple, and numbering the
//       last row more logically as 10,11,12.
//
#define NUM_BTNS 12
#define ALG_ERR 8
int algValues[NUM_BTNS] = {
   1023,930,850,
   790,730,680,
   640,600,570,
   540,510,490
};

int lastbtn1, lastbtn2;

int tones[NUM_BTNS*2] = {
  110, 117, 123, 131, 139, 147, 156, 165,
  175, 185, 196, 208, 220, 233, 247, 262,
  277, 294, 311, 330, 349, 370, 392, 415,  
};

uint8_t readSwitches (int pin) {
  int val = analogRead (pin);

  // As the exact value may change slightly due to errors
  // or inaccuracies in the readings, we take the calculated
  // values and look for a range of values either side.
  //
  // To save having to have lots of "if >MIN and <MAX" statements
  // I just start at the highest and look for matches and if found, stop.
  //
  // Count down from highest to lowest, but note that
  // we don't handle "0" in the loop - that is left as a default
  // if nothing else is found...
  //
  // Note that this doesn't really cope very well with
  // multiple buttons being pressed, so I don't even bother
  // here, it just returns the highest matching button.
  //
  for (int i=0; i<NUM_BTNS; i++) {
    // Start at the highest and count backwards
    if (val  > (algValues[i] - ALG_ERR)) {
      // We have our match
      // Buttons are numbered 1 to NUM_BTNS
      return i+1;
    }
  }

  // If we get this far, we had no match further up the list
  // so the default position is to assume no switches pressed.
  return 0;
}

void setup() {
   Serial.begin(9600);
   Serial.println("Analog Switch MIDI Controller Test Reader");
}

void loop() {

  // Read the first keypad
  uint8_t btn1 = readSwitches(A0);
  if (btn1 != lastbtn1) {
    if (btn1 != 0) {
      Serial.print("MIDI Button 1: ");
      Serial.println(btn1, DEC);
      tone(TONEPIN,tones[btn1-1]);
    }    
  }
  lastbtn1 = btn1;

  // Read the second keypad
  uint8_t btn2 = readSwitches(A1);
  if (btn2 != lastbtn2) {
    if (btn2 != 0) {
      Serial.print("MIDI Button 2: ");
      Serial.println(btn2, DEC);
      tone(TONEPIN,tones[btn2-1+12]);
    }
  }
  lastbtn2 = btn2;

  // If no buttons are pressed, kill the tones
  if (btn1==0 && btn2==0) {
    noTone(TONEPIN);
  }

  delay(200);
}
