/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Xiao Simple USB MIDI Merge
//  https://diyelectromusic.wordpress.com/2023/03/27/xiao-samd21-arduino-and-midi-part-4/
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
  Using principles from the following tutorials:
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Xiao Getting Started - https://wiki.seeedstudio.com/Seeeduino-XIAO/
    Samd Serial SERCOM   - https://docs.arduino.cc/tutorials/communication/SamdSercom
*/
#include <Arduino.h>
#include "wiring_private.h"
#include <MIDI.h>
#include <USB-MIDI.h>

#define MIDI_LED LED_BUILTIN
#define LED_COUNT 200
int ledcount;

void ledInit() {
#ifdef MIDI_LED
  pinMode(MIDI_LED, OUTPUT);
#endif
  ledcount = 0;
}

void ledOn() {
#ifdef MIDI_LED
  // Xiao on board LED is reversed
  digitalWrite(MIDI_LED, LOW);
#endif
  ledcount = LED_COUNT;
}

void ledOff() {
#ifdef MIDI_LED
  // Xiao on board LED is reversed
  digitalWrite(MIDI_LED, HIGH);
#endif
}

void ledScan () {
  if (ledcount > 0) {
    ledcount--;
  } else {
    ledOff();
    ledcount = 0;
  }
}

//---------------------------------------------------------
//
// Initialise USB for MIDI.
// Use this for MIDI IN and OUT.
// Note: does not enable MIDI THRU for USB.
//
//---------------------------------------------------------
USBMIDI_CREATE_INSTANCE(0, UMIDI);


//---------------------------------------------------------
//
// Initialise default serial port for MIDI.
//
//---------------------------------------------------------
MIDI_CREATE_DEFAULT_INSTANCE();


//---------------------------------------------------------
//
// Create the second hardware serial port using SERCOM2 on pins 4,5
//
//---------------------------------------------------------
Uart mySerial1 (&sercom2, A5, A4, SERCOM_RX_PAD_1, UART_TX_PAD_0);

void mySerial1Init () {
  mySerial1.begin(9600);
  pinPeripheral(A5, PIO_SERCOM); //Assign RX function to pin 5
  pinPeripheral(A4, PIO_SERCOM); //Assign TX function to pin 4
}

// Attach the interrupt handler to the SERCOM
void SERCOM2_Handler() {
  mySerial1.IrqHandler();
}

MIDI_CREATE_INSTANCE(HardwareSerial, mySerial1, MIDI1);

//---------------------------------------------------------
//
// Create the third hardware serial port using SERCOM0 on pins 9,10
//
//---------------------------------------------------------
Uart mySerial2 (&sercom0, A9, A10, SERCOM_RX_PAD_1, UART_TX_PAD_2);

void mySerial2Init () {
  mySerial2.begin(9600);
  pinPeripheral(A9, PIO_SERCOM);  //Assign RX function to pin 9
  pinPeripheral(A10, PIO_SERCOM); //Assign TX function to pin 10
}

// Attach the interrupt handler to the SERCOM
void SERCOM0_Handler() {
  mySerial2.IrqHandler();
}

MIDI_CREATE_INSTANCE(HardwareSerial, mySerial2, MIDI2);

//---------------------------------------------------------
//
// Main setup and loop code
//
//---------------------------------------------------------
void setup()
{
  mySerial2Init();

  // Initialise each MIDI channel and ensure automatic THRU is off
  UMIDI.begin(MIDI_CHANNEL_OMNI);
  UMIDI.turnThruOff();
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  MIDI1.begin(MIDI_CHANNEL_OMNI);
  MIDI1.turnThruOff();
  MIDI2.begin(MIDI_CHANNEL_OMNI);
  MIDI2.turnThruOff();
}

void midiThru(int in, int out, midi::MidiType cmd, midi::DataByte d1, midi::DataByte d2, midi::Channel ch) {
  // Ignore "Active Sensing" and SysEx messages
  if ((cmd != 0xFE) && (cmd != 0xF0)) {
    ledOn();
    if (out == 1) {
      MIDI.send(cmd, d1, d2, ch);
    } else if (out == 2) {
      MIDI1.send(cmd, d1, d2, ch);
    } else if (out == 3) {
      MIDI2.send(cmd, d1, d2, ch);
    } else {
      UMIDI.send(cmd, d1, d2, ch);      
    }
  }
}

void loop()
{
  ledScan();

  // MIDI Ports:
  //   0 = USB MIDI
  //   1 = Default hardware serial port
  //   2 = First additional serial port
  //   3 = Second additional serial port

  // Scan USB MIDI
  if (UMIDI.read()) {
    // Pass on anything received via USB (0) to all three serial ports (1,2,3)
    for (int i=1; i<=3; i++) {
      midiThru(0, i, UMIDI.getType(),
                     UMIDI.getData1(),
                     UMIDI.getData2(),
                     UMIDI.getChannel());
    }
  }

  // Scan MIDI on default HardwareSerial pass to USB
  if (MIDI.read()) {
    midiThru(1, 0, MIDI.getType(),
                   MIDI.getData1(),
                   MIDI.getData2(),
                   MIDI.getChannel());
  }

  // Scan MIDI1 on mySerial1 pass to USB
  if (MIDI1.read())
  {
    midiThru(2, 0, MIDI1.getType(),
                   MIDI1.getData1(),
                   MIDI1.getData2(),
                   MIDI1.getChannel());
  }

  // Scan MIDI2 on mySerial2 pass to USB
  if (MIDI2.read())
  {
    midiThru(3, 0, MIDI2.getType(),
                   MIDI2.getData1(),
                   MIDI2.getData2(),
                   MIDI2.getChannel());
  }
}
