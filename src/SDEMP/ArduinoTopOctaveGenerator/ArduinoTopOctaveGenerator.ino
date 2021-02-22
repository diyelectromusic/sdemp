/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Top Octave Generator
//  https://diyelectromusic.wordpress.com/2021/02/22/arduino-top-octave-generator/
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
    Arduino Direct Port Manipulation - https://www.arduino.cc/en/Reference/PortManipulation
    Arduino Timer1  - https://playground.arduino.cc/Code/Timer1/


*/
#include <TimerOne.h>

// Test pin is hardcoded to A0 or PORTC PC0.
// Use this to check the timing of the interrupt routine.
// Comment this out to disable the test.
#define TESTPIN 1
#define TESTMASK 1 // PC0

// ATmega328 Timers
//
// There are three timers on the 328, and we are only using one
// here for our interrupt.
//
// This means that we can use the other two for a PWM output
// to provide two of the required frequencies with no additional
// effort from the CPU.
//
// Both Timer 0 and Timer 2 are 8-bit counters, but Timer 1 is a
// 16-bit timer, which gives us a higher resolution for output
// frequencies.
//
// If some calculations are done using the Arduino's 16MHz clock,
// and the range of prescaler values of 1, 8, 64, 256, 1024, then
// it turns out that the most accurate reproduction of any of our
// required frequencies comes with the following values:
//   C:  523.25Hz; Prescaler=1; counter TOP=30577, reproduced freq=523.252Hz.
//   D#: 622.25Hz; Prescaler=1; counter TOP=25712, reproduced freq=622.253Hz.
//
// But these would both require 16-bit counters, and we only have
// one of those.  Consequently, I'm using the values for C above
// and the next best "fit" using an 8-bit counter value as follows:
//   A:  880.00Hz; Prescaler=64; counter TOP=283, reproduced freq=880.282Hz.
//   A:  880.00Hz; Prescaler=128; counter TOP=141, reproduced freq=880.282Hz.
//   B:  987.77Hz; Prescaler=64; counter TOP=252, reproduced freq=988.142Hz
//
// Due to the "toggle bit mode" in use, we can use an equivalent counter
// value up to 511 rather than 255 for the 8-bit counter.
// See comments later on for explanation!)
//
// We thus don't bother calculating these and instead just
// set them up to continually "play" on Timers 1 and 2 PWM outputs.
//
// We have a choice of outputs, so choosing two pins that are next to each
// other means using the following:
//   Timer 1 PWM output OC1B - Pin D10
//   Timer 2 PWM output OC2A - Pin D11
//
// Consequently we are only using 10 software counters within the code,
// relying on the final two frequencies to be set up via PWM on
// pins 10 and 11 as described above.
//
#define NUM_TONES 10
#define PORTDMASK  0b11111100; // Pins 2-7
#define PORTBMASK  0b00111111; // Pins 8-9, 12-13; 10-11 in PWM mode
//int pins[NUM_TONES]={2,3,4,5, 6,7,8,9, 12,13};

// This project runs code on the command of one of the Arduino Timers.
// This determines the length of the "tick" of the timer.
//
// Using a TICK of 10 means our code runs every
// 10 microseconds, i.e. at a frequency of 100kHz.
//
// A TICK of 20 is 50kHz and so on.
//
// Sounds are created by sending HIGH or LOW pulses to the IO pins.
// The length of the pulses determines the eventual frequency of the note played.
//
// The same TICK can be used for a whole range of frequencies as long as
// the TICK period is a useful divisor of the required frequencies.
//
// Calculations using a spreadsheet, and analysing the errors in the
// output frequencies from "rounding" to multiples of the basic pulse
// suggest aiming for a 20uS pulse.
//
// This supports a frequency range of 130.81Hz (C3) up to 523.25Hz (C5)
// without too much error at the high end.
//
// Basically, the tuning is tolerable at this range!
//
// Faster (i.e. lower) is always better but if the time spent in the
// timer routing to update all the pulses ends up longer than the TICK
// itself, everything will fall apart.
//
// A TICK of 20uS is only accurate for 10 counters...
//
#define TICK   20    // 20uS

