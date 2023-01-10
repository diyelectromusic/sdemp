/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Make Your Uno Synth - Drum Machine
//  https://diyelectromusic.wordpress.com/2023/01/05/arduino-make-your-uno-synth/
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
    Arduino PWM Output - https://diyelectromusic.wordpress.com/2021/07/27/arduino-pwm-sound-output/
    Potentiometer      - https://www.arduino.cc/en/Tutorial/Potentiometer

  Sounds and patterns based on "O2 Minipops" Arduino drum machine by Jan Ostman.
*/
#include "minipops.h"

// IO pins in use
#define POT_PATTERN A0
#define POT_TEMPO   A2
#define PWM_PIN     9
//#define TEST_PIN    8  // Uncomment for IRQ timing test

#define TEMPO_MIN 60  // 1 "tick" every second
#define TEMPO_MAX 960 // 16 "ticks" a second

//#define DEBUG  // Uncomment for diagnostic serial output

#ifdef DEBUG
#define PRINT(x) Serial.print(x)
#define PRINTLN(x) Serial.println(x)
#else
#define PRINT(x)
#define PRINTLN(x)
#endif


//
// PWM Audio output at 62.5kHz using Timer 1
// Output on OC1A (D9)
//
void pwmInit() {
  // Initialise Timer 1 as follows:
  //    Output to OC1A (non-inverting mode) - D9.
  //    Run at CPU clock speed (16MHz), no prescalar.
  //    Use PWM Fast mode (count up then reset).
  //    TOP value = 255
  //    PWM value is updated by writing to OCR1A (0..255)
  //    No interrupts
  //    PWM Carrier freq = 16MHz / 256 = 62.5kHz
  //
  // So set up PWM for Timer 1, Output A:
  //   WGM1[3:0] = 0101 = FastPWM; 8-bit; TOP=0xFF; Update OCR1A at BOTTOM
  //   COM1A[1:0] = 10 = Clear OC1A on compare match; set at BOTTOM
  //   CS1[2:0] = 001 = No prescalar
  //
  pinMode(PWM_PIN, OUTPUT);
  TCCR1A = _BV(COM1A1) | _BV(WGM10);  // COM1A[1:0]=10; WGM[1:0]=01
  TCCR1B = _BV(WGM12) | _BV(CS10);    // WGM[3:2]=01; CS[2:1]=001
  TCCR1C = 0;
  OCR1A = 128; // Start at midpoint

  TIMSK1 = 0;
}

//
// Sample player at 20kHz using Timer 2
//
void sampleInit() {
  // Initialise Timer 2 as follows:
  //    Run at CPU clock speed (16MHz).
  //    Use CTC mode (count up, then reset).
  //
  //    Want sample frequency of 20kHz
  //      Number of Steps = Counter Freq / (N . SampleRate)
  //    If no prescalar: Steps = 16MHz / 20kHz = 800
  //    Ideally DIV4 prescalar, but only have DIV1 or DIV8
  //    so use Prescalar = DIV8:
  //      Number of Steps = 16MHz / (8 x 20kHz) = 100
  //
  //    TOP = 100-1 (so it runs 0 to 99)
  //    Interrupt = Compare on Match A
  //
  // So set up for Timer 2:
  //   WGM2[2:0] = 010 = CTC; OCR2A as TOP; TOV set at MAX
  //   COM2A[1:0] = COM2B[1:0] = 00 = OC2A/B disconnected
  //   CS2[2:0] = 010 = clkIO/8
  //   OCIE2A = Timer/Counter 2 Compare Match A Interrupt Enable
  //
#ifdef TEST_PIN
  pinMode(TEST_PIN, OUTPUT);
#endif
  TCCR2A = _BV(WGM21);  // COM2A[1:0]=00; COM2B[1:0]=00;WGM2[1:0]=10
  TCCR2B = _BV(CS21);   // WGM[2]=0; CS2[2:0]=010
  OCR2A = 99;
  TIMSK2 = _BV(OCIE2A);
}

int irqtoggle;
ISR(TIMER2_COMPA_vect) {
  uint8_t sample;
  if (rbPop (&sample)) {
    // Update the PWM value for output
    OCR1A = sample;
  }

#ifdef TEST_PIN
  irqtoggle = !irqtoggle;
  digitalWrite(TEST_PIN, irqtoggle);
#endif
}

// Sample Order (same as used for the patterns)
//     GU BG2 BD CL CW MA CY QU
const unsigned char *sampleVoice[NUM_VOICES] = {GU, BG2, BD, CL, CW, MA, CY, QU};
int sampleSize[NUM_VOICES] = {GU_SAMPLE_SIZE, BG2_SAMPLE_SIZE, BD_SAMPLE_SIZE, CL_SAMPLE_SIZE, CW_SAMPLE_SIZE, MA_SAMPLE_SIZE, CY_SAMPLE_SIZE, QU_SAMPLE_SIZE};
int sampleCnt[NUM_VOICES];
int sampleIdx[NUM_VOICES];

