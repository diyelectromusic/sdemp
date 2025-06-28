/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Xiao Synth Serial MIDI
//  https://diyelectromusic.com/2025/06/28/xiao-esp32-c3-midi-synthesizer-part-5/
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
/*
  Using principles from the following tutorials:
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    XIAO Getting Started - https://wiki.seeedstudio.com/Seeeduino-XIAO/
*/
#include <MIDI.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial0, MIDI);

// IO pins assumed in the following order:
//   Program Up
//   Program Down
//   Volume Up
//   Volume Down
//
// IO buttons for the XIAO Synth have external pull-ups
int ioPins[4] = {D0,D1,D2,D3};  // Probably works, try following if not
//int ioPins[4] = {0,1,2,3};  // XIAO SAMD21
//int ioPins[4] = {5,4,3,2};  // XIAO ESP32-C3
int ioLast[4];

// Destination for the UI/IO actions
#define MIDI_CHANNEL 1  // 1..16 only
#define MIDI_VOL 0x07   // MIDI Channel Volume CC
uint8_t program;
uint8_t volume;

void midiSendProgram (uint8_t prog) {
  MIDI.sendProgramChange(prog, MIDI_CHANNEL);
}

void midiSendControl (uint8_t cmd, uint8_t d) {
  MIDI.sendControlChange(cmd, d, MIDI_CHANNEL);
}

void programUp () {
  if (program < 127) {
    program++;
  } else {
    program = 0;
  }
  midiSendProgram(program);
}

void programDown () {
  if (program > 0) {
    program--;
  } else {
    program = 127;
  }
  midiSendProgram(program);
}

void volumeUp () {
  volume += 10;
  if (volume > 120) volume = 120;
  midiSendControl(MIDI_VOL, volume);
}

void volumeDown () {
  if (volume > 10) {
    volume -= 10;
  } else {
    volume = 0;
  }
  midiSendControl(MIDI_VOL, volume);
}

void buttonPressed (int b) {
  switch (b) {
    case 0:
      programUp();
      break;
    case 1:
      programDown();
      break;
    case 2:
      volumeUp();
      break;
    case 3:
      volumeDown();
      break;
    default:
      break;
  }
}

void buttonReleased (int b) {
  switch (b) {
    case 0:
      break;
    case 1:
      break;
    case 2:
      break;
    case 3:
      break;
    default:
      break;
  }
}

void buttonHeld (int b) {
  switch (b) {
    case 0:
      break;
    case 1:
      break;
    case 2:
      break;
    case 3:
      break;
    default:
      break;
  }
}

void setup()
{
  MIDI.begin(MIDI_CHANNEL);

  for (int i=0; i<4; i++) {
    // There are external pull-ups
    pinMode(ioPins[i], INPUT);
    ioLast[i] = HIGH;
  }

  program = 0;
  midiSendProgram(program);
  volume = 100;
  midiSendControl(MIDI_VOL, volume);
}

unsigned long lasttime;
void loop()
{
  // Performs automatic MIDI THRU on UART
  MIDI.read();

  // UI/IO handling
  for (int i=0; i<4; i++) {
    int ioNew = digitalRead(ioPins[i]);
    if ((ioNew == LOW) && (ioLast[i] == HIGH)) {
      buttonPressed(i);
    } else if ((ioNew == HIGH) && (ioLast[i] == LOW)) {
      buttonReleased(i);
    } else if ((ioNew == LOW) && (ioLast[i] == LOW)) {
      buttonHeld(i);
    } else {
      // Nothing
    }
    ioLast[i] = ioNew;
  }
}