// The frequency table is specified in terms of the number of pulses
// required to toggle the output pin at the right frequency.
//
// The counters are initialised to these values and decremented by 1
// each time the interrupt routine runs.  When they reach zero the
// output pin for that frequency is toggled and the counter is reset
// to these values to start again.
//
// There are two sets of counters, one for the even (e.g. HIGH)
// timing and one for the odd (e.g. LOW) timing.  This allows a crude
// approach to supporting "half" cycle accuracy, for example an
// even 80 and odd 81 is like have a period of 80.5.
//
// This gives slightly more accurate tuning for some notes...
//
// Note that these (and the corresponding counters/playing values below)
// are all 8-bit values, so the largest count we currently support is 256.
//
uint8_t freqH[NUM_TONES]={
//   No C, C#, D, D#, E, F, F#, G, G#, No A, A#, B
           45,43, 40,38,36, 34,32, 30,       27, 25  // C5 to B5
};
uint8_t freqL[NUM_TONES]={
//   No C, C#, D, D#, E, F, F#, G, G#, No A, A#, B
           45,42, 40,38,36, 34,32, 30,       27, 26  // C5 to B5
};

uint8_t counters[NUM_TONES];
uint16_t toneenable;
uint16_t tonepulse;

void setup() {
  // Set the pin directions using direct port accesses
  DDRD |= PORTDMASK;
  DDRB |= PORTBMASK;

#ifdef TESTPIN
  DDRC |= TESTMASK;
#endif
  
  // initialise the counters
  for (int i=0; i<NUM_TONES; i++) {
    counters[i] = freqH[i];
  }

  tonepulse  = 0;

  // Use an Arduino Timer 0 to generate a regular "tick"
  // to run our code every TICK microseconds.
  //
  // This messes with millis() and delay(), so we can no
  // no longer use them in the code!
  //
  // The default settings are as follows:
  //    Prescaler: 64
  //    Mode: Normal "counter" mode
  //    TOIE0 = enabled - Interrupt on OVERFLOW (i.e. counter wraps around at 256)
  //
  // This results in the Arduino's standard millis() interrupt
  // handler being called with a frequency of 16000000/64/256 ~ 976.6Hz,
  // or once approx every 1mS.
  //
  // We want to trigger on each TICK which we need to be 20uS,
  // so we need to change it to the following:
  //    Prescaler: 8
  //    Mode: Clear Timer on Compare (CTC)
  //    Output: None
  //    OCIE0A = enabled - Interrupt on COMPARE A (TIMER0_COMPA)
  //    Counter value: 39
  //
  // This will trigger the interrupt at a frequency of 50kHz,
  // or once every 20uS.
  //
  // This also means that the Arduino's Timer interrupt will never trigger!
  //
  // Relevant registers (see section 18 in ATmega328 datasheet):
  //    TCCR0A - Timer 0 control register
  //    TCCR0B - Timer 0 second control register
  //    TIMSK0 - Timer 0 interrupt mask register
  //    TCNT0  - Timer 0 counter value register
  //    OCR0A  - Timer 0 output compare register A
  //    OCR0B  - Timer 0 output compare register B
  //    TIFR0  - Timer 0 interrupt flag register
  //
  // Pins associated with Timer 0: OC0A, OC0B
  //
  // Disable all interrupts whilst messing with the Arduino timer
  // to prevent weird things happening!
  noInterrupts();
  TCCR0A = 0b00000010;  // COM0A=00; COM0B=00; WGM=010 i.e. Unconnect OC0A/OC0B; CTC Mode
  TCCR0B = 0b00000010;  // FOC0x=00; WGM=010; CS0=010 i.e. Prescaler 1/8
  TIMSK0 = 0b00000010;  // OCIE0A interrupt enabled.
  OCR0A  = 39;          // 16MHz/(8*(39+1)) = 50kHz or 20uS

  // Use Arduino Timer 1 for one of the PWM fixed frequency outputs.
  //
  // Set Timer 1 as follows
  //   Prescaler: 1
  //   Mode: Clear Timer on Compare (CTC)
  //   Output: Toggle OC1B on match
  //   No Interrupts
  //   Counter value: 15288
  //     Note: Want counter of 30577 for a complete pulse i.e. the HIGH+LOW times.
  //           But this toggles on timer compare, so need to toggle at twice the frequency.
  //
  // Relevant registers (see section 19 in ATmega328 datasheet):
  //   TCCR1A - Timer 1 control register
  //   TCCR1B - Timer 1 second control register
  //   TCCR1C - Timer 1 third control register
  //   TCNT1L/H - Timer 1 counter value register low and high byte
  //   ICR1L/H  - Timer 1 input capture register low and high byte
  //   OCR1AL/H - Timer 1 output compare A register low and high byte
  //   OCR1BL/H - Timer 1 output compare B register low and high byte
  //   TIMSK1   - Timer 1 interrupt mask register
  //   TIFR1    - Timer 1 interrupt flag register
  //
  // Pins associated with Timer 1: OC1A, OC1B
  //
  TCCR1A = 0b00010000;  // COM1A=00; COM1B=01; WGM=0100 i.e. Toggle OC1B on match; CTC mode
  TCCR1B = 0b00001001;  // ICxx1=00; WGM=0100; CS1=001 i.e. Prescaler=1
  TCCR1C = 0;           // F0C1x=00
  OCR1A  = 15288;       // 16MHz/(1*(15288+1)) = 1046.504Hz; / 2 = 523.252Hz
  TIMSK1 = 0;           // No interrupts

  // Use Arduino Timer 2 for the other PWM fixed frequency outputs.
  //
  // Set Timer 2 as follows
  //   Prescaler: 64
  //   Mode: Clear Timer on Compare (CTC)
  //   Output: Toggle OC2A on match
  //   No Interrupts
  //   Counter value: 141
  //     Note: Once again, we need to toggle at twice the frequency to get our
  //           required equivalent counter value of 283.
  //
  // Relevant registers (see section 19 in ATmega328 datasheet):
  //   TCCR2A - Timer 2 control register
  //   TCCR2B - Timer 2 second control register
  //   TCNT2  - Timer 2 counter value register
  //   OCR2A  - Timer 2 output compare A register
  //   OCR2B  - Timer 2 output compare B register
  //   TIMSK2 - Timer 2 interrupt mask register
  //   TIFR2  - Timer 2 interrupt flag register
  //
  // Pins associated with Timer 1: OC2A, OC2B
  //
  TCCR2A = 0b01000010;  // COM2A=01; COM2B=00; WGM=010 i.e. Toggle OC2A on match; CTC mode
  TCCR2B = 0b00000100;  // FOC2x=00; WGM=010; CS1=100 i.e. Prescaler=64
  OCR2A  = 141;         // 16MHz/(64*(141+1)) = 1760.563Hz; / 2 = 880.281Hz
  TIMSK2 = 0;           // No interrupts

  // Can now enable all interrupts
  interrupts();
}

