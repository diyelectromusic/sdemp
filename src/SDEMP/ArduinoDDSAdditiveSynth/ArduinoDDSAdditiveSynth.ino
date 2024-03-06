/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino DDS Additive Synthesis
//  https://diyelectromusic.wordpress.com/2024/03/06/arduino-direct-digital-additive-synthesis/
//
      MIT License
      
      Copyright (c) 2024 diyelectromusic (Kevin)
      
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
    Direct Digital Synthesis - https://www.electronics-notes.com/articles/radio/frequency-synthesizer/dds-direct-digital-synthesis-synthesizer-what-is-basics.php
    Secrets of Arduino PWM - https://www.arduino.cc/en/Tutorial/SecretsOfArduinoPWM
    Arduino 101 Timers and Interrupts - https://www.robotshop.com/community/forum/t/arduino-101-timers-and-interrupts/13072
    Arduino R2R DAC - https://makezine.com/2008/05/29/makeit-protodac-shield-fo/
    Arduino Direct Port Manipulation - https://www.arduino.cc/en/Reference/PortManipulation
    Arduino Timer1  - https://playground.arduino.cc/Code/Timer1/
    Mozzi Library         - https://sensorium.github.io/Mozzi/
    Adafruit MCP4725      - https://learn.adafruit.com/mcp4725-12-bit-dac-tutorial/overview    
*/
// Audio Outputs
//   PWM - Uses PWM on D9 or D3
//   R2R - Uses a R2R DAC on D8,D9,D2-D7
//   DAC - Uses an I2C MCP4725 DAC
//
// Uncomment the output of choice
//
#define PWM_OUTPUT
//#define R2R_OUTPUT
//#define DAC_OUTPUT

//#define TIMING_TEST  10 // Toggle a timing pin
//#define DAC_TEST // Fix the amplitudes

// Uncomment out PWM output to use - D3 or D9
#define PWM_PIN_9   // Uses Timer 1
//#define PWM_PIN_3   // Uses Timer 2

/*
   There are 256 samples in the table.  For a 440Hz "A"
   there needs to be 256*440 samples played a second.
      256 * 440 = 112640

   Which would be a sample rate of 112.6 kHz or one sample
   every 8uS.  But that assumes every value has to be sent
   out every time.  That is not the case and credible sounds
   can still be produced by skipping values.

   So for a sample rate of 16384 Hz say (much more plausible),
   that requires a sample approx every 60uS.

   To output the sine wave at a 16384Hz sample rate
   to yield a 440Hz resulting pitch the increment has to be:
     For a increment of 1, the base freq = 16384 / 256 = 64 Hz
       So inc = (SAMPLE RATE / SAMPLES) / FREQ
              = FREQ * SAMPLES / SAMPLE RATE
              = FREQ * 256 / 16384
              = FREQ * 0.015625

   A good trick is to use fixed-point maths with an increment
   that is larger than it needs to be, then when calculating
   the increment, only the most significant bits are used.
       unsigned inc16;
       So inc16 = FREQ * (0.015625 * 256)
                = FREQ * 4
   Note that this can do a frequency up to around 16kHz before
   the value overflows a 16-bit integer.

   For 8192Hz:
      inc = FREQ * 256 / 8192 = 0.03125
      inc16 = FREQ * (0.03125 * 256) = 8
      period = 1 / 8192 = 122 uS

   For 4096: inc16 = 16; period = 244uS

   NB: For 6 pots, highest harmonic = 6xfundamental frequency
       so need sample rate = 2 * 6 * freq as a minimum.
       E.g. for 440Hz minimum sample rate = 5280Hz to capture
            all 6 first harmonics.
*/
// 4096Hz
//#define SAMPLE_RATE   4096
//#define SAMPLE_PERIOD 244
//#define INC_MULT 16
// 8192Hz
//#define SAMPLE_RATE   8192
//#define SAMPLE_PERIOD 122
//#define INC_MULT 8
// 16384 Hz
#define SAMPLE_RATE   16384
#define SAMPLE_PERIOD 61
#define INC_MULT 4
// 32768 Hz
//#define SAMPLE_RATE   32768
//#define SAMPLE_PERIOD 30
//#define INC_MULT 2

#define FREQ2INC(freq) (freq*INC_MULT)
#define DEFAULT_FREQ 440

#define NUMPOTS 6 // Up to 6
#define MAXPOTS 6
int pot_pins[MAXPOTS] = {A0,A1,A2,A3,A6,A7}; // Skip 4/5 for I2C
int pots[NUMPOTS];

// Need to set scaling for the volume depending on number of pots:
// - Wave table goes 0..255 * max volume.
// - And there are NUMPOTS of these to add up.
// - This has to be kept within a 16-bit value.
//
// Example: 6 pots has a theoretical maximum of
//   (255*SC)*6 < 65535
// So required scaling = 65535 / (6 * 255) ~42
//
// But as there is unlikely to be all 6 waves
// added at maximum amplitude at the same time
// we can round up to a sensible ^2.
#define SC  32
#define PSC 4  // 10-bit to 6-bit for analogRead

#define MCP4725ADDR (0x60)

// Some presets
int sine[MAXPOTS] = {SC*2,0,0,0,0,0};
int saw[MAXPOTS] = {SC,SC/2,SC/4,SC/8,SC/16,SC/32};
int squ[MAXPOTS] = {SC,0,SC/2,0,SC/4,0};

void setDefaultAmplitudes () {
  for (int i=0; i<NUMPOTS; i++) {
#ifdef DAC_TEST
    pots[i] = sine[i];
#else
    pots[i] = 0;
#endif
  }
}

// Accumulator/index into wavetable per pot
uint16_t acc[NUMPOTS]; // 8.8 fixed point accumulator
uint16_t freq[NUMPOTS];

// NB: Rely on fact we're using an 8-bit index to auto wrap for our 256 samples
#define NUM_SAMPLES   256
uint8_t sinedata[NUM_SAMPLES] = {
  128, 131, 134, 137, 140, 144, 147, 150, 153, 156, 159, 162, 165, 168, 171, 174,
  177, 179, 182, 185, 188, 191, 193, 196, 199, 201, 204, 206, 209, 211, 213, 216,
  218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 239, 240, 241, 243, 244,
  245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255,
  255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251, 250, 250, 249, 248, 246,
  245, 244, 243, 241, 240, 239, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220,
  218, 216, 213, 211, 209, 206, 204, 201, 199, 196, 193, 191, 188, 185, 182, 179,
  177, 174, 171, 168, 165, 162, 159, 156, 153, 150, 147, 144, 140, 137, 134, 131,
  128, 125, 122, 119, 116, 112, 109, 106, 103, 100,  97,  94,  91,  88,  85,  82,
   79,  77,  74,  71,  68,  65,  63,  60,  57,  55,  52,  50,  47,  45,  43,  40,
   38,  36,  34,  32,  30,  28,  26,  24,  22,  21,  19,  17,  16,  15,  13,  12,
   11,  10,   8,   7,   6,   6,   5,   4,   3,   3,   2,   2,   2,   1,   1,   1,
    1,   1,   1,   1,   2,   2,   2,   3,   3,   4,   5,   6,   6,   7,   8,  10,
   11,  12,  13,  15,  16,  17,  19,  21,  22,  24,  26,  28,  30,  32,  34,  36,
   38,  40,  43,  45,  47,  50,  52,  55,  57,  60,  63,  65,  68,  71,  74,  77,
   79,  82,  85,  88,  91,  94,  97, 100, 103, 106, 109, 112, 116, 119, 122, 125
};

//////////////////////////////////////////////////////////////////////////////
//
// PWM Audio Output on D9 or D3
//
//////////////////////////////////////////////////////////////////////////////
#ifdef PWM_OUTPUT
#define myAnalogRead analogRead

#ifdef PWM_PIN_9
unsigned SampleScaling;
void initPwm() {
  // Initialise Timer 1 as follows:
  //    Output on pin 9 (OC1A).
  //    Run at CPU clock speed (16MHz) no prescalar.
  //    Use 65536Hz PWM frequency.
  //    Use PWM Fast mode (count up then reset).
  //    Use ICR1 as the TOP value.
  //    TOP value = (clock / (prescaler * 65536))-1 = 243
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
  ICR1 = 243;

#if SAMPLE_RATE==4096
  SampleScaling = 16;
#elif SAMPLE_RATE==8192
  SampleScaling = 8;
#elif SAMPLE_RATE==16384
  SampleScaling = 4;
#elif SAMPLE_RATE==32768
  SampleScaling = 2;
#else
#warning Unrecognised Sample Rate
#endif

  // Check PWM output is active
  OCR1A = 243/2;
//  delay (5000);

  TIMSK1 = _BV(TOIE1);
}

void setPwm (uint8_t pwmval) {
  OCR1A = pwmval;
}

int pwmout;
ISR(TIMER1_OVF_vect) {
  pwmout++;
  if (pwmout>(SampleScaling-1)) {
    dacPlayer();
    pwmout = 0;
  }
}
#endif

#ifdef PWM_PIN_3
unsigned SampleScaling;
void initPwm() {
  // Initialise Timer 2 as follows:
  //    Output on pin 3 (OC2B).
  //    Run at CPU clock speed (16MHz) no prescalar.
  //    Use FastPWM mode (count up, then reset).
  //    Use OC2A as the TOP value.
  //    TOP value = (clock / (prescaler * 65536)) - 1 = 243
  //    PWM value is updated by writing to OCR2B
  //    Interrupt enabled for overflow: TIMER2_OVF_vect
  //
  // So set up PWM for Timer 2, Output B:
  //   WGM2[2:0] = 111 = FastPWM; OCR2A as TOP; TOV1 set at BOTTOM
  //   COM2A[1:0] = 00 = OC2A disconnected
  //   COM2B[1:0] = 10  Clear OC2B on cmp match up; Set OC2B at cmp match down
  //   CS2[2:0] = 001 = No prescalar
  //   TOIE2 = Timer/Counter 2 Overflow Interrupt Enable
  //
  pinMode(3, OUTPUT);
  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS20);
  OCR2A = 243;

#if SAMPLE_RATE==4096
  SampleScaling = 16;
#elif SAMPLE_RATE==8192
  SampleScaling = 8;
#elif SAMPLE_RATE==16384
  SampleScaling = 4;
#elif SAMPLE_RATE==32768
  SampleScaling = 2;
#else
#warning Unrecognised Sample Rate
#endif

  // Check PWM output is active
  OCR2B = 243/2;
//  delay (5000);

  TIMSK2 = _BV(TOIE2);
}

void setPwm (uint8_t pwmval) {
  OCR2B = pwmval;
}

int pwmout;
ISR(TIMER2_OVF_vect) {
  pwmout++;
  if (pwmout>(SampleScaling-1)) {
    dacPlayer();
    pwmout = 0;
  }
}
#endif

void dacSetup (){
  initPwm();
}

void dacWrite (uint16_t value) {
  // Convert to 8-bit
  setPwm(value>>8);
}

void dacScan () {
  // Nothing to do - all triggered from the timer ISR
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// R2R Audio Output on D8-D9,D2-D7
//
//////////////////////////////////////////////////////////////////////////////
#ifdef R2R_OUTPUT
#include "TimerOne.h"
#define myAnalogRead analogRead

void dacSetup (){
  // Initialise our output pins
  DDRD |= 0b11111100;
  DDRB |= 0b00000011;

  // Use an Arduino Timer1 to generate a regular "tick"
  // to run our code every SAMPLE_PERIOD microseconds.
  Timer1.initialize (SAMPLE_PERIOD);
  Timer1.attachInterrupt (dacPlayer);
}

void dacWrite (uint16_t value) {
  // Ideally this would be a single write action, but
  // I didn't really want to use the TX/RX pins on the Arduino
  // as outputs - these are pins 0 and 1 of PORTD.
  //
  // There is the possibility that the slight delay between
  // setting the two values might create a bit of a "glitch"
  // in the output, but the last two bits (PORTB) are pretty
  // much considered minor adjustments really, so it shouldn't
  // be too noticeable.
  //
  // Convert to 8-bits
  value = value>>8;
  uint8_t pd = (PIND & 0b00000011) | (value & 0b11111100);
  uint8_t pb = (PINB & 0b11111100) | (value & 0b00000011);
  PORTD = pd;
  PORTB = pb;
}

void dacScan () {
  // Nothing to do - all triggered from the timer
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// MCP4725 I2C 12-bit DAC Output
//
//////////////////////////////////////////////////////////////////////////////
#ifdef DAC_OUTPUT
// Mozzi provided fast ADC routines and I2C handling
#include "mozzi_analog.h"
#include "twi_nonblock.h"
#define myAnalogRead mozziAnalogRead

void dacSetup (){
  // Initialise Mozzi fast ADC routines
  setupMozziADC();
  setupFastAnalogRead();

  // Set A2 and A3 as Outputs to make them our GND and Vcc,
  // which will power the MCP4725.  Allows the DAC to be
  // plugged directly into A2-A5 on an Uno.
#if 0
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  digitalWrite(A2, LOW);//Set A2 as GND
  digitalWrite(A3, HIGH);//Set A3 as VCC
#endif

  // activate internal pullups for I2C
  digitalWrite(SDA, 1);
  digitalWrite(SCL, 1);

  initialize_twi_nonblock();

  // Update the TWI registers directly for fast mode I2C.
  // They will have already been preset to 100000 in twi_nonblock.cpp
  TWBR = ((F_CPU / 400000L) - 16) / 2;
}

void dacWrite (uint16_t value) {
  // There are several modes of writing DAC values (see the MCP4725 datasheet).
  // In summary:
  //     Fast Write Mode requires two bytes:
  //          0x0n + Upper 4 bits of data - d11-d10-d9-d8
  //                 Lower 8 bits of data - d7-d6-d5-d4-d3-d2-d1-d0
  //
  //     "Normal" DAC write requires three bytes:
  //          0x40 - Write DAC register (can use 0x50 if wanting to write to the EEPROM too)
  //          Upper 8 bits - d11-d10-d9-d9-d7-d6-d5-d4
  //          Lower 4 bits - d3-d2-d1-d0-0-0-0-0
  //
  // Convert to a 12-bit value
  value = value>>4;
  uint8_t val1 = (value & 0xf00) >> 8; // Will leave top 4 bits zero = "fast write" command
  uint8_t val2 = (value & 0xff);
  twowire_beginTransmission(MCP4725ADDR);
  twowire_send (val1);
  twowire_send (val2);
  twowire_endTransmission();
}

unsigned long timing;
void dacScan () {
  // NB: On an Uno micros() has a resolution of ~4uS
  unsigned long timenow = micros();
  if (timenow > timing) {
    dacPlayer();
    timing = timenow + SAMPLE_PERIOD;
  }
}
#endif  // DAC OUTPUT

//////////////////////////////////////////////////////////////////////////////
//
// Common dacPlayer, setup and loop
//
//////////////////////////////////////////////////////////////////////////////
int toggle;
uint16_t dacvalue=0;
void dacPlayer (){
#ifdef TIMING_TEST
  digitalWrite(TIMING_TEST, toggle);
  toggle = !toggle;
#endif
  // Output last DAC value first so it is "on time"
  dacWrite(dacvalue);

  // Update all accumulators and calculate next sample
  dacvalue = 0;
  for (int i=0; i<NUMPOTS; i++) {
    // Recall this is an 8.8 fixed point value
    // that will auto wrap around from 255.255 to 0.0
    acc[i] += FREQ2INC(freq[i]);

    // Additive synthesis:
    //   Add in harmonic * amplitude
    //   Read value from sinetable based on top
    //   8-bits of the accumulator.
    //
    // NB: dacvalue is 16-bits at this stage
    dacvalue += (sinedata[acc[i]>>8]*pots[i]);
  }  
}

void setFreqs (uint16_t f) {
  for (int i=0; i<NUMPOTS; i++) {
    // Create the basic harmonic series of integer multiples of fundamental
    freq[i] = f*(i+1);
  }
}

void setup(void) {
#ifdef TIMING_TEST
  pinMode (TIMING_TEST, OUTPUT);
  toggle = LOW;
  digitalWrite (TIMING_TEST, toggle);
#endif

  setDefaultAmplitudes();
  for (int i=0; i<NUMPOTS; i++) {
    acc[i] = 0;
  }

  setFreqs(DEFAULT_FREQ);

  dacSetup();
}

int sequencer;
int pot;
void loop(void) {
  dacScan();

  sequencer++;
  if (sequencer > 10) {
    // Convert 0..1023 into scaled value.
    // Use Mozzi routine as it is quicker.
#ifndef DAC_TEST
    pots[pot] = myAnalogRead(pot_pins[pot])>>PSC;
#endif
    pot++;
    if (pot >= NUMPOTS) pot = 0;
  }
}
