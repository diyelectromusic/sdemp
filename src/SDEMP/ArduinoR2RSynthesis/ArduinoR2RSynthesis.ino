/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino R2R Synthesis
//  https://diyelectromusic.wordpress.com/2020/06/28/arduino-r2r-digital-audio/
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

*/

// Comment this out to ignore the analog read and run
// as fast as possible with a fixed frequency
//#define USE_POT 1

uint8_t wave;
uint8_t wave_step = 8;

void setup() {
  // put your setup code here, to run once:

  // Initialise our output pins
  DDRD |= 0b11111100;
  DDRB |= 0b00000011;

  wave = 0;
}

void loop() {
  // put your main code here, to run repeatedly:

#ifdef USE_POT
  // Use the value read from the potentiometer to define
  // the delay to use later on (minimum of 5 uS).
  //
  // Also, the shorter the delay, the faster the loop
  // runs, so the higher the note.
  //
  // Swap the values around so that a low reading
  // plays a lower note.
  //
  int pot = analogRead(A0);
  int pot_delay = 1024-pot+5;
#else
  int pot_delay = 70;
#endif

  // We loop through the values (0 to 255) in steps of wave_step
  // in the waveform, once every time this loop operates.
  //
  // For a wave_step = 8, this means that the frequency
  // will be the frequency of this loop divided by 32.
  //
  // For a wave_step of 16, it is divided by 16.
  //
  // If we want to output a wave playing a "concert A"
  // at 440Hz, we need a cycle time of
  //      1/440 = 0.002272 or 2.27mS
  //
  // This loop therefore has to run 256/wave_step times
  // faster than that.
  // For wave_step= 8, which would mean it needs to run at
  //    2.27/32 = 0.071
  // or approximately every 71 microseconds to play 440Hz A.
  //
  // Note the rest of the Arduino code will take a bit
  // of time to run as well, and it is quite a bit slower
  // if reading from the analog potentiometer each time
  // as well.
  //
  // Typically an arduino can support an accurate delay
  // of around 3 microseconds and up using this method.
  //
  delayMicroseconds(pot_delay);

  // This will automatically wrap around from 255 back to 0
  // So it will create a simple saw-tooth waveform.
  wave += wave_step;
  ddsOutput(wave);
}

void ddsOutput (uint8_t value) {
  uint8_t pd;
  uint8_t pb;

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
  pd = value & 0b11111100;
  pb = value & 0b00000011;
  PORTD = pd;
  PORTB = pb;
}