// Calculate the next sample to play.
// Will return a value in the range 0..255 biased for 128=centre (zero)
//
uint8_t sampleGet () {
  // Check the sample indexes for each instrument and add up any that are playing
  int total = 0;
  for (int i=0; i<NUM_VOICES; i++) {
    if (sampleCnt[i] > 0) {
      // Convert any playing sample into a +/- value and add it to the total
      total = total + pgm_read_byte_near(sampleVoice[i] + sampleIdx[i]) - SAMPLE_BIAS;
      sampleIdx[i]++;
      sampleCnt[i]--;
    }
  }

  // Bias the value to the centre of an 8-bit unsigned value
  total = total + 128;

  // Clip to limits of a biased 8-bit unsigned value
  if (total < 0) total = 0;
  if (total > 255) total = 255;

  return total;
}

void sampleRestart (int voice) {
  sampleCnt[voice] = sampleSize[voice];
  sampleIdx[voice] = 0;
}

//
// Ring Buffer handler for samples to be played
//
#define RB_SIZE 256
uint8_t rb[RB_SIZE];
unsigned rbWrite;
unsigned rbRead;
unsigned rbCount;  // As may be updated in interrupt routine

void rbInit() {
  rbWrite = 0;
  rbRead = 0;
  rbCount = 0;
}

// This will be called from the non-interrupt main loop
void rbPush(uint8_t rbval) {
  // Musn't let the RB get updated whilst it is being updated here
  noInterrupts();
  if (rbCount < RB_SIZE) {
    rb[rbWrite] = rbval;
    rbWrite++;
    if (rbWrite >= RB_SIZE) rbWrite = 0;
    rbCount++;
  }
  interrupts();
}

// This will be called from the interrupt routine
bool rbPop (uint8_t *pRbVal) {
  if (rbCount > 0) {
    *pRbVal = rb[rbRead];
    rbRead++;
    if (rbRead >= RB_SIZE) rbRead = 0;
    rbCount--;
    return true;
  } else {
    return false;
  }
}

bool rbIsFull () {
  if (rbCount < RB_SIZE) {
    return false;
  } else {
    return true;
  }
}

//
// Analog Inputs for TEMPO and PATTERN
//
int tempo;
int pattern;
int patternlen;
void adcInit() {
  // Initialise both values to start...
  adcUpdate();
  adcUpdate();
}

#define ADCCOUNT 200
int adccount;
void adcUpdate() {

  // Read one of the analog inputs each scan
  if (adccount == ADCCOUNT) {
    tempo = map (analogRead (POT_TEMPO), 0, 1023, TEMPO_MIN, TEMPO_MAX);
  } if (adccount == ADCCOUNT*2) {
    pattern = analogRead(POT_PATTERN) >> 6;
    patternlen = pgm_read_byte_near(PATLEN + pattern);
  }
  adccount++;
  if (adccount > ADCCOUNT*2) {
    adccount = 0;
  }
}

unsigned long playtime;
bool isTimeToPlay () {
  unsigned long timenow = millis();
  if (timenow > playtime) {
    // The tempo is in beats per minute (bpm).
    // So beats per second = tempo/60
    // So the time in seconds of each beat = 1/beats per second
    // So time in mS = 1000 / (tempo/60)
    // or time in mS = 60000 / tempo
    playtime = timenow + 60000UL/tempo;
    return true;
  }

  return false;
}

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#endif
  rbInit();
  adcInit();
  pwmInit();
  sampleInit();
}

int stepcount;
void loop() {
  //
  // First handling any playing sounds...
  //
  if (rbIsFull()) {
    // Can't do anything right now if our buffer is full...
  } else {
    // Grab the next sample to be played...
    uint8_t sample = sampleGet ();
    
    // ... and add it to the buffer.
    rbPush(sample);
    
    // Note: as this is the only thing writing to the buffer,
    // the result of rbIsFull() should still be valid at this point.
  }

  //
  // Read the IO to update tempo/pattern selection
  //
  adcUpdate();

  //
  // Run the Sequencer Step
  //
  if (isTimeToPlay()) {
    uint8_t pat = pgm_read_byte_near(PATTERN + (pattern*MAX_PATTERN_LEN) + stepcount);
    for (int i=0; i<NUM_VOICES; i++) {
      // NB: patterns are read from left to right...
      if (pat & (1<<(NUM_VOICES-1-i))) {
        // Voice is required to be played so restart the sample player for this voice
        sampleRestart(i);
      }
    }

    stepcount++;
    if (stepcount >= patternlen) {
      stepcount = 0;
    }
  }
}
