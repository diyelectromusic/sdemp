/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Adafruit Feather MIDI VS1053 Synth and LEDs
//  https://diyelectromusic.wordpress.com/2020/10/18/adafruit-feather-midi-music-and-leds/
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
    VS1053 software patches - http://www.vlsi.fi/en/support/software/vs10xxpatches.html
    Adafruit Music Maker Wing - https://learn.adafruit.com/adafruit-music-maker-featherwing/overview
    Adafruit IS31FL3731 Wing  - https://learn.adafruit.com/adafruit-15x7-7x15-charlieplex-led-matrix-charliewing-featherwing
    Adafruit GFX Library      - https://learn.adafruit.com/adafruit-gfx-graphics-library/overview
    
  Whilst the Adafruit Music Maker Feather Wing supports a jumper to
  allow the VS1053 to boot up into real time MIDI mode, this clashes with
  the use of the Adafruit Feather MIDI Wing which uses TX/RX already.
  
  Consequently this code uses the SPI MIDI mode and a software "patch"
  provided by the VS1053 maufacturers to put the chip into
  real-time MIDI mode.  This is the more complicated technique
  but is pretty much universal on all VS1053 based boards.

  Many thanks to Matthias Neeracher and Nathan Seidle for
  publishing the code that explains how all this works.

*/
#include <SPI.h>
#include <MIDI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_IS31FL3731.h>

// Use binary to say which MIDI channels this should respond to.
// Every "1" here enables that channel. Set all bits for all channels.
// Make sure the bit for channel 10 is set if you want drums.
//
//                               16  12  8   4  1
//                               |   |   |   |  |
uint16_t MIDI_CHANNEL_FILTER = 0b1111111111111111;

// Comment out this #define to disable the Charliewing LED display
#define MIDI_LED_CHANNEL 1

#ifdef MIDI_LED_CHANNEL
// Definitions for the Adafruit Feather CharlieWing 15x7 LED matrix
#define CW_XMAX 15 // pixels go 0 to 14
#define CW_YMAX 7  // pixels go 0 to 6
Adafruit_IS31FL3731_Wing leds = Adafruit_IS31FL3731_Wing();
#endif

// VS1053 Shield pin definitions
#define VS_XCS    6 // Control Chip Select Pin (for accessing SPI Control/Status registers)
#define VS_XDCS   10 // Data Chip Select / BSYNC Pin
#define VS_DREQ   9 // Data Request Pin: Player asks for more data

// There are three selectable sound banks on the VS1053
// These can be selected using the MIDI command 0xBn 0x00 bank
#define DEFAULT_SOUND_BANK 0x00  // General MIDI 1 sound bank
#define DRUM_SOUND_BANK    0x78  // Drums
#define ISNTR_SOUND_BANK   0x79  // General MIDI 2 sound bank

// List of instruments to send to any configured MIDI channels.
byte preset_instruments[16] = {
/* 01 */  0,
/* 02 */  10,
/* 03 */  20,
/* 04 */  30,
/* 05 */  40,
/* 06 */  50,
/* 07 */  60,
/* 08 */  70,
/* 09 */  80,
/* 10 */  0,  // Channel 10 will be ignored later as that is percussion anyway.
/* 11 */  90,
/* 12 */  100,
/* 13 */  105,
/* 14 */  110,
/* 15 */  115,
/* 16 */  120
};

// This is required to set up the MIDI library.
// The default MIDI setup uses the built-in serial port.
MIDI_CREATE_DEFAULT_INSTANCE();

byte instrument;

