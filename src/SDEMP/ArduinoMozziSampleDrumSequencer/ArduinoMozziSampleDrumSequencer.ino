/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Mozzi Sample Drum Sequencer
//  https://diyelectromusic.wordpress.com/2021/06/23/arduino-mozzi-sample-drum-sequencer/
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
    Arduino Digital Input Pullup - https://www.arduino.cc/en/Tutorial/DigitalInputPullup
    Mozzi Library        - https://sensorium.github.io/Mozzi/
    Arduino Analog Input - https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogInput
    Read Multiple Switches using ADC - https://www.edn.com/read-multiple-switches-using-adc/

  Some of this code is based on the Mozzi example "Sample" (C) Tim Barrass

*/
#include <MozziGuts.h>
#include <Sample.h> // Sample template
#include "d_kit.h"

// Uncomment to output max/min values of the samples for calibration purposes
//#define CALIBRATE 1

// This defines the size of the combined output value from summing
// all the samples together.  The worst case has to allow for adding
// up +/- 127 for each sample, but a more accurate estimation can be
// obtained by running the "CALIBRATE" mode above which will output
// the maximum and minimum values for each sample, then you just need
// to cope with a "bit depth" that supports the worst cases.
//
// Example: If the output of the CALIBRATE function is:
// Data for drum on pin 5: Max: 103 Min: -128
// Data for drum on pin 4: Max: 95 Min: -128
// Data for drum on pin 3: Max: 127 Min: -121
// Data for drum on pin 2: Max: 98 Min: -128
//
// Then the worst case - i.e. all peaks occuring at the same time:
//    Positive range: 103+95+127+98 = 423
//    Negative range: -106-114-19-13 = -505
//
// So allowing for +/-512 in the output is plenty.  This is satisfied
// with a 10-bit signed value, so we use an OUTPUTSCALING of 10 bits.
//
// The most likely case though is that all samples won't play at the same
// time, and almost certainly not with their peaks together, so in reality
// you could probably get away with less anyway.
//
#define OUTPUTSCALING 9

#define POT_KP1   A0
#define POT_KP2   A1
//#define POT_TEMPO A2

#define D_NUM 4
#define D_BD  1
#define D_SD  2
#define D_CH  3
#define D_OH  4
int drums[D_NUM] = {D_BD,D_SD,D_OH,D_CH};

// use: Sample <table_size, update_rate> SampleName (wavetable)
Sample <BD_NUM_CELLS, AUDIO_RATE> aBD(BD_DATA);
Sample <SD_NUM_CELLS, AUDIO_RATE> aSD(SD_DATA);
Sample <CH_NUM_CELLS, AUDIO_RATE> aCH(CH_DATA);
Sample <OH_NUM_CELLS, AUDIO_RATE> aOH(OH_DATA);

#define TRIG_LED LED_BUILTIN
#define LED_ON_TIME 300 // On time in mS
bool trig[D_NUM];

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

#define BEATS 6
#define DRUMS D_NUM
uint8_t pattern[BEATS][DRUMS];
unsigned long nexttick;
int seqstep;
uint16_t tempo  = 120;
int loopstate;

// Map positions in the DRUM/BEAT pattern matrix onto key presses
uint8_t keys[2][NUM_BTNS] = {
  0x14,0x24,0x34, // Drum 4, beats 1,2,3
  0x13,0x23,0x33, // Drum 3, beats 1,2,3
  0x12,0x22,0x32, // Drum 2, beats 1,2,3
  0x11,0x21,0x31, // Drum 1, beats 1,2,3
  0x44,0x54,0x64, // Drum 4, beats 4,5,6
  0x43,0x53,0x63, // Drum 3, beats 4,5,6
  0x42,0x52,0x62, // Drum 2, beats 4,5,6
  0x41,0x51,0x61, // Drum 1, beats 4,5,6
};

