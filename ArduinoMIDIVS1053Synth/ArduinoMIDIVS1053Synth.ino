/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI VS1053 Synth
//  https://diyelectromusic.wordpress.com/2020/07/08/arduino-midi-vs1053-synth/
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
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Real-time MIDI Mode for the VS1053 - https://gist.github.com/microtherion/2636608
    Sparkfun Musical Instrument Shield Tutorial - https://www.sparkfun.com/tutorials/302
    VS1053 software patches - http://www.vlsi.fi/en/support/software/vs10xxpatches.html

  This uses a generic VS1053 MP3 Player board that does not
  support booting up into real time MIDI mode and does not
  support the RX/TX serial port MIDI connections.

  If you have a board that supports those, such as the
  SparkFun Musical Instrument Shield linked above, or
  the Adafruit VD1053 breakout board, then things are a
  lot simpler and you can use the basic libraries provided
  with those boards instead.

  This code uses the SPI MIDI mode and a software "patch"
  provided by the VS1053 maufacturers to put the chip into
  real-time MIDI mode.  This is the more complicated technique
  but is pretty much universal on all VS1053 based boards.

  Many thanks to Matthias Neeracher and Nathan Seidle for
  publishing the code that explains how all this works.

*/
#include <SPI.h>
#include <MIDI.h>

// VS1053 Shield pin definitions
#define VS_XCS    6 // Control Chip Select Pin (for accessing SPI Control/Status registers)
#define VS_XDCS   7 // Data Chip Select / BSYNC Pin
#define VS_DREQ   2 // Data Request Pin: Player asks for more data
#define VS_RESET  8 // Reset is active low

// There are three selectable sound banks on the VS1053
// These can be selected using the MIDI command 0xBn 0x00 bank
#define DEFAULT_SOUND_BANK 0x00  // General MIDI 1 sound bank
#define DRUM_SOUND_BANK    0x78  // Drums
#define ISNTR_SOUND_BANK   0x79  // General MIDI 2 sound bank

// This is required to set up the MIDI library.
// The default MIDI setup uses the built-in serial port.
MIDI_CREATE_DEFAULT_INSTANCE();

byte instrument;

void setup() {
  // put your setup code here, to run once:
  initialiseVS1053();

  // This listens to all MIDI channels
  MIDI.begin(MIDI_CHANNEL_OMNI);

  delay(1000);

  // A test "output" to see if the VS053 is working ok
  for (byte i=60; i<73; i++) {
    talkMIDI (0x90, i, 127);
    delay (100);
    talkMIDI (0x80, i, 0);
    delay (200);
  }
  for (byte i=40; i<50; i++) {
    talkMIDI (0x99, i, 127);
    delay (100);
    talkMIDI (0x89, i, 0);
    delay (200);    
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  // Read the potentiometer for a value between 0 and 127
  // to use to select the instrument in the general MIDI bank
  // for channel 1.
  byte pot = analogRead (A0) >> 3;
  if (pot != instrument) {
    // Change the instrument to play
    instrument = pot;
    talkMIDI(0xC0, instrument, 0);
  }

  // If we have MIDI data then forward it on.
  // Based on the DualMerger.ino example from the MIDI library.
  //
  if (MIDI.read()) {
    // Extract the channel and type to build the command to pass on
    // Recall MIDI channels are in the range 1 to 16, but will be
    // encoded as 0 to 15.
    //
    // All commands in the range 0x80 to 0xE0 support a channel.
    // Any command 0xF0 or above do not (they are system messages).
    // 
    byte ch = MIDI.getChannel();
    byte cmd = MIDI.getType();
    if ((cmd >= 0x80) && (cmd <= 0xE0)) {
      // Merge in the channel number
      cmd |= (ch-1);
    }
    talkMIDI(cmd, MIDI.getData1(), MIDI.getData2());
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
 * Code from here on is the magic required to initialise the VS1053 and 
 * put it into real-time MIDI mode using an SPI-delivered patch.
 * 
 * Here be dragons...
 *
 * Taken from MP3_Shield_RealtimeMIDI.ino by Matthias Neeracher
 * which was based on Nathan Seidle's Sparkfun Electronics example code for the Sparkfun 
 * MP3 Player and Music Instrument shields and and VS1053 breakout board.
 *
 ***********************************************************************************************
 */

void initialiseVS1053 () {
  // Set up the pins controller the SPI link to the VS1053
  pinMode(VS_DREQ, INPUT);
  pinMode(VS_XCS, OUTPUT);
  pinMode(VS_XDCS, OUTPUT);
  digitalWrite(VS_XCS, HIGH);  //Deselect Control
  digitalWrite(VS_XDCS, HIGH); //Deselect Data
  pinMode(VS_RESET, OUTPUT);

  //Setup SPI for VS1053
  pinMode(10, OUTPUT); // Pin 10 must be set as an output for the SPI communication to work
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

  // Enable real-time MIDI mode
  VSLoadUserCode();
}

//Write to VS10xx register
//SCI: Data transfers are always 16bit. When a new SCI operation comes in 
//DREQ goes low. We then have to wait for DREQ to go high again.
//XCS should be low for the full duration of operation.
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

//
// Plugin to put VS10XX into realtime MIDI mode
// Originally from http://www.vlsi.fi/fileadmin/software/VS10XX/vs1053b-rtmidistart.zip
// Permission to reproduce here granted by VLSI solution.
//
const unsigned short sVS1053b_Realtime_MIDI_Plugin[28] = { /* Compressed plugin */
  0x0007, 0x0001, 0x8050, 0x0006, 0x0014, 0x0030, 0x0715, 0xb080, /*    0 */
  0x3400, 0x0007, 0x9255, 0x3d00, 0x0024, 0x0030, 0x0295, 0x6890, /*    8 */
  0x3400, 0x0030, 0x0495, 0x3d00, 0x0024, 0x2908, 0x4d40, 0x0030, /*   10 */
  0x0200, 0x000a, 0x0001, 0x0050,
};

void VSLoadUserCode(void) {
  int i = 0;

  while (i<sizeof(sVS1053b_Realtime_MIDI_Plugin)/sizeof(sVS1053b_Realtime_MIDI_Plugin[0])) {
    unsigned short addr, n, val;
    addr = sVS1053b_Realtime_MIDI_Plugin[i++];
    n = sVS1053b_Realtime_MIDI_Plugin[i++];
    while (n--) {
      val = sVS1053b_Realtime_MIDI_Plugin[i++];
      VSWriteRegister(addr, val >> 8, val & 0xFF);
    }
  }
}
