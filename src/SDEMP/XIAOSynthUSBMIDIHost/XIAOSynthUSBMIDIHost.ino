/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Xiao Synth USB MIDI Host
//  https://diyelectromusic.com/2025/06/27/xiao-esp32-c3-midi-synthesizer-part-4/
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
    XIAO Synth - https://github.com/FortySevenEffects/arduino_midi_library
    XIAO Getting Started - https://wiki.seeedstudio.com/Seeeduino-XIAO/
    USBH SAMD Example - USB_MIDI_Converter
*/
#include <usbh_midi.h>
#include <usbhub.h>

USBHost UsbH;
USBH_MIDI  UsbMidi(&UsbH);

// IO pins assumed in the following order:
//   Program Up
//   Program Down
//   Volume Up
//   Volume Down
//
// IO buttons for the XIAO Synth have external pull-ups
int ioPins[4] = {0,1,2,3};
int ioLast[4];

// Destination for the UI/IO actions
#define MIDI_CHANNEL 1  // 1..16 only
#define MIDI_PC  0xC0
#define MIDI_CC  0xB0
#define MIDI_VOL 0x07   // MIDI Channel Volume CC
uint8_t program;
uint8_t volume;

void midiSendProgram (uint8_t prog) {
  uint8_t buf[2];
  buf[0] = MIDI_PC | (MIDI_CHANNEL-1);
  buf[1] = prog;
  Serial1.write(buf, 2);
}

void midiSendControl (uint8_t cmd, uint8_t d) {
  uint8_t buf[3];
  buf[0] = MIDI_CC | (MIDI_CHANNEL-1);
  buf[1] = cmd;
  buf[2] = d;
  Serial1.write(buf, 3);
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
  Serial1.begin(31250);

  if (UsbH.Init()) {
    while (1); //halt
  }
  delay( 200 );

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
  // Run every 1mS
  while (lasttime > micros()) {
    // Wait for a bit
  }
  lasttime = micros() + 1000;

  // USB handling
  UsbH.Task();

  // MIDI scanning
  uint8_t buf[3];
  uint8_t size;
  do {
    if ( (size = UsbMidi.RecvData(buf)) > 0 ) {
      Serial1.write(buf, size);
    }
  } while (size > 0);

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
