/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Filter - Revisited
//  https://diyelectromusic.wordpress.com/2021/04/25/arduino-midi-filter-revisited/
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
    Arduino MIDI Library  - https://github.com/FortySevenEffects/arduino_midi_library
*/
#include <MIDI.h>

// Use binary to say which MIDI channels this should respond to.
// Every "1" here enables that channel. Set all bits for all channels.
//
//                               16  12  8   4  1
//                               |   |   |   |  |
uint16_t MIDI_CHANNEL_FILTER = 0b0000000010000000;

// List of instruments to send to any configured MIDI channels.
// Can use any voice numbers here (1 to 128).
//
// Note that General MIDI lists voices as 1-128 but they are
// sent "over the wire" as 0-127.  We use the 1-128 indexing here.
//
// 0 means don't initialise a voice for this channel.
//
byte preset_instruments[16] = {
/* 01 */  0,
/* 02 */  0,
/* 03 */  0,
/* 04 */  0,
/* 05 */  0,
/* 06 */  0,
/* 07 */  0,
/* 08 */  48, // 48 = Timpani
/* 09 */  0,
/* 10 */  0,  // Channel 10 is usually percussion
/* 11 */  0,
/* 12 */  0,
/* 13 */  0,
/* 14 */  0,
/* 15 */  0,
/* 16 */  0
};

// This is required to set up the MIDI library.
// The default MIDI setup uses the built-in serial port.
MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_LED LED_BUILTIN

void setup() {
  pinMode (MIDI_LED, OUTPUT);
  digitalWrite (MIDI_LED, LOW);
  
  // This listens to all MIDI channels
  // They will be filtered out later...
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();  // Disable automatic MIDI Thru function

  // Configure the instruments for all required MIDI channels.
  // Even though MIDI channels are 1 to 16, all the code here
  // is working on 0 to 15 (bitshifts, index into array, MIDI command).
  //
  // Also, instrument numbers in the MIDI spec are 1 to 128,
  // but need to be converted to 0 to 127 here.
  //
  for (byte ch=0; ch<16; ch++) {
    if (preset_instruments[ch] != 0) {
      uint16_t ch_filter = 1<<ch;
      if (MIDI_CHANNEL_FILTER & ch_filter) {
        // API requires a voice 0-127 and channel 1-16
        MIDI.sendProgramChange(preset_instruments[ch]-1, ch+1);
      }
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
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
    uint16_t ch_filter = 1<<(ch-1);  // bit numbers are 0 to 15; channels are 1 to 16
    if (ch == 0) ch_filter = 0xffff; // special case - always pass system messages where ch==0
    if (MIDI_CHANNEL_FILTER & ch_filter) {
      // Now we've filtered on channel, filter on command
      byte cmd = MIDI.getType();
      switch (cmd) {
        case midi::NoteOn:
        case midi::NoteOff:
          digitalWrite (MIDI_LED, HIGH);
          MIDI.send(MIDI.getType(), MIDI.getData1(), MIDI.getData2(), MIDI.getChannel());
          digitalWrite (MIDI_LED, LOW);
          break;
        // Add in any additional messages to pass through here
        // Alternatively, put the "passthrough" in a default statement here
        // and add specific cases for any messages to filter out.
      }
    }
  }
}
