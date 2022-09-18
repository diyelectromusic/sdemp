/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino VS1053 MIDI GM Synth - Part 2
//  https://diyelectromusic.wordpress.com/2022/09/18/arduino-vs1053-general-midi-synth-part-2/
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
  Using principles from the following Arduino tutorials:
    Arduino MIDI Library    - https://github.com/FortySevenEffects/arduino_midi_library
    Rotary Encoder Library  - https://github.com/mathertel/RotaryEncoder
    Arduino 7-Segment Tutorial - https://www.circuitbasics.com/arduino-7-segment-display-tutorial/
    VS10xx software patches - http://www.vlsi.fi/en/support/software/vs10xxpatches.html
    VS10xx applications     - http://www.vlsi.fi/en/support/software/vs10xxapplications.html

  This uses a generic VS1003 or VS1053 MP3 Player board utilising
  VS10xx software patching to provide a robust "real time MIDI" mode
  with MIDI access provided over the SPI bus.

  If you want an easier time of things, be sure to get a recognised
  board that supports the VS1053 more completely such as those from
  Adafruit or Sparkfun.

  But this code is pretty universal on non-name VS1003 and VS1053
  based modules.
*/
#include <SPI.h>
#include <MIDI.h>
#include <SevSeg.h>
#include <RotaryEncoder.h>
#include "vs10xx_uc.h" // From VLSI website: http://www.vlsi.fi/en/support/software/microcontrollersoftware.html
extern "C" {
#include "rtmidi1053b.h"
}

// Uncomment this to print to the Serial port instead of sending MIDI
//#define TEST 1

// This is required to set up the MIDI library.
// The default MIDI setup uses the Arduino built-in serial port
// which is pin 1 for transmitting on the Arduino Uno.
MIDI_CREATE_DEFAULT_INSTANCE();

// Define which MIDI channel to transmit on (1 to 16).
// If set to zero it will send on all channels apart from 10.
#define MIDI_CHANNEL 1
#define MIDI_PROGRAM_CHANGE 0xC0

// Rotary Encoder
#define RE_A  A1  // "CLK"
#define RE_B  A0  // "DT"
//#define RE_SW A2  // "SW" - Not being used
RotaryEncoder encoder(RE_A, RE_B, RotaryEncoder::LatchMode::FOUR3);

// Set up the LED display
SevSeg disp;

// Pin definitions for the display
//
//   = A =
//  F     B
//   = G =
//  E     C
//   = D =
//
// Pattern repeated for three digits (1,2,3) each with a common cathode.
// Note: Cathode should be connected to the "digit" pins via a resistor (1k).
//
// Digit Order: D1-D2-D3-D4
//
// Pinout of my 12-pin display.
//
// D1-A--F-D2-D3--B
// |              |
// E--D-DP--C--G-D4
//
#define NUM_DIGITS   2
#define NUM_SEGMENTS 7
byte digitPins[NUM_DIGITS]     = {4,3};           // Order: D3,D4 (D1, D2 not used)
byte segmentPins[NUM_SEGMENTS] = {5,10,A3,A4,A5,A2,1}; // Order: A,B,C,D,E,F,G (DP not used)
#define RESISTORS_ON_SEGMENTS false          // Resistors on segments (true) or digits (false)
#define HARDWARE_CONFIG       COMMON_CATHODE // COMMON_CATHODE or COMMON_ANODE (or others)
#define UPDATE_WITH_DELAYS    false          // false recommended apparently
#define LEADING_ZEROS         true           // Set true to show leading zeros
#define NO_DECIMAL_POINT      true           // Set true if no DP or not connected

int prognum;

// VS1053 Shield pin definitions
#define VS_XCS    6 // Control Chip Select Pin (for accessing SPI Control/Status registers)
#define VS_XDCS   7 // Data Chip Select / BSYNC Pin
#define VS_DREQ   2 // Data Request Pin: Player asks for more data
#define VS_RESET  8 // Reset is active low

// VS10xx SPI pin connections
// Provided here for info only - not used in the sketch as the SPI library handles this
#define VS_SS     10
#define VS_MOSI   11
#define VS_MISO   12
#define VS_SCK    13

// There are three selectable sound banks on the VS1053
// These can be selected using the MIDI command 0xBn 0x00 bank
#define DEFAULT_SOUND_BANK 0x00  // General MIDI 1 sound bank
#define DRUM_SOUND_BANK    0x78  // Drums
#define ISNTR_SOUND_BANK   0x79  // General MIDI 2 sound bank

// ------------------------------------------------------
//   The display needs to be updated continually, which
//   means that we can't use delay() anywhere in the code
//   without interrupting the display.
//
//   This function can be used as a replacement for the
//   delay() function which will do the "waiting" whilst
//   at the same time keeping the display updated.
//
// ------------------------------------------------------
void delayWithDisplay(unsigned long mS) {
  unsigned long end_mS = mS + millis();
  unsigned long time_mS  = millis();
  while (time_mS < end_mS) {
    disp.refreshDisplay();
    time_mS = millis();
  }
}

void setup() {
#ifdef TEST
  Serial.begin(9600);
#else
  // Initialise MIDI - listening on all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
#endif
  UCSR0B &= ~(1<<TXEN0);
  initialiseVS10xx();

  // Initialise the 7-Segment display
  disp.begin(HARDWARE_CONFIG, NUM_DIGITS, digitPins, segmentPins,
             RESISTORS_ON_SEGMENTS, UPDATE_WITH_DELAYS, LEADING_ZEROS, NO_DECIMAL_POINT);
  disp.setBrightness(90);

  prognum = 0;
  updateProgDisplay(prognum);
}

int re_pos = 0;
void loop() {
  midiScan();

  // Note: Need to keep the display updated continually.
  //       This means we cannot use delay(), so have to use
  //       the replacement delayWithDisplay()
  //
  disp.refreshDisplay();

  // Read the rotary encoder
  encoder.tick();

  int new_pos = encoder.getPosition();
  if (new_pos != re_pos) {
    int re_dir = (int)encoder.getDirection();
    if (re_dir < 0) {
      prognum--;
      prognum &= 0x7F;
    } else if (re_dir > 0) {
      prognum++;
      prognum &= 0x7F;
    } else {
      // if re_dir == 0; do nothing
    }
    re_pos = new_pos;
    updateProgDisplay(prognum);
    updateProgNum (prognum);
  }
}

void updateProgDisplay (int progNum) {
  // Change to user-friendly 1 to 128 range
//  disp.setNumber(progNum+1,0);
  // Show 0 to 127 in HEX
  disp.setNumber(progNum,0,1);
}

void updateProgNum (int progNum) {
#ifdef TEST
  Serial.print ("Program Change: ");
  Serial.println (progNum);
#else
  // MIDI programs are defined as 1 to 128, but "on the wire"
  // have to be converted to 0 to 127.
#if (MIDI_CHANNEL == 0)
  // Send on all channels apart from 10 (which is the drum channel)
  for (int ch=1; ch<=16; ch++) {
    if (ch == 10) {
      // skip
    } else {
      talkMIDI (MIDI_PROGRAM_CHANGE, progNum, 0, ch);
    }
  }
#else
  talkMIDI (MIDI_PROGRAM_CHANGE, progNum, 0, MIDI_CHANNEL);
#endif
#endif
}

void midiScan () {
  // If we have MIDI data then forward it on.
  if (MIDI.read()) {
    midi::MidiType mt  = MIDI.getType();
    if (mt == midi::SystemExclusive) {
      // Ignore SysEx messages
    } else {
      talkMIDI(MIDI.getType(), MIDI.getData1(), MIDI.getData2(), MIDI.getChannel());
    }
  }
}

/***********************************************************************************************
 * 
 * Here is the code to send MIDI data to the VS1053 over the SPI bus.
 *
 * Taken from MP3_Shield_RealtimeMIDI.ino by Matthias Neeracher
 * which was based on Nathan Seidle's Sparkfun Electronics example code for the Sparkfun 
 * MP3 Player and Music Instrument shields and and VS1053 breakout board.
 *
 ***********************************************************************************************
 */
void sendMIDI(byte data) {
  SPI.transfer(0);
  SPI.transfer(data);
}

void talkMIDI(byte cmd, byte data1, byte data2, byte ch) {
  //
  // Wait for chip to be ready (Unlikely to be an issue with real time MIDI)
  //
  while (!digitalRead(VS_DREQ)) {
  }
  digitalWrite(VS_XDCS, LOW);

  if (cmd >= 0xF0) {
    // Ignore all system messages
    return;
  }
  sendMIDI(cmd | (ch-1));
  
  //Some commands only have one data byte. All cmds less than 0xBn have 2 data bytes 
  //(sort of: http://253.ccarh.org/handout/midiprotocol/)
  // SysEx messages (0xF0 onwards) are variable length but not supported at present!
  if( (cmd & 0xF0) <= 0xB0 || (cmd & 0xF0) >= 0xE0) {
    sendMIDI(data1);
    sendMIDI(data2);
  } else {
    sendMIDI(data1);
  }

  digitalWrite(VS_XDCS, HIGH);
}


/***********************************************************************************************
 * 
 * Code from here on is the magic required to initialise the VS10xx and 
 * put it into real-time MIDI mode using an SPI-delivered patch.
 * 
 * Here be dragons...
 * 
 * Based on VS1003b/VS1033c/VS1053b Real-Time MIDI Input Application
 * http://www.vlsi.fi/en/support/software/vs10xxapplications.html
 *
 * With some input from MP3_Shield_RealtimeMIDI.ino by Matthias Neeracher
 * which was based on Nathan Seidle's Sparkfun Electronics example code for the Sparkfun 
 * MP3 Player and Music Instrument shields and and VS1053 breakout board.
 * 
 ***********************************************************************************************
 */

void initialiseVS10xx () {
  // Set up the pins controller the SPI link to the VS1053
  pinMode(VS_DREQ, INPUT);
  pinMode(VS_XCS, OUTPUT);
  pinMode(VS_XDCS, OUTPUT);
  pinMode(VS_RESET, OUTPUT);

  // Setup SPI
  // The Arduino's Slave Select pin is only required if the
  // Arduino is acting as an SPI slave device.
  // However, the internal circuitry for the ATmeta328 says
  // that if the SS pin is low, the MOSI/MISO lines are disabled.
  // This means that when acting as an SPI master (as in this case)
  // the SS pin must be set to an OUTPUT to prevent the SPI lines
  // being automatically disabled in hardware.
  // We can still use it as an OUTPUT IO pin however as the value
  // (HIGH or LOW) is not significant - it just needs to be an OUTPUT.
  // See: http://www.gammon.com.au/spi
  //
  pinMode(VS_SS, OUTPUT);

  // Now initialise the VS10xx
  digitalWrite(VS_XCS, HIGH);  //Deselect Control
  digitalWrite(VS_XDCS, HIGH); //Deselect Data
  digitalWrite(VS_RESET, LOW); //Put VS1053 into hardware reset

  // And then bring up SPI
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);

  //From page 12 of datasheet, max SCI reads are CLKI/7. Input clock is 12.288MHz. 
  //Internal clock multiplier is 1.0x after power up. 
  //Therefore, max SPI speed is 1.75MHz. We will use 1MHz to be safe.
  SPI.setClockDivider(SPI_CLOCK_DIV16); //Set SPI bus speed to 1MHz (16MHz / 16 = 1MHz)
  SPI.transfer(0xFF); //Throw a dummy byte at the bus

  delayMicroseconds(1);
  digitalWrite(VS_RESET, HIGH); //Bring up VS1053

  // Dummy read to ensure VS SPI bus in a known state
  VSReadRegister(SCI_MODE);

  // Perform software reset and initialise VS mode
  VSWriteRegister16(SCI_MODE, SM_SDINEW|SM_RESET);
  delay(200);

  // Enable real-time MIDI mode
  VSLoadUserCode();
}

// This sends a special sequence of bytes to the device to 
// get it to output a test sine wave.
//
// See the datasheets for details.
//
void VSSineTest () {
  VSWriteRegister16(SCI_MODE, SM_SDINEW|SM_RESET|SM_TESTS);
  delay(100);
  while(!digitalRead(VS_DREQ)) ; //Wait for DREQ to go high indicating IC is available
  digitalWrite(VS_XDCS, LOW); //Select control

  //Special 8-byte sequence to trigger a sine test
  SPI.transfer(0x53);
  SPI.transfer(0xef);
  SPI.transfer(0x6e);
  SPI.transfer(0x63);  // FIdx(7:5)=b011; S(4:0)=b00011 ==> F= ~517Hz
  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(0);
  while(!digitalRead(VS_DREQ)) ; //Wait for DREQ to go high indicating command is complete
  digitalWrite(VS_XDCS, HIGH); //Deselect Control

  delay (2000);

  while(!digitalRead(VS_DREQ)) ; //Wait for DREQ to go high indicating IC is available
  digitalWrite(VS_XDCS, LOW); //Select control

  //Special 8-byte sequence to disable the sine test
  SPI.transfer(0x45);
  SPI.transfer(0x78);
  SPI.transfer(0x69);
  SPI.transfer(0x74);
  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(0);
  while(!digitalRead(VS_DREQ)) ; //Wait for DREQ to go high indicating command is complete
  digitalWrite(VS_XDCS, HIGH); //Deselect Control

  delay(100);
  VSWriteRegister16(SCI_MODE, SM_SDINEW|SM_RESET);
  delay(200);
}

// Write to VS10xx register
// SCI: Data transfers are always 16bit. When a new SCI operation comes in 
// DREQ goes low. We then have to wait for DREQ to go high again.
// XCS should be low for the full duration of operation.
//
void VSWriteRegister(unsigned char addressbyte, unsigned char highbyte, unsigned char lowbyte){
  while(!digitalRead(VS_DREQ)) ; //Wait for DREQ to go high indicating IC is available
  digitalWrite(VS_XCS, LOW); //Select control

  //SCI consists of instruction byte, address byte, and 16-bit data word.
  SPI.transfer(0x02); //Write instruction
  SPI.transfer(addressbyte);
  SPI.transfer(highbyte);
  SPI.transfer(lowbyte);
  while(!digitalRead(VS_DREQ)) ; //Wait for DREQ to go high indicating command is complete
  digitalWrite(VS_XCS, HIGH); //Deselect Control
}

// 16-bit interface to the above function.
//
void VSWriteRegister16 (unsigned char addressbyte, uint16_t value) {
  VSWriteRegister (addressbyte, value>>8, value&0xFF);
}

// Read a VS10xx register using the SCI (SPI command) bus.
//
uint16_t VSReadRegister(unsigned char addressbyte) {
  while(!digitalRead(VS_DREQ)) ; //Wait for DREQ to go high indicating IC is available
  digitalWrite(VS_XCS, LOW); //Select control
  
  SPI.transfer(0x03); //Read instruction
  SPI.transfer(addressbyte);
  delayMicroseconds(10);
  uint8_t d1 = SPI.transfer(0x00);
  uint8_t d2 = SPI.transfer(0x00);
  while(!digitalRead(VS_DREQ)) ; //Wait for DREQ to go high indicating command is complete
  digitalWrite(VS_XCS, HIGH); //Deselect control

  return ((d1<<8) | d2);
}

// Load a user plug-in over SPI.
//
// See the application and plug-in notes on the VLSI website for details.
//
void VSLoadUserCode(void) {
#ifdef TEST
  Serial.print("Loading User Code");
#endif
  for (int i=0; i<VS10xx_CODE_SIZE; i++) {
    uint8_t addr = pgm_read_byte_near(&vs10xx_atab[i]);
    uint16_t dat = pgm_read_word_near(&vs10xx_dtab[i]);
#ifdef TEST
    if (!(i%8)) Serial.print(".");
//    sprintf(teststr, "%4d --> 0x%04X => 0x%02x\n", i, dat, addr);
//    Serial.print(teststr);
#endif
    VSWriteRegister16 (addr, dat);
  }

  // Set the start address of the application code (see rtmidi.pdf application note)
  VSWriteRegister16(SCI_AIADDR, 0x50);

#ifdef TEST
  Serial.print("Done\n");
#endif
}
