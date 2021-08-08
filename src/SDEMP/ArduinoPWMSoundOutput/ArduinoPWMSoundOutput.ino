/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino PWM Sound Output
//  https://diyelectromusic.wordpress.com/2021/07/27/arduino-pwm-sound-output/
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
    Secrets of Arduino PWM - https://www.arduino.cc/en/Tutorial/SecretsOfArduinoPWM
    Arduino 101 Timers and Interrupts - https://www.robotshop.com/community/forum/t/arduino-101-timers-and-interrupts/13072
    OpenMusic Labs PWM DAC - http://www.openmusiclabs.com/learning/digital/pwm-dac.1.html
    Direct Digital Synthesis - https://www.electronics-notes.com/articles/radio/frequency-synthesizer/dds-direct-digital-synthesis-synthesizer-what-is-basics.php
    "Audio Output and Sound Synthesis" from "Arduino for Musicians" Brent Edstrom
*/

#define FREQPOT A0

// Choose the wave you want:
//   0 Sine
//   1 Saw
//   2 Triangle
//   3 Square
#define WAVE 0

// Direct Digital Synthesis relies on an "accumulator" to index
// into a wavetable to read out samples for writing to the audio
// output circuitry.
//
// The accumulator is a 16 bit value, but we only need
// to refer to one of 256 values in the wavetable.  However
// the calculations involving the frequency often turn up decimals
// which are awkward to handle in code like this (and you lose accuracy).
//
// One way round this is to use "fixed point maths" where
// we take, say, a 16 bit value and use the top 8 bits
// for the whole number part and the low 8 bits for the
// decimal part.
//
// This is what I do with the accumulator.  This means
// that when it comes to actually using the accumulator
// to access the wavetable, it is 256 times bigger than
// we need it to be, so will always use (accumulator>>8)
// to calculate the index into the wavetable.
//
// This needs to be taken into account when calculating
// accumulator increments which will be using the formulas
// discussed above, but with several pre-set constant values.
//
// As we are using constant values involving 256 and a sample
// rate of 32768 it is very convenient to pre-calculate the
// values to be used in the frequency to increment calculation,
// especially making use of the 8.8 fixed point maths we've already mentioned.
//
// Recall:
//     Inc = freq * wavetable size / Sample rate
//
// We also want this 256 times larger to support the 8.8 FP maths.
// So for a sample rate of 32768 and wavetable size of 256
//     Inc = 256 * freq * 256 / 32768
//     Inc = freq * 65535/32768
//     Inc = freq * 2
//
// This makes calculating the (256 times too large) increment
// from the frequency pretty simple.
//
#define FREQ2INC(freq) (freq*2)
uint16_t accumulator;
uint16_t lastaccumulator;
uint16_t frequency;
uint16_t nextfrequency;
uint8_t  wave;
int      playnote;

//NOTE: If you change the size of the wavetable, then the setWavetable
//      function will need re-writing.
#define WTSIZE 256
unsigned char wavetable[WTSIZE];

// This is the only wave we set up "by hand"
// Note: this is the first half of the wave,
//       the second half is just this mirrored.
const unsigned char PROGMEM sinetable[128] = {
    0,   0,   0,   0,   1,   1,   1,   2,   2,   3,   4,   5,   5,   6,   7,   9,
   10,  11,  12,  14,  15,  17,  18,  20,  21,  23,  25,  27,  29,  31,  33,  35,
   37,  40,  42,  44,  47,  49,  52,  54,  57,  59,  62,  65,  67,  70,  73,  76,
   79,  82,  85,  88,  90,  93,  97, 100, 103, 106, 109, 112, 115, 118, 121, 124,
  128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170, 173,
  176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215,
  218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244,
  245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255,
};