uint8_t readSwitches (int pin) {
  int val = mozziAnalogRead (pin);

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

#ifdef CALIBRATE
void calcMaxMin (int drum) {
  int8_t max=0;
  int8_t min=0;
  int num_cells;
  int8_t *pData;
  switch (drum) {
    case D_BD:
       num_cells = BD_NUM_CELLS;
       pData = (int8_t *)&BD_DATA[0];
       break;
    case D_SD:
       num_cells = SD_NUM_CELLS;
       pData = (int8_t *)&SD_DATA[0];
       break;
    case D_CH:
       num_cells = CH_NUM_CELLS;
       pData = (int8_t *)&CH_DATA[0];
       break;
    case D_OH:
       num_cells = OH_NUM_CELLS;
       pData = (int8_t *)&OH_DATA[0];
       break;
    default:
       Serial.print("Unknown drum on calibration\n");
       return;
  }
  for (int i=0; i<num_cells; i++) {
    int8_t val = pgm_read_byte_near (pData+i);
    if (val > max) max = val;
    if (val < min) min = val;
  }
  Serial.print("Data for drum on pin ");
  Serial.print(drum);
  Serial.print(":\tMax: ");
  Serial.print(max);
  Serial.print("\tMin: ");
  Serial.println(min);  
}
#endif

void startDrum (int drum) {
  switch (drum) {
    case D_BD: aBD.start(); break;
    case D_SD: aSD.start(); break;
    case D_CH: aCH.start(); break;
    case D_OH: aOH.start(); break;
  }
}

unsigned long millitime;
void ledOff () {
  if (millitime < millis()) {
     digitalWrite(TRIG_LED, LOW);
  }
}

void ledOn () {
  millitime = millis() + LED_ON_TIME;
  digitalWrite(TRIG_LED, HIGH);
}

void setup () {
  pinMode(TRIG_LED, OUTPUT);
  ledOff();
  startMozzi();
  // Initialise all samples to play at the speed it was recorded
  aBD.setFreq((float) D_SAMPLERATE / (float) BD_NUM_CELLS);
  aSD.setFreq((float) D_SAMPLERATE / (float) SD_NUM_CELLS);
  aCH.setFreq((float) D_SAMPLERATE / (float) CH_NUM_CELLS);
  aOH.setFreq((float) D_SAMPLERATE / (float) OH_NUM_CELLS);

#ifdef CALIBRATE
  Serial.begin(9600);
  calcMaxMin (D_BD);
  calcMaxMin (D_SD);
  calcMaxMin (D_CH);
  calcMaxMin (D_OH);
#endif
}

int drumScan;
void updateControl() {
  ledOff();

  // Only play the note in the pattern if we've met the criteria for
  // the next "tick" happening.
  unsigned long timenow = millis();
  if (timenow >= nexttick) {
    // Start the clock for the next tick.
    // Take the tempo mark as a "beats per minute" measure...
    // So time (in milliseconds) to the next beat is:
    //    1000 * 60 / tempo
    //
    // This assumes that one rhythm position = one beat.
    //
    nexttick = millis() + (unsigned long)(60000/tempo);

    // Now play the next beat in the pattern
    seqstep++;
    if (seqstep >= BEATS){
      seqstep = 0;
      // Flash the LED every cycle
      ledOn();
    }
    for (int d=0; d<DRUMS; d++) {
      if (pattern[seqstep][d] != 0) {
        startDrum(pattern[seqstep][d]);
      }
    }
  }

  // Take it in turns each loop depending on the value of "loopstate" to:
  //    0 - read the first keypad
  //    1 - read the second keypad
  //    2 - read the tempo pot
  //
  if (loopstate == 0) {
    // Read the first keypad
    uint8_t btn1 = readSwitches(POT_KP1);
    if (btn1 != lastbtn1) {
      if (btn1 != 0) {
        // A keypress has been detected.
        // The key values are encoded in the form:
        //    <beat><drum> HEX
        // Where <beat> is from 1 to BEATS
        //   and <drum> is from 1 to DRUMS
        //   in hexadecimal.
        //
        // This means the beat index and drum index
        // can be pulled directly out of the key value.
        //
        uint8_t key = keys[0][btn1-1];
        int drumidx = (key & 0x0F) - 1;
        int beatidx = (key >> 4) - 1;
  
        // So toggle this drum
        if (pattern[beatidx][drumidx] != 0) {
          pattern[beatidx][drumidx] = 0;
        } else {
          pattern[beatidx][drumidx] = drums[drumidx];
        }
      }    
    }
    lastbtn1 = btn1;
  }
  else if (loopstate == 1) {
    // Repeat for the second keypad
    uint8_t btn2 = readSwitches(POT_KP2);
    if (btn2 != lastbtn2) {
      if (btn2 != 0) {
        byte key = keys[1][btn2-1];
        int drumidx = (key & 0x0F) - 1;
        int beatidx = (key >> 4) - 1;

        // So toggle this drum
        if (pattern[beatidx][drumidx] != 0) {
          pattern[beatidx][drumidx] = 0;
        } else {
          pattern[beatidx][drumidx] = drums[drumidx];
        }
      }    
    }
    lastbtn2 = btn2;
  }
  else if (loopstate == 2) {
#ifdef POT_TEMPO
    // Read the potentiometer for a value between 0 and 255.
    // This will be converted into a delay to be used to control the tempo.
    int pot1 = 20 + (mozziAnalogRead (POT_TEMPO) >> 2);
    if (pot1 != tempo) {
      tempo = pot1;  // Tempo range is 20 to 275.
    }
#endif
  }
  loopstate++;
  if (loopstate > 2) loopstate = 0;
}

AudioOutput_t updateAudio(){
  // Need to add together all the sample sources.
  // We down-convert using the scaling factor worked out
  // for our specific sample set from running in "CALIBRATE" mode.
  //
  int16_t d_sample = aBD.next() + aSD.next() + aCH.next() + aOH.next();
  return MonoOutput::fromNBit(OUTPUTSCALING, d_sample);
}

void loop(){
  audioHook();
}
