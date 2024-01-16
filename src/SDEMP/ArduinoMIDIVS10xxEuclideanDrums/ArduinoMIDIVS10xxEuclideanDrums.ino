/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI VS10xx Synth
//  https://diyelectromusic.wordpress.com/2021/01/09/arduino-midi-vs1003-synth/
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
    Arduino MIDI Library    - https://github.com/FortySevenEffects/arduino_midi_library
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
#include "vs10xx_uc.h" // From VLSI website: http://www.vlsi.fi/en/support/software/microcontrollersoftware.html

#include "euclid.h"

// the pattern generator
euclid euc;

#define TEST 1

// Uncomment to perform a test play of the instruments and drums on power up
//#define SOUND_CHECK 1

// Uncomment this if you have a test LED
// Note: The LED_BUILTIN for the Uno is on pin 13,
//       which is used below for the VS1003 link.
//#define MIDI_LED 4

// This code supports several variants of the VS10xx based shields.
// Choose the apprppriate one here (and comment out the others).
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

// Use binary to say which MIDI channels this should respond to.
// Every "1" here enables that channel. Set all bits for all channels.
// Make sure the bit for channel 10 is set if you want drums.
//
//                               16  12  8   4  1
//                               |   |   |   |  |
uint16_t MIDI_CHANNEL_FILTER = 0b1111111111111111;

// Comment out any of these if not in use
//#define POT_MIDI  A0 // MIDI control
#define POT_VOL   A1 // Volume control

// Channel to link to the potentiometer (1 to 16)
#define POT_MIDI_CHANNEL 1

#ifdef VS1053_MP3_SHIELD
// VS1053 Shield pin definitions
#define VS_XCS    6 // Control Chip Select Pin (for accessing SPI Control/Status registers)
#define VS_XDCS   7 // Data Chip Select / BSYNC Pin
#define VS_DREQ   2 // Data Request Pin: Player asks for more data
#define VS_RESET  8 // Reset is active low
#endif
#ifdef VS1003_MODULE
// VS1003 Module pin definitions
#define VS_XCS    10 //8 // Control Chip Select Pin (for accessing SPI Control/Status registers)
#define VS_XDCS   6 // 9 // Data Chip Select / BSYNC Pin
#define VS_DREQ   7 // Data Request Pin: Player asks for more data
#define VS_RESET  8 // 10 // Reset is active low
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

// There are three selectable sound banks on the VS1053
// These can be selected using the MIDI command 0xBn 0x00 bank
#define DEFAULT_SOUND_BANK 0x00  // General MIDI 1 sound bank
#define DRUM_SOUND_BANK    0x78  // Drums
#define ISNTR_SOUND_BANK   0x79  // General MIDI 2 sound bank

// List of instruments to send to any configured MIDI channels.
// Can use any GM MIDI voice numbers here (1 to 128), or more specific definitions
// (for example as found in vs1003inst.h for the VS1003).
//
// 0 means "ignore"
//
byte preset_instruments[16] = {
/* 01 */  1,
/* 02 */  9,
/* 03 */  17,
/* 04 */  25,
/* 05 */  30,
/* 06 */  33,
/* 07 */  41,
/* 08 */  49,
/* 09 */  57,
/* 10 */  0,  // Channel 10 will be ignored later as that is percussion anyway.
/* 11 */  65,
/* 12 */  73,
/* 13 */  81,
/* 14 */  89,
/* 15 */  113,
/* 16 */  48
};

// This is required to set up the MIDI library.
// The default MIDI setup uses the built-in serial port.
MIDI_CREATE_DEFAULT_INSTANCE();

byte instrument;
byte volume;

#ifdef TEST
char teststr[32];
#endif

void setup() {

#ifndef TEST
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
    delay(100);
    digitalWrite(MIDI_LED, LOW);
    delay(100);
  }
#endif // MIDI_LED

  // put your setup code here, to run once:
  initialiseVS10xx();

  // This listens to all MIDI channels
  // They will be filtered out later...
#ifndef TEST
  MIDI.begin(MIDI_CHANNEL_OMNI);
#endif // TEST

  delay(500);

#ifdef SOUND_CHECK
  // A test "output" to see if the VS0xx is working ok
#ifdef POT_MIDI
  byte ch = POT_MIDI_CHANNEL-1;
#else  // POT_MIDI
  byte ch = 0;
#endif // POT_MIDI
  for (byte i=60; i<73; i++) {
    talkMIDI (0x90|ch, i, 127);
    delay (100);
    talkMIDI (0x80|ch, i, 0);
    delay (200);
  }
  for (byte i=40; i<50; i++) {
    talkMIDI (0x99, i, 127);
    delay (100);
    talkMIDI (0x89, i, 0);
    delay (200);    
  }
#endif // SOUND_CHECK

  // Configure the instruments for all required MIDI channels.
  // Even though MIDI channels are 1 to 16, all the code here
  // is working on 0 to 15 (bitshifts, index into array, MIDI command).
  //
  // Also, instrument numbers in the MIDI spec are 1 to 128,
  // but need to be converted to 0 to 127 here.
  //
  for (byte ch=0; ch<16; ch++) {
    if (ch != 9) {  // Ignore channel 10 (drums)
      if (preset_instruments[ch] != 0) {
        uint16_t ch_filter = 1<<ch;
        if (MIDI_CHANNEL_FILTER & ch_filter) {
          talkMIDI(0xC0|ch, preset_instruments[ch]-1, 0);
        }
      }
    }
  }
  // For some reason, the last program change isn't registered
  // without an extra "dummy" read here.
  talkMIDI(0x80, 0, 0);

  // Set these invalid to trigger a read of the pots
  // (if being used) first time through.
  instrument = -1;
  volume = -1;


// Euclidean sequences
euc.generateSequence(16,31);


  
}
/* from the euclidiean attiny
 *  
uint8_t analogPins[3]={A1,A2,A3};
uint8_t _xor;
int _val;
uint8_t counter;
bool clkState;
bool rstState;
uint8_t address=0;
uint8_t steps,fills, lastSteps, lastFills;
bool butState;
uint8_t clkCounter;
bool usePin[4]={  true,false,true,true };
 */
uint8_t steps,fills, lastSteps, lastFills;
bool butState;
uint8_t clkCounter =0;
 
void loop() {
  // put your main code here, to run repeatedly:
  if( clkCounter == 31) {
    instrument = random(15) + 8;
    euc.generateSequence(16,31);
    clkCounter = 0;
  }

#ifndef TEST
  Serial.println ("Step");
  Serial.println ( euc.getCurrentStep() );
  Serial.println ( euc.getStepNumber() );
#endif
  euc.doStep();
  clkCounter++;
  

  // Read the potentiometer for a value between 0 and 127
  // to use to select the instrument in the general MIDI bank
  // for POT_MIDI_CHANNEL (if POT_MIDI is defined).
#ifdef POT_MIDI
  byte pot1 = analogRead (POT_MIDI) >> 3;
  if (pot1 != instrument) {
    // Change the instrument to play
#ifdef VS1003_MODULE
    // The VS1003 has a limited set of instruments to choose from
    // So convert 0 to 127 into 0 to VS1003VOICES-1
    int voice = ((int)pot1)/(128/VS1003VOICES);
    if (voice >= VS1003VOICES) voice = VS1003VOICES-1;
    instrument = vs1003voices[voice];
#else
    instrument = pot1;
#endif // VS1003_MODULE
    
    talkMIDI(0xC0 | (POT_MIDI_CHANNEL-1), instrument, 0);
  }
#endif // POT_MIDI

    if(euc.getCurrentStep() ) {
      int note = random(45)+30;
      talkMIDI (0x99, note, 127 - (random(50)));
      delay (100);
      talkMIDI (0x89, note, 0);
      delay (50); 
    }
 

  // Read the potentiometer to set the volume
  // (if POT_VOL is defined).
#ifdef POT_VOL
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

  // If we have MIDI data then forward it on.
  // Based on the DualMerger.ino example from the MIDI library.
  //
  if (MIDI.read()) {
#ifdef MIDI_LED
    digitalWrite(MIDI_LED, HIGH);
#endif // MIDI_LED
    // Extract the channel and type to build the command to pass on
    // Recall MIDI channels are in the range 1 to 16, but will be
    // encoded as 0 to 15.
    //
    // All commands in the range 0x80 to 0xE0 support a channel.
    // Any command 0xF0 or above do not (they are system messages).
    // 
    byte ch = MIDI.getChannel();
    uint16_t ch_filter = 1<<(ch-1);  // bit numbers are 0 to 15; channels are 1 to 16
    if (ch == 0) ch_filter = 0; // special case - always ignore system messages where ch==0
    if (MIDI_CHANNEL_FILTER & ch_filter) {
      byte cmd = MIDI.getType();
      if ((cmd >= 0x80) && (cmd <= 0xE0)) {
        // Merge in the channel number
        cmd |= (ch-1);
      }
      talkMIDI(cmd, MIDI.getData1(), MIDI.getData2());
    }
#ifdef MIDI_LED
    digitalWrite(MIDI_LED, LOW);
#endif // MIDI_LED
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

#ifdef TEST
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// A set of test functions that can be used to probe the voices
// used on less capable modules where voices are mapped over to GM voice sets.
//
// Can be ignored for most practical purposes!
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void checkVoices () {
  // Voices to check - range 1 to 128
  for (int i=120; i<129; i++) {
    Serial.print("Voice: ");
    Serial.println(i);
    testVoice(i);
  }  
}

void probeVoice (int voice) {
  int ins[]  = {  1,  9, 17, 25, 30, 33, 41, 49, 57, 65, 73, 81, 89,113}; // 1 to 128; change to 0 to 127 in msg
  char vce[] = {'P','V','O','G','g','B','v','S','T','x','F','L','d','s'};
  Serial.print("Probing voice: ");
  Serial.println(voice);
  for (int i=0; i<(sizeof(ins)/sizeof(ins[0])); i++) {
    Serial.print("Voice: ");
    Serial.print(ins[i]);
    Serial.print(" ");
    Serial.println(vce[i]);
    testVoice(ins[i]);
    testVoice(voice);
  }
  Serial.print("\n");
}

// voice is 1 to 128
void testVoice (int voice) {
  talkMIDI(0xC0, voice-1, 0);
  talkMIDI (0x90, 60, 127);
  delay(200);
  talkMIDI (0x90, 64, 127);
  delay(200);
  talkMIDI (0x90, 67, 127);
  delay(200);
  talkMIDI (0x80, 60, 0);
  talkMIDI (0x80, 64, 0);
  talkMIDI (0x80, 67, 0);
  delay(200);
}
#endif