void initPwm() {
  // Initialise Timer 1 as follows:
  //    Output on pin 9 (OC1A).
  //    Run at CPU clock speed (16MHz).
  //    Use PWM Fast mode (count up then reset).
  //    Use ICR1 as the TOP value.
  //    TOP value = (clock / (prescaler * 32768))-1 = 487
  //    PWM value is updated by writing to OCR1A
  //    Interrupt enabled for overflow: TIMER1_OVF_vect
  //
  // So set up PWM for Timer 1, Output A:
  //   WGM1[3:0] = 1110 = FastPWM; 16-bit; ICR1 as TOP; TOV1 set at TOP
  //   COM1A[1:0] = 10 = Clear OC1A on cmp match(up); Set OC1A at BOTTOM
  //   CS1[2:0] = 001 = No prescalar
  //   TOIE1 = Timer/Counter 1 Overflow Interrupt Enable
  //
  pinMode(9, OUTPUT);
  TCCR1A = _BV(COM1A1) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
  TCCR1C = 0;
  ICR1 = 487;

  // Check PWM output is active
  OCR1A = 487/4;  // 25% duty cycle test

  // 5 seconds wait to output test PWM signal before enabling the interrupts
  delay (5000);

  TIMSK1 = _BV(TOIE1);
}

void setPwm (uint8_t pwmval) {
  OCR1A = pwmval;
}

ISR(TIMER1_OVF_vect) {
  ddsOutput();
}

void setup() {
  // Set up the wavetable
  setWavetable(WAVE);

  // Dump the wavetable to the serial plotter
  Serial.begin(9600);
  for (int i=0; i<WTSIZE; i++) {
    Serial.println(wavetable[i]);
  }
  
  initPwm();

  wave = 0;
  accumulator = 0;
  lastaccumulator = 0;
  frequency = 0;

  nextfrequency = 440;
  delay(5000); // 5 seconds to output a test 440Hz tone ("concert A").
}

void loop() {
  // Use an analog input to set the frequency.
  // This will choose a frequency between 220 (A3) and 1243 Hz (approx D#6).
  nextfrequency = 220+analogRead(FREQPOT);
}

// This is the code that is run on every "tick" of the timer.
void ddsOutput () {
  // Output the last calculated value first to reduce "jitter"
  // then go on to calculate the next value to be used.
  setPwm(wave);

  // Avoid jumps part way through the waveform on change of frequency.
  // Only update the frequency when the accumulator wraps around.
  if (nextfrequency != frequency) {
    if ((accumulator == 0) || (lastaccumulator > accumulator)) {
      frequency = nextfrequency;
      accumulator = 0;
    } else if (frequency == 0) {
      // Exception - if the frequency is already zero then
      // can move to a new frequency straight away as long as we
      // reset the accumulator.
      frequency = nextfrequency;
      accumulator = 0;
    }
  }
  lastaccumulator = accumulator;

  // Recall that the accumulator is as 16 bit value, but
  // representing an 8.8 fixed point maths value, so we
  // only need the top 8 bits here.
  wave = wavetable[accumulator>>8];
  accumulator += (FREQ2INC(frequency));
}

// This function will initialise the wavetable
void setWavetable (int wavetype) {
  if (wavetype == 0) {
    // Sine wave
    for (int i = 0; i < 128; i++) {
      // Read out the first half the wave directly
      wavetable[i] = pgm_read_byte_near(sinetable + i);
    }
    wavetable[128] = 255;  // the peak of the wave
    for (int i = 1; i < 128; i++) {
      // Need to read the second half in reverse missing out
      // sinetable[0] which will be the start of the next one
      wavetable[128 + i] = pgm_read_byte_near(sinetable + 128 - i);
    }
  }
  else if (wavetype == 1) {
    // Sawtooth wave
    for (int i = 0; i < 256; i++) {
      wavetable[i] = i;
    }
  }
  else if (wavetype == 2) {
    // Triangle wave
    for (int i = 0; i < 128; i++) {
      // First half of the wave is increasing from 0 to almost full value
      wavetable[i] = i * 2;
    }
    for (int i = 0; i < 128; i++) {
      // Second half is decreasing from full down to just above 0
      wavetable[128 + i] = 255 - (i*2);
    }
  }
  else { // Default also if (wavetype == 3)
    // Square wave
    for (int i = 0; i < 128; i++) {
      wavetable[i] = 255;
    }
    for (int i = 128; i < 256; ++i) {
      wavetable[i] = 0;
    }
  }
}
