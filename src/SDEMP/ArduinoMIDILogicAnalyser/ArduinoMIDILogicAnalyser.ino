/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MIDI Analyser
//  https://diyelectromusic.wordpress.com/2021/06/08/arduino-midi-analyser/
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
  Using principles from the following tutorials:
    Sparkfun MIDI Tutorial - https://learn.sparkfun.com/tutorials/midi-tutorial/all
    Arduino PORT IO        - https://www.arduino.cc/en/Reference/PortManipulation
    TimerOne               - https://www.arduino.cc/reference/en/libraries/timerone/
*/
#include <TimerOne.h>

//#define GRAPHMODE 1    // Uncomment to enable Serial Plotter mode
#define FILTER_TICKS 1 // Uncomment to skip possible spurious "ticks" on the MIDI signal

// MIDI Baud is 31250 which is 1 bit every 1/32150 seconds
//    = 1 bit every 32 uS
//
// So ideally need to sample at twice this to get the data
// assuming an accurate MIDI baud setting.
//
// Note: The baud rate estimation will not go down very far.
//
//       It could sample down to 19200 if the buffer size
//       is increased and the "laststop" test increased to match.
//       It should spot faster baud rates though.
//
//       If you are having baud rate issues, it might be better to
//       run in "GRAPHMODE" initially to get a feel for what is going on.
//
// I actually sample at 4x here, so 8uS, in order to help
// debug baud rate problems via the "ascii" graph at the top.
//
#define SAMPLE  4
#define MIDICLK 8

int lastval;
unsigned reccnt;
bool recording;

#define BUFSIZE 200
int buf[BUFSIZE];
char notes[12]  = {' ','C',' ','D',' ',' ','F',' ','G',' ','A',' '};
char sharps[12] = {'C','#','D','#','E','F','#','G','#','A','#','B'};

#define PREAMBLE ">>>>>  "
void print2hex (uint8_t num) {
  if (num < 16) Serial.print("0");
  Serial.print(num,HEX);
}
void printMidiMsg (uint8_t b1, uint8_t b2, uint8_t b3) {
  // First MIDI byte is:
  //   bCCCCNNNN = CCCC = command (0 to 15); NNNN = channel (0 to 15).
  // NB: Convert channel to be 1 to 16.
  uint8_t cmd = b1>>4;
  uint8_t ch = 1 + b1 & 0xf;

  Serial.print(PREAMBLE);
  Serial.print("0x");
  print2hex(b1);
  print2hex(b2);
  print2hex(b3);
  Serial.print("\t\t");
  Serial.print(ch);
  Serial.print("\t0x");
  Serial.print(cmd,HEX);
  Serial.print("\t");
  switch (cmd) {
    case 0x8:
      Serial.print(b2);
      Serial.print("\t");
      Serial.print(b3);
      Serial.print("\t");
      Serial.print("NoteOff\t");
      Serial.print(notes[b2%12]);
      Serial.print(sharps[b2%12]);
      Serial.print((b2/12)-1,DEC);
      break;
    case 0x9:
      Serial.print(b2);
      Serial.print("\t");
      Serial.print(b3);
      Serial.print("\t");
      Serial.print("NoteOn\t");
      Serial.print(notes[b2%12]);
      Serial.print(sharps[b2%12]);
      Serial.print((b2/12)-1,DEC);
      break;
    case 0xA:
      Serial.print(b2);
      Serial.print("\t");
      Serial.print(b3);
      Serial.print("\t");
      Serial.print("Pressure\t");
      break;
    case 0xB:
      Serial.print(b2);
      Serial.print("\t");
      Serial.print(b3);
      Serial.print("\t");
      Serial.print("Control Change\t");
      break;
    case 0xC:
      Serial.print(b2);
      Serial.print("\t");
      Serial.print("Program Change\t");
      break;
    case 0xD:
      Serial.print(b2);
      Serial.print("\t");
      Serial.print("Ch Pressure\t");
      break;
    case 0xE:
      Serial.print(b2);
      Serial.print("\t");
      Serial.print(b3);
      Serial.print("\t");
      Serial.print("Pitch Bend\t");
      break;
    case 0xF:
      Serial.print("SysEx\t(Not Decoded)");
      break;
  }
  Serial.print("\n");
}

