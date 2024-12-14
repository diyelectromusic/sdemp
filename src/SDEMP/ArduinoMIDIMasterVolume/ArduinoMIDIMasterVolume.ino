/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Master Volume
//  https://diyelectromusic.com/2024/12/14/arduino-midi-master-volume-control/
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
  Using principles from the following tutorials:
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
*/
#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

#define VOL_POT A0
#define MAX_POT 1023  // 10-bit analog reading
//#define MAX_POT 4095  // 12-bit analog reading
int lastvolume;

void setMasterVolume (int volume) { 
  // Master Volume is set using a MIDI SysEx message as follows:
  //   F0  Start of SysEx
  //   7F  System Realtime SysEx
  //   7F  SysEx "channel" - 7F = all devices
  //   04  Device Control
  //   01  Master Volume Device Control
  //   LL  Low 7-bits of 14-bit volume
  //   HH  High 7-bits of 14-bit volume
  //   F7  End SysEx
  //
  //  See MIDI Specification "Device Control"
  //   "Master Volume and Master Balance"
  //
  // Need to scale the volume parameter to fit
  // a 14-bit value: 0..16383
  // and then split into LSB/MSB.
  uint16_t midivol = map(volume, 0, MAX_POT, 0, 16383);
  byte sysex[9] = {0xF0, 0x7F, 0x7F, 0x04, 0x01, 0, 0, 0xF7, 0};
  sysex[5] = midivol & 0x7F; // LSB
  sysex[6] = (midivol >> 7) & 0x7F; // MSB
  MIDI.sendSysEx(8, sysex, true); // true = need F0/F7 in message
}

void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);
  //Serial.begin(115200);
  lastvolume = 0;
}

void loop() {
  // We don't process any MIDI messages, but this allows THRU to function
  MIDI.read();

  int newvol = avgeAnalogRead(VOL_POT);
  if (newvol != lastvolume) {
    setMasterVolume(newvol);
    lastvolume = newvol;
  }
}

#define AVGEREADINGS 32
int avgepotvals[AVGEREADINGS];
int avgepotidx;
long avgepottotal;

// Implement an averaging routine.
// NB: All values are x10 to allow for 1
//     decimal place of accuracy to allow
//     "round to nearest" rather the default
//     "round down" calculations, which gives
//     a much more stable result.
int avgeAnalogRead (int pot) {
  int reading = 10*analogRead(pot);
  avgepottotal = avgepottotal - avgepotvals[avgepotidx];
  avgepotvals[avgepotidx] = reading;
  avgepottotal = avgepottotal + reading;
/* // Can be used to show how the averaging process works
  Serial.print(avgepotidx);
  Serial.print("\t");
  Serial.print(reading);
  for (int i=0; i<AVGEREADINGS; i++) {
    Serial.print("\t");
    Serial.print(avgepotvals[i]);
  }
  Serial.print("\t");
  Serial.print(avgepottotal);
  Serial.print("\t");
  Serial.print(avgepottotal/10);
  Serial.print("\t");
  Serial.println(((avgepottotal / AVGEREADINGS) + 5) / 10);*/
  avgepotidx++;
  if (avgepotidx >= AVGEREADINGS) avgepotidx = 0;
  return (((avgepottotal / AVGEREADINGS) + 5) / 10);
}
