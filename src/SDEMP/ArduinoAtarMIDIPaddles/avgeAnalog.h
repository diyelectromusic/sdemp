/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Averaging Analog Read
//  https://diyelectromusic.com/2025/06/23/arduino-midi-atari-paddles/
//
      MIT License
      
      Copyright (c) 2025 diyelectromusic (Kevin)
      
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
#ifndef _avgeAnalog_h
#define _avgeAnalog_h

//--------------------------------------------------
// Analog Averaging General Case
//--------------------------------------------------
#define AVGEREADINGS 32

class AvgePot {
public:
  AvgePot(int pot) {
    potpin = pot;
    for (int i=0; i<AVGEREADINGS; i++) {
      avgepotvals[i] = 0;
    }
    avgepotidx = 0;
    avgepottotal = 0;
  }

  int getPin () {
    return potpin;
  }

  // Implement an averaging routine.
  // NB: All values are x10 to allow for 1
  //     decimal place of accuracy to allow
  //     "round to nearest" rather the default
  //     "round down" calculations, which gives
  //     a much more stable result.
  unsigned avgeAnalogRead () {
    unsigned reading = 10*avgeReader(potpin);
    avgepottotal = avgepottotal - avgepotvals[avgepotidx];
    avgepotvals[avgepotidx] = reading;
    avgepottotal = avgepottotal + reading;
    avgepotidx++;
    if (avgepotidx >= AVGEREADINGS) avgepotidx = 0;
    return (((avgepottotal / AVGEREADINGS) + 5) / 10);
  }

  unsigned avgeAnalogRead (int pot) {
    if (pot == potpin) {
      return avgeAnalogRead();
    }
    return 0;
  }

  // Default reader is the standard Arduino analogRead() function.
  // Override this for bespoke analogRead() implementations.
  virtual unsigned avgeReader (int potpin) {
    return analogRead(potpin);
  }

private:
  int potpin;
  unsigned avgepotvals[AVGEREADINGS];
  unsigned avgepotidx;
  unsigned long avgepottotal;
};

#endif