uint32_t estimateBaud () {
  // Look for a consecutive sequence of "1" amd assume this
  // is the end of the MIDI bytes.  Calculated an estimated
  // baud rate by working out the time for this number of samples
  // and dividing by the number of bits in a three-byte MIDI message:
  //    1 start bit + 8 data bits + 1 stop bit
  //
  // This is pretty crude: it doesn't measure two-byte messages and
  // can't cope with multi-byte sequences, including "running status".
  //
  // It is also just an estimation due to errors in the sample
  // sizes, possible rounding in calculations, and I'm not convinced
  // by the calculation wrt number bits in the sample size - but it
  // is good enough for an approximation.
  //
  int laststop=0;
  int overlap = SAMPLE*12; // at least "12 bits" worth of 1s
  for (int i=0; i<BUFSIZE-overlap; i++) {
    int ones=0;
    for (int j=0; j<overlap; j++) {
      if (buf[i+j] != 0) {
        ones++;
      }
    }

    if (ones == overlap) {
      // This is the end - or at least the start of the last STOP bit
      laststop = i;
      break;
    }
  }

  if (laststop == 0 || laststop > 30*SAMPLE) {
    // no valid result - at least not one that really means anything to us.
    return 0;
  }

  // 10 bits per byte, 3 bytes, less last STOP bit, at SAMPLE rate.
  // NB: Calculated 1000x to keep some precision.
  uint32_t uSbit = laststop * MIDICLK * (1000UL / 28UL);
  uint32_t baud = 1000000000UL/uSbit;
  return baud;
}

int decodeMidiMsg (uint32_t msg) {
  // First check for stop and start bits in the right places.
  //
  // Format:
  //   Start bit (0)
  //   Byte 1
  //   Stop bit (1)
  //   Start bit (0)
  //   Byte 2
  //   Stop bit (1)
  //   Start bit (0)
  //   Byte 3
  //   Stop bit (1)
  //   The rest will be (1) if this is the end of the message.
  //
  // Each byte is stored "least significant bit first".
  // BUT as these were stored from bit 0 up to bit 32, they
  // are actually in the right order in msg when it comes to reading them...
  // i.e. These have been stored "in order" from right to left
  //
  // Note: this doesn't decode multi-byte messages, e.g. SysEx
  //       or "running status" messages.
  //
  // This is the pattern we're looking for wrt start and stop bits.
  //                         ..1........01........01........0
  uint32_t startstop     = 0b00100000000010000000001000000000;
  uint32_t startstopmask = 0b00100000000110000000011000000001;
  if ((msg & startstopmask) != startstop) {
    // Invalid message - we haven't seen start/stop bits in the right places
    return 0;
  }

  // Now pull out the data
  msg = msg >>1; // Skip start bit
  uint8_t b1 = msg & 0xff;
  msg = msg >> 10; // skip byte, stop bit, next start bit
  uint8_t b2 = msg & 0xff;
  msg = msg >> 10; // skip byte, stop bit, next start bit
  uint8_t b3 = msg & 0xff;

  // Now print out the message.
  printMidiMsg (b1, b2, b3);

  return 1;
}

void setup() {
  Serial.begin(9600);
  pinMode(2, INPUT);

  recording = false;
  reccnt = 0;

  Timer1.initialize(MIDICLK);
  Timer1.attachInterrupt(readSample);
}

// The Interrupt Routine.
// Execution time needs to be kept as short as possible.
void readSample () {
  if (recording) {
    // Read D2 directly using PORT IO
    buf[reccnt] = PIND & 0x04;
    reccnt++;
    if (reccnt >= BUFSIZE) {
      recording = false;
    }
  }
}

