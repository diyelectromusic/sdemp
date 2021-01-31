/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI VS10xx Drum Machine
//  https://diyelectromusic.wordpress.com/2021/01/13/arduino-vs1003-drum-machine/
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
    VS10xx software patches - http://www.vlsi.fi/en/support/software/vs10xxpatches.html
    VS10xx applications     - http://www.vlsi.fi/en/support/software/vs10xxapplications.html
    Arduino Keypad          - https://playground.arduino.cc/Main/KeypadTutorial/

  This uses a generic VS1003 or VS1053 MP3 Player board utilising
  VS10xx software patching to provide a robust "real time MIDI" mode
  with MIDI access provided over the SPI bus.

  If you want an easier time of things, be sure to get a recognised
  board that supports the VS1053 more completely such as those from
  Adafruit or Sparkfun.

  But this code is pretty universal on non-name VS1003 and VS1053
  based modules.
*/
#include <Keypad.h>
#include <SPI.h>
#include "vs10xx_uc.h" // From VLSI website: http://www.vlsi.fi/en/support/software/microcontrollersoftware.html

//#define TEST 1

// Uncomment to perform a test play of the instruments and drums on power up
#define SOUND_CHECK 1

// Uncomment this if you have a test LED
// Note: The LED_BUILTIN for the Uno is on pin 13,
//       which is used below for the VS1003 link.
#define MIDI_LED 6

// This code supports several variants of the VS10xx based shields.
// Choose the apprppriate one here (and comment out the others).
//
// WARNING: VS1053 is untested at present!
//
//#define VS1053_MP3_SHIELD 1
#define VS1003_MODULE 1

#ifdef VS1003_MODULE
extern "C" {
#include "rtmidi1003b.h"
#include "vs1003inst.h"
}
#endif
#ifdef VS1053_MP3_SHIELD
extern "C" {
#include "rtmidi1053b.h"
}
#endif

// Comment out any of these if not in use
#define POT_TEMPO A0 // Tempo control
#define POT_VOL   A1 // Volume control

#define MIDI_CHANNEL 10 // For drums!

#ifdef VS1053_MP3_SHIELD
// VS1053 Shield pin definitions
#define VS_XCS    6 // Control Chip Select Pin (for accessing SPI Control/Status registers)
#define VS_XDCS   7 // Data Chip Select / BSYNC Pin
#define VS_DREQ   2 // Data Request Pin: Player asks for more data
#define VS_RESET  8 // Reset is active low
#endif
#ifdef VS1003_MODULE
// VS1003 Module pin definitions
#define VS_XCS    8 // Control Chip Select Pin (for accessing SPI Control/Status registers)
#define VS_XDCS   9 // Data Chip Select / BSYNC Pin
#define VS_DREQ   7 // Data Request Pin: Player asks for more data
#define VS_RESET  10 // Reset is active low
#endif
// VS10xx SPI pin connections (both boards)
// Provided here for info only - not used in the sketch as the SPI library handles this
#define VS_MOSI   11
#define VS_MISO   12
#define VS_SCK    13
#define VS_SS     10

// Optional - use Digital IO as the power pins
//#define VS_VCC    6
//#define VS_GND    5

// Defaults, but will be overridden by POT settings if enabled
uint16_t volume = 0;
uint16_t tempo  = 120;

// "Keypad" settings for a 4x4 matrix of switches.
//
// Initialisation code based on CustomKeypad example from Keypad
// library - which is the recommended way to test these settings!
//
#define KP_ROWS 4
#define KP_COLS 4
// The key values to return here encode the beat/drum positions
// to make things simpler later on in the code.
//
// We cannot use 0 here though so the positions and drums have
// to be indexed 1 to 4 not 0 to 3.
//
char keys[KP_ROWS][KP_COLS] = {
  {0x14,0x24,0x34,0x44}, // Drum 4, beats 1,2,3,4
  {0x13,0x23,0x33,0x43}, // Drum 3, beats 1,2,3,4
  {0x12,0x22,0x32,0x42}, // Drum 2, beats 1,2,3,4
  {0x11,0x21,0x31,0x41}, // Drum 1, beats 1,2,3,4
};
byte rowPins[KP_ROWS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
byte colPins[KP_COLS] = {A5, A4, A3, A2}; //connect to the column pinouts of the keypad
Keypad kp = Keypad( makeKeymap(keys), rowPins, colPins, KP_ROWS, KP_COLS);

unsigned long nexttick;
int loopstate;

// Define the drum patterns.
// There are four instruments, each controlled by switches in the keypad.
#define BEATS 4
#define DRUMS 4
#ifdef VS1003_MODULE
byte drums[DRUMS]={VS1003_D_BASS,VS1003_D_SNARE,VS1003_D_HHC,VS1003_D_HHO};
#endif
#ifdef VS1053_MP3_SHIELD
byte drums[DRUMS]={36,38,42,46}; // Bass, Snare, HHO, HHC
#endif
byte pattern[BEATS][DRUMS];
int  seqstep; // index in the pattern 0 to BEATS-1

#ifdef TEST
char teststr[32];
#endif

void setup() {
#ifdef TEST
  Serial.begin(9600);
  Serial.println ("Initialising VS10xx");
#endif // TEST

#ifdef VS_VCC
  pinMode(VS_VCC, OUTPUT);
  digitalWrite(VS_VCC, HIGH);
#endif // VS_VCC
#ifdef VS_GND
  pinMode(VS_GND, OUTPUT);
  digitalWrite(VS_GND, LOW);
#endif // VS_GND
#ifdef MIDI_LED
  pinMode(MIDI_LED, OUTPUT);
  // flash the LED on startup
  for (int i=0; i<4; i++) {
    digitalWrite(MIDI_LED, HIGH);
    digitalWrite(MIDI_LED, LOW);
    delay(50);
  }
#endif // MIDI_LED

  // put your setup code here, to run once:
  initialiseVS10xx();

  delay(1000);

#ifdef SOUND_CHECK
  // A test "output" to see if the VS0xx is working ok
  for (byte i=0; i<DRUMS; i++) {
    talkMIDI (0x90|(MIDI_CHANNEL-1), drums[i], 127);
    delay (100);
    talkMIDI (0x80|(MIDI_CHANNEL-1), drums[i], 0);
    delay (200);    
  }
#endif // SOUND_CHECK

  // Set these invalid to trigger a read of the pots
  // (if being used) first time through.
#ifdef POT_TEMPO
  tempo  = -1;
#endif
#ifdef POT_VOL
  volume = -1;
#endif
}

void loop() {
  // Only play the note in the pattern if we've met the criteria for
  // the next "tick" happening.
  unsigned long timenow = millis();
  if (timenow >= nexttick) {
    // Start the clock for the next tick.
    // Take the tempo mark as a "beats per minute" measure...
    // So time (in milliseconds) to the next beat is:
    //    1000 * 60 / tempo
    //
    // This assumes that one rhythm position = one beat.
    //
    nexttick = millis() + (unsigned long)(60000/tempo);

    // Now play the next beat in the pattern
    seqstep++;
    if (seqstep >= BEATS){
      seqstep = 0;
      // Flash the LED every cycle
#ifdef MIDI_LED
     digitalWrite(MIDI_LED, HIGH);
     digitalWrite(MIDI_LED, LOW);
#endif      
    }
    for (int d=0; d<DRUMS; d++) {
      // First send noteOff for all drums
      // It would be nice to only send noteOff for playing drums,
      // but that means both knowing which notes were in the previous
      // step (which we know) but also if anything has changed via
      // the programming (which we don't really know).
      //
      // So although it is more MIDI messages, I just send noteOff
      // for each drum individually here every time.
      //
      talkMIDI (0x80|(MIDI_CHANNEL-1), drums[d], 0);
      
      // Then send noteOns for the next step
      if (pattern[seqstep][d] != 0) {
        talkMIDI (0x90|(MIDI_CHANNEL-1), pattern[seqstep][d], 127);
      }
    }
  }

  // Take it in turns each loop depending on the value of "loopstate" to:
  //    0 - read the keypad
  //    1 - read one of the pots
  //    2 - read the other pot
  //
  // This means we don't spend more time in the loop() than we need to
  // which helps to keep the playing of the sequence as accurate as possible.
  //
  if (loopstate == 0) {
    // Scan the keypad
    char key = kp.getKey();
    if (key) {
      // A keypress has been detected.
      // The key values returned are of the form:
      //    <beat><drum> HEX
      // Where <beat> is from 1 to BEATS
      //   and <drum> is from 1 to DRUMS
      //   in hexadecimal.
      //
      // This means the beat index and drum index
      // can be pulled directly out of the key value.
      int drumidx = (key & 0x0F) - 1;
      int beatidx = (key >> 4) - 1;

      // So toggle this drum
      if (pattern[beatidx][drumidx] != 0) {
        pattern[beatidx][drumidx] = 0;
      } else {
        pattern[beatidx][drumidx] = drums[drumidx];
      }
    }
  }
  else if (loopstate == 1) {
#ifdef POT_TEMPO
    // Read the potentiometer for a value between 0 and 255.
    // This will be converted into a delay to be used to control the tempo.
    int pot1 = 20 + (analogRead (POT_TEMPO) >> 2);
    if (pot1 != tempo) {
      tempo = pot1;  // Tempo range is 20 to 275.
      // Reset the tick on changing the tempo
//      nexttick = 0;
    }
#endif // POT_TEMPO
  }
  else if (loopstate == 2) {
#ifdef POT_VOL
    // Read the potentiometer to set the volume
    // (if POT_VOL is defined).
    // Need a value between 0 and 254 for the volume
    byte pot2 = analogRead (POT_VOL) >> 2;
    if (pot2 > 254) pot2 = 254;
    if (pot2 != volume) {
      // Set the volume on both channels.
      //
      // Note that the volume setting is actually an attenuation
      // setting, which means the loudest is 0 (no attenuation)
      // and the quietest is 254 or 0xFE (full attenuation).
      //
      // To preserve the "pot at zero, volume at zero" feeling
      // we reverse the values here.
      //
      VSWriteRegister(SCI_VOL, 254-pot2, 254-pot2);
      volume = pot2;
    }
#endif // POT_VOL
  }
  loopstate++;
  if (loopstate > 2) loopstate = 0;
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

void talkMIDI(byte cmd, byte data1, byte data2) {
  //
  // Wait for chip to be ready (Unlikely to be an issue with real time MIDI)
  //
  while (!digitalRead(VS_DREQ)) {
  }
  digitalWrite(VS_XDCS, LOW);

  sendMIDI(cmd);
  
  //Some commands only have one data byte. All cmds less than 0xBn have 2 data bytes 
  //(sort of: http://253.ccarh.org/handout/midiprotocol/)
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
  VSStatus();
#ifdef TEST
#ifdef SINTEST
  // Output a test sine wave to check everything is working ok
  VSSineTest();
  delay(100);
  VSStatus();
#endif // SINTEST
#endif // TEST

  // Enable real-time MIDI mode
  VSLoadUserCode();
  VSStatus();

  // Set the default volume
//  VSWriteRegister(SCI_VOL, 0x20, 0x20);  // 0 = Maximum; 0xFEFE = silence
  VSStatus();
}

// This will read key status and mode registers from the VS10xx device
// and dump them to the serial port.
//
void VSStatus (void) {
#ifdef TEST
  // Print out some of the VS10xx registers
  uint16_t vsreg = VSReadRegister(SCI_MODE); // MODE Mode Register
  sprintf(teststr, "Mode=0x%04x b",vsreg);
  Serial.print(teststr);
  Serial.println(vsreg, BIN);
  vsreg = VSReadRegister(SCI_STATUS);
  sprintf(teststr, "Stat=0x%04x b",vsreg);
  Serial.print(teststr);
  Serial.print(vsreg, BIN);
  switch (vsreg & SS_VER_MASK) {
    case SS_VER_VS1001: Serial.println(" (VS1001)"); break;
    case SS_VER_VS1011: Serial.println(" (VS1011)"); break;
    case SS_VER_VS1002: Serial.println(" (VS1002)"); break;
    case SS_VER_VS1003: Serial.println(" (VS1003)"); break;
    case SS_VER_VS1053: Serial.println(" (VS1053)"); break;
    case SS_VER_VS1033: Serial.println(" (VS1033)"); break;
    case SS_VER_VS1063: Serial.println(" (VS1063)"); break;
    case SS_VER_VS1103: Serial.println(" (VS1103)"); break;
    default: Serial.println(" (Unknown)"); break;
  }
  vsreg = VSReadRegister(SCI_VOL); // VOL Volume
  sprintf(teststr, "Vol =0x%04x\n",vsreg);
  Serial.print(teststr);
  vsreg = VSReadRegister(SCI_AUDATA); // AUDATA Misc Audio data
  sprintf(teststr, "AUDA=0x%04x (%uHz)\n",vsreg,(vsreg&0xFFFE));
  Serial.print(teststr);
  Serial.println();
#endif
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
#ifdef VS1003_MODULE
  VSWriteRegister16(SCI_AIADDR, 0x30);
#endif
#ifdef VS1053_MP3_SHIELD
  VSWriteRegister16(SCI_AIADDR, 0x50);
#endif

#ifdef TEST
  Serial.print("Done\n");
#endif
}