void loop() {
  // Nothing to do - its all in the interrupt handler
}

// This has to run quicker than the TICK is running!
// That should probably be checked with an oscilloscpe or logic analyser.
//
ISR(TIMER0_COMPA_vect) {
#ifdef TESTPIN
  PORTC |= TESTMASK;
#endif
  // For each tone pin, we decrement the counter associated with the pin.
  // When it reaches zero we then toggle the output pin and reset the counter.
  for (int i=0; i<NUM_TONES; i++) {
    counters[i]--;
    if (counters[i] == 0) {
      if (tonepulse & (1<<i)) {
        tonepulse &= ~(1<<i);
        counters[i] = freqL[i];
      } else {
        tonepulse |= (1<<i);
        counters[i] = freqH[i];
      }
    }
  }

  // Write the outputs using following split:
  //   tonepulse = 0b0000 0011 1111 1111
  //      port D =             0b11 1111 00
  //      port B = 0b  00 1100 11
  uint8_t pd = ((uint16_t)(tonepulse)<<2) & 0xFC;  // PORTD sets 0b11111100
  uint8_t pb = ((uint16_t)(tonepulse)>>4) & 0x30;  // PORTB sets 0b00110000
         pb |= ((uint16_t)(tonepulse)>>6) & 0x03;  // PORTB sets 0b00000011

  // As PORTD pins 0 and 1 are the RX/TX pins, we don't
  // want to trash those while writing the other pins in case
  // that messes with the serial comms.
  //
  // I don't care about the rest of PORTB!
  //
  PORTD = (PORTD & 0x03) | pd;
  PORTB = pb;  
#ifdef TESTPIN
  PORTC &= ~TESTMASK;
#endif
}