void loop() {
  // Whilst in recording mode, the loop does nothing.
  if (recording) {
    return;
  }

  // Even though the pin is read as part of the data acquisition
  // process, it is read here to spot the HIGH->LOW transition
  // that kicks off the recording.
  //
  int reading = digitalRead(2);

  // If the recording has just completed, then reccnt
  // will equal the size of the buffer (assuming further
  // recording hasn't started yet - which it won't have,
  // as the detection happens later in this same function).
  //
  if (reccnt == BUFSIZE) {
#ifdef GRAPHMODE
    // This outputs data suitable for displaying in the Arduino "Serial Plotter".
    //
    for (int i=0; i<10; i++) {
      // Print a short pre-amble
      Serial.println("50");
    }
    for (int i=0; i<BUFSIZE; i++) {
      if (buf[i]) {
        Serial.println("50");
      } else {
        Serial.println("0");
      }
    }
#else
    // This outputs am ASCII version of the plot, MIDI data and various statistics.
    // It is meant to be used with the Arduino "Serial Monitor".
    //
    // But a quick sanity check first to see if there might be sensible data
    // My PC MIDI interface seems to output a regular two zeros followed by "all ones"
    // so I'm ignoring this as a special case...
    //
#ifdef FILTER_TICKS
    int ones=0;
    for (int i=0; i<BUFSIZE; i++) {
      if (buf[i]) ones++;
    }
    if (ones >= BUFSIZE-(3*SAMPLE)) {
      // It was largely a spurious trigger so
      // clear this buffer and drop out!
      reccnt = 0;
      return;
    }
#endif
    // Output the "graph" or "trace".
    //
    for (int i=0; i<BUFSIZE; i++) {
      if (buf[i]) {
        Serial.print("-");
      } else {
        Serial.print(" ");
      }
    }
    Serial.print("\n");
    for (int i=0; i<BUFSIZE; i++) {
      if (buf[i]) {
        Serial.print(" ");
      } else {
        Serial.print("_");
      }
    }
    Serial.print("\n");
    Serial.print("\n");

    // Output the associated binary stream.
    //
    uint32_t msg = 0;
    int msgbit=0;
    for (int i=0; i<BUFSIZE; i+=SAMPLE) {
      if (buf[i]) {
        // MIDI messages are sent least significant bit first
        // and include a start bit (0) and stop bit (1).
        //
        // We are storing up to three bytes, plus start/stop bits
        // here for later printing out.
        //
        Serial.print("1");
        // We grab the first 32 bits as the message for decoding later
        if (msgbit<32) {
          msg |= (1UL<<msgbit);
        }
      } else {
        Serial.print("0");
      }
      msgbit++;
      for (int j=0; j<SAMPLE-1; j++) {
        // Line up the raw sample with our interpretation of it
        Serial.print(" ");
      }
    }
    Serial.print("\n");

    // Output the anticipated "good pattern" of START and STOP bits.
    // Useful for reference/debugging purposes.
    //
    msgbit=0;
    for (int i=0; i<BUFSIZE; i+=SAMPLE) {
      switch (msgbit) {
        case 0:
        case 10:
        case 20:
          // Start bit
          Serial.print("0");
          break;
        case 9:
        case 19:
        case 29:
          // Stop bit
          Serial.print("1");
          break;
        default:
          Serial.print(" ");
      }
      msgbit++;
      for (int j=0; j<SAMPLE-1; j++) {
        Serial.print(" ");
      }
    }
    Serial.print("\n");

    // Output the decoded MIDI data (if possible).
    //
    if (decodeMidiMsg (msg)) {
      // Output the estimated MIDI baud rate, with errors (if possible).
      //
      uint32_t baud = estimateBaud();
      Serial.print(PREAMBLE);
      Serial.print("Estimated baud rate: ");
      if (baud == 0) {
        Serial.print("(not available)");
      } else {
        Serial.print(baud);
        Serial.print(" (31250 ");
        if (baud > 31250) {
          Serial.print("+ ");
          uint32_t error = 1000UL*(baud-31250UL)/31250UL; // 10 times too big
          Serial.print(error/10);
          Serial.print(".");
          Serial.print(error%10);
        } else {
          Serial.print("- ");
          uint32_t error = 1000UL*(31250UL-baud)/31250UL; // 10 times too big
          Serial.print(error/10);
          Serial.print(".");
          Serial.print(error%10);
        }
        Serial.print("%)");
      }
      Serial.print("\n");
    }
#endif
    reccnt = 0;
  }

  // Start recording on a HIGH to LOW transition
  if (lastval == HIGH && reading == LOW) {
    recording = true;
  }
  lastval = reading;
}
