/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino R2R Synthesis - Part 2
//  https://diyelectromusic.wordpress.com/2020/06/25/arduino-r2r-digital-audio-part-2/
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
    Arduino R2R DAC - https://makezine.com/2008/05/29/makeit-protodac-shield-fo/
    Arduino Direct Port Manipulation - https://www.arduino.cc/en/Reference/PortManipulation
    Arduino Timer1  - https://playground.arduino.cc/Code/Timer1/

*/
#include "TimerOne.h"

// The length of the "tick" of the timer.
//
// Using a TICK of 10 means our code runs every
// 10 microseconds, i.e. at a frequency of 100kHz.
//
// A TICK of 100 is 10 kHz and so on.
//
#define TICK  10

uint8_t wave;
uint8_t wave_step = 4;
int pot_delay;
int delay_counter;

void setup() {
  // put your setup code here, to run once:

  // Initialise our output pins
  DDRD |= 0b11111100;
  DDRB |= 0b00000011;

  // Use an Arduino Timer1 to generate a regular "tick"
  // to run our code every TICK microseconds.
  Timer1.initialize (TICK);
  Timer1.attachInterrupt (ddsOutput);

  wave = 0;
  delay_counter = 0;
}

void loop() {
  // put your main code here, to run repeatedly:

  // Use the value read from the potentiometer to define
  // the delay to use later on (measured in TICKs).
  //
  // Also, the shorter the delay, the faster the loop
  // runs, so the higher the note.
  //
  // Swap the values around so that a low reading
  // plays a lower note.
  //
  // As the full range of the readings are 0 to 1023,
  // subtract from 1023 to reverse.
  //
  int pot = analogRead(A0);
  pot_delay = 1023-pot;

}

// This is the code that is run on every "tick" of the timer.
void ddsOutput () {

  // The fastest this will run is if pot_delay is zero,
  // in which case this will run everytime the TICK ticks.
  //
  // For any value of pot_delay>0 it will run slower
  // generating different frequencies of tone.
  //
  // Note however that an increase in 1 in pot_delay
  // will mean the period of the code running changes
  // by 256/wave_step each time as that is how many
  // numbers need to be written out for 1 cycle of sound.
  //
  // I use pot_delay = 1023 as a special case to turn
  // everything off.
  //
  delay_counter++;
  if ((delay_counter < pot_delay) || (pot_delay >= 1023)) {
    // nothing to do yet - we just wait for the next delay to pass
    return;
  }
  // reset the counter ready for the next count
  delay_counter = 0;

  // We loop through the values (0 to 255) in steps of wave_step
  // in the waveform, once every time this code operates.
  //
  // With a TICK of 10uS we can use a wave_step of 1 and still
  // manage a freqency of 1/(256*0.00001) or 390Hz.
  //
  // I use a wave_step of 4, which means the highest frequency
  // is 1/(64*.00001) or approx 1.5kHz.
  //
  // The lowest frequency with a wave_step of 4 will
  // be 1/(1024*64*0.00001) or approx 1.5Hz.
  //

  // This will automatically wrap around from 255 back to 0
  // So it will create a simple saw-tooth waveform.
  wave += wave_step;

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
  uint8_t pd = wave & 0b11111100;
  uint8_t pb = wave & 0b00000011;
  PORTD = pd;
  PORTB = pb;
}