void setup() {
  // put your setup code here, to run once:
  pinMode (LED_BUILTIN, OUTPUT);
#ifdef MIDI_LED_CHANNEL
  leds.begin();
#endif
  
  initialiseVS1053();

  // This listens to all MIDI channels
  // They will be filtered out later...
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Configure the instruments for all required MIDI channels.
  // Even though MIDI channels are 1 to 16, all the code here
  // is working on 0 to 15 (bitshifts, index into array, MIDI command).
  for (byte ch=0; ch<16; ch++) {
    if (ch != 9) { // Ignore channel 10 (drums)
      uint16_t ch_filter = 1<<ch;
      if (MIDI_CHANNEL_FILTER & ch_filter) {
        talkMIDI(0xC0|ch, preset_instruments[ch], 0);

        // Indicate the active channels on the LEDs
#ifdef MIDI_LED_CHANNEL
        leds.drawPixel (ch, CW_YMAX/2, 255);
#endif
      }
    }
  }
  delay(2000);
  leds.clear();
}

void loop() {
  // If we have MIDI data then forward it on.
  // Based on the DualMerger.ino example from the MIDI library.
  //
  if (MIDI.read()) {
    digitalWrite(LED_BUILTIN, HIGH);
    // Extract the channel and type to build the command to pass on
    // Recall MIDI channels are in the range 1 to 16, but will be
    // encoded as 0 to 15.
    //
    // All commands in the range 0x80 to 0xE0 support a channel.
    // Any command 0xF0 or above do not (they are system messages).
    // 
    byte ch = MIDI.getChannel();
    uint16_t ch_filter = 1<<(ch-1);  // bit numbers are 0 to 15; channels are 1 to 16
    if (ch == 0) ch_filter = 0xffff; // special case - always pass system messages where ch==0
    if (MIDI_CHANNEL_FILTER & ch_filter) {
      byte cmd = MIDI.getType();
      if ((cmd >= 0x80) && (cmd <= 0xE0)) {
        // Merge in the channel number
        cmd |= (ch-1);
      }
      byte data1 = MIDI.getData1();
      byte data2 = MIDI.getData2();
      talkMIDI(cmd, data1, data2);

#ifdef MIDI_LED_CHANNEL
      // Active the LEDs if appropriate - i.e. Note On for the right channel
      if (cmd == (0x90|(MIDI_LED_CHANNEL-1))) {
        // data1 will be the note
        doLed (data1, true);
      }
      if (cmd == (0x80|(MIDI_LED_CHANNEL-1))) {
        // data1 will be the note
        doLed (data1, false);
      }
#endif
    }
    digitalWrite(LED_BUILTIN, LOW);
  }
}

#ifdef MIDI_LED_CHANNEL
void doLed (byte note, bool ledOn) {
    // Will use each row the display as an "octave" of 12 semitones
    //  X coordinate will be remainder after dividing by 12
    //  Y coordinate will be the solution after integer dividing by 12
    //
    // Note: Start from note C1 which is MIDI note 24.
    //       So C1 (24) is set to 0,0
    note = note - 24;
    int x = note % 12;
    int y = note / 12;
    if (ledOn) {
      leds.drawPixel (x, y, 128);
    } else {
      leds.drawPixel (x, y, 0);
    }
}
#endif

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
#ifdef VS_RESET
  pinMode(VS_RESET, OUTPUT);
#endif
  //Setup SPI for VS1053
//  pinMode(10, OUTPUT); // Pin 10 must be set as an output for the SPI communication to work
//  Adafruit VS1053 Wing - there is no "SS" pin (pin 10 on Arduino) for SPI comms...
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);

  //From page 12 of datasheet, max SCI reads are CLKI/7. Input clock is 12.288MHz. 
  //Internal clock multiplier is 1.0x after power up. 
  //Therefore, max SPI speed is 1.75MHz. We will use 1MHz to be safe.
  SPI.setClockDivider(SPI_CLOCK_DIV16); //Set SPI bus speed to 1MHz (16MHz / 16 = 1MHz)
  SPI.transfer(0xFF); //Throw a dummy byte at the bus

  delayMicroseconds(1);
#ifdef VS_RESET
  digitalWrite(VS_RESET, HIGH); //Bring up VS1053
#endif
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
