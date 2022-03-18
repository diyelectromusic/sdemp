/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  I2CMIDI to Serial MIDI Latency
//  https://diyelectromusic.wordpress.com/2022/01/18/raspberry-pi-pico-i2c-midi-interface-part-3/
//
      MIT License
      
      Copyright (c) 2022 diyelectromusic (Kevin)
      
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
  Using principles from the following:
     Arduino MIDI Library: https://github.com/FortySevenEffects/arduino_midi_library
     Master Reader/Slave Sender: https://www.arduino.cc/en/Tutorial/LibraryExamples/MasterReader
     I2CMIDI Transport: https://github.com/diyelectromusic/I2CMIDI
*/
#include <MIDI.h>
#include <I2CMIDI.h>

#include <SoftwareSerial.h>
// From https://www.arduino.cc/en/Reference/softwareSerial
// This default configurationis not supported on ATmega32U4
// or ATmega2560 based boards, see MIDI_SW_SERIAL2.
#define SS_RX  2
#define SS_TX  3
using Transport = MIDI_NAMESPACE::SerialMIDI<SoftwareSerial>;
SoftwareSerial sSerial = SoftwareSerial(SS_RX, SS_TX);
Transport serialMIDI(sSerial);
MIDI_NAMESPACE::MidiInterface<Transport> MIDI((Transport&)serialMIDI);

#define MIDI_CHANNEL  1  // MIDI Transmit channel choose 1-16
#define I2CMIDIADDR   0x40
I2CMIDI_CREATE_INSTANCE(I2CMIDIPERIPHERAL, I2CMIDIADDR, I2CMIDI);

#define MIDI_LED LED_BUILTIN

void setup() {
  pinMode(MIDI_LED, OUTPUT);
  MIDI.begin(MIDI_CHANNEL);
  MIDI.turnThruOff();
  I2CMIDI.begin(MIDI_CHANNEL_OMNI);
  I2CMIDI.turnThruOff();
  Serial.begin(9600);
  Serial.println("MIDI Latency Test: MIDI OUT over serial, MIDI IN over I2C");
  Serial.print("Serial MIDI TX on pin 3 on MIDI channel ");
  Serial.print(MIDI_CHANNEL);
  Serial.print("\n\n");
}

int ledCount;
void ledOn() {
  digitalWrite(MIDI_LED, HIGH);
}

void ledOff() {
  digitalWrite(MIDI_LED, LOW);
}

unsigned long mstime;
int state;
void loop() {
  unsigned long newmstime=0;
  if (state == 0) {
    Serial.print("Starting test... ");
    ledOn();
    MIDI.send(midi::NoteOff, 60, 0, MIDI_CHANNEL);
    state++;
    mstime = micros();
  } else if (state == 1) {
    if (I2CMIDI.read())
    {
      midi::MidiType cmd = I2CMIDI.getType();
      midi::DataByte d1 = I2CMIDI.getData1();
      midi::DataByte d2 = I2CMIDI.getData2();
      midi::Channel ch = I2CMIDI.getChannel();
      if ((ch == MIDI_CHANNEL) &&
          (cmd == midi::NoteOff) &&
          (d1 == 60) &&
          (d2 == 0)) {
        newmstime = micros();
        ledOff();
        Serial.print("Round Trip Time = ");
        Serial.print(newmstime-mstime);
        Serial.println(" uS");
        state++;
      }
    }
  } else {
    newmstime = micros();
    if (newmstime > mstime+3000000) {
      state = 0;
    }
  }
}
