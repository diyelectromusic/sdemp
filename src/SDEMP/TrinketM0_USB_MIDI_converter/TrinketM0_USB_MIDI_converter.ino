/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Trinket M0 USB MIDI To MIDI Revisited
//  https://diyelectromusic.wordpress.com/2021/03/11/usb-midi-to-midi-revisited/
//
// Based on the original USB_Midi_Converter example from the SAMD USB Host Library.
*/
/*
 *******************************************************************************
 * USB-MIDI to Legacy Serial MIDI converter
 * Copyright (C) 2012-2017 Yuuichi Akagawa
 *
 * Idea from LPK25 USB-MIDI to Serial MIDI converter
 *   by Collin Cunningham - makezine.com, narbotic.com
 *
 * This is sample program. Do not expect perfect behavior.
 *******************************************************************************
 */

#include <usbh_midi.h>
#include <usbhub.h>

#ifdef ADAFRUIT_TRINKET_M0
// setup Dotstar LED on Trinket M0
#include <Adafruit_DotStar.h>
#define DATAPIN    7
#define CLOCKPIN   8
Adafruit_DotStar strip = Adafruit_DotStar(1, DATAPIN, CLOCKPIN, DOTSTAR_BRG);
#endif

#define MIDI_LED LED_BUILTIN

#ifdef USBCON
#define _MIDI_SERIAL_PORT Serial1
#else
#define _MIDI_SERIAL_PORT Serial
#endif
//////////////////////////
// MIDI Pin assign
// 2 : GND
// 4 : +5V(Vcc) with 220ohm
// 5 : TX
//////////////////////////

USBHost UsbH;
USBH_MIDI  Midi(&UsbH);

void MIDI_poll();
void doDelay(uint32_t t1, uint32_t t2, uint32_t delayTime);

void midiLedOn() {
#ifdef MIDI_LED
   digitalWrite(MIDI_LED, HIGH);
#endif
}

void midiLedOff() {
#ifdef MIDI_LED
   digitalWrite(MIDI_LED, LOW);
#endif
}

void setup()
{
  // Turn off built-in RED LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#ifdef MIDI_LED
  pinMode(MIDI_LED, OUTPUT);
  midiLedOff();
#endif
#ifdef ADAFRUIT_TRINKET_M0
  // Turn off built-in Dotstar RGB LED
  strip.begin();
  strip.clear();
  strip.show();
#endif

  _MIDI_SERIAL_PORT.begin(31250);

  if (UsbH.Init()) {
    while (1); //halt
  }
  delay( 200 );
}

void loop()
{
  UsbH.Task();
  uint32_t t1 = (uint32_t)micros();
  if ( Midi ) {
    MIDI_poll();
  }
  //delay(1ms)
  doDelay(t1, (uint32_t)micros(), 1000);
  midiLedOff();
}

// Poll USB MIDI Controler and send to serial MIDI
void MIDI_poll()
{
  uint8_t outBuf[ 3 ];
  uint8_t size;

  do {
    if ( (size = Midi.RecvData(outBuf)) > 0 ) {
      midiLedOn();
      //MIDI Output
      _MIDI_SERIAL_PORT.write(outBuf, size);
    }
  } while (size > 0);
}

// Delay time (max 16383 us)
void doDelay(uint32_t t1, uint32_t t2, uint32_t delayTime)
{
  uint32_t t3;

    t3 = t2 - t1;
  if ( t3 < delayTime ) {
    delayMicroseconds(delayTime - t3);
  }
}
