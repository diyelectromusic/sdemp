/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino PSS-680 Editor - Part 1
//  https://diyelectromusic.wordpress.com/2023/08/02/arduino-pss-680-synth-editor-part-1/
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
    PSS-680 User Manual MIDI Implementation Chart
*/
#include <MIDI.h>

#define SWTEST
#define SYSEXDUMP

#ifdef SWTEST
#include <SoftwareSerial.h>

// From https://www.arduino.cc/en/Reference/softwareSerial
#define SS_RX  2
#define SS_TX  3
using Transport = MIDI_NAMESPACE::SerialMIDI<SoftwareSerial>;
SoftwareSerial sSerial = SoftwareSerial(SS_RX, SS_TX);
Transport serialMIDI(sSerial);
MIDI_NAMESPACE::MidiInterface<Transport> MIDI((Transport&)serialMIDI);

#define DP(...) Serial.print(__VA_ARGS__)
#define DPLN(...) Serial.println(__VA_ARGS__)
#else
#define DP(...)
#define DPLN(...)
MIDI_CREATE_DEFAULT_INSTANCE();
#endif

// The following define the MIDI parameters to use.
//
#define MIDI_CHANNEL 1  // MIDI Channel number (1 to 16, or MIDI_CHANNEL_OMNI)
#define PSS680BANK   0  // Which of the 5 PSS680 banks to use: 0 to 4

// Comment out to remove the LED indicator or change if you want your own LED
#define MIDI_LED     LED_BUILTIN
#define LED_ON_TIME 300
unsigned long ledmillis;
void ledInit() {
#ifdef MIDI_LED
  pinMode(MIDI_LED, OUTPUT);
  digitalWrite(MIDI_LED, LOW);
#endif
}
void ledOn() {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, HIGH);
  ledmillis = LED_ON_TIME+millis();
#endif
}
void ledOff() {
#ifdef MIDI_LED
  digitalWrite(MIDI_LED, LOW);
  ledmillis=0;
#endif
}
void ledScan() {
#ifdef MIDI_LED
  if (ledmillis) {
    if (ledmillis < millis()) {
      ledOff();
    }
  }
#endif
}

bool bankLoaded;
bool lastBankLoaded;
unsigned long delaytime;

void setup() {
  ledInit();

#ifdef SWTEST
  // Using 115200 as that is less overhead debugging
  // and more time for MIDI handling.
  Serial.begin(115200);
#endif
  MIDI.begin(MIDI_CHANNEL);
  MIDI.turnThruOff();
  MIDI.setHandleSystemExclusive(midiSysEx);

  bankLoaded = false;
  lastBankLoaded = false;
}

bool trigger;
void loop() {
  MIDI.read();
  ledScan();

  if (!lastBankLoaded && bankLoaded) {
    DP("Bank ");
    DP(getSysExBank());
    DPLN(" loaded!");
    delaytime = millis() + 5000; // 5 seconds
    trigger = true;
    bankLoaded = false;
  }

  if ((delaytime < millis()) && (trigger == true)) {
    // Trigger delayed action.
    // Need to wait for the SysEx dump to complete!
    int oldST = getSinTbl(2);
    DP("Sin Table: ");
    DP(oldST);
    oldST++;
    if (oldST > 3) oldST = 0;
    DP(" -> ");
    DPLN(oldST);
    setSinTbl(2, oldST);
    sendSysExBank();
    trigger = false;
  }

  lastBankLoaded = bankLoaded;
}

// Note: From a performance point of view, if this function
//       is spending any significant time processing a SysEx
//       message then it won't be able to keep up with a full
//       dump of SysEx messages over MIDI!
//
//       This will be particularly true for a SoftwareSerial!
//
//       Chances are, in any sequence of SysEx messages, this
//       will only be able to parse the first.
//
//       Also note that the MIDI library will "chunk" up a
//       SysEx message if it is larger than the MIDI library's
//       max SysEx message size (by default this is 128).
//
//       A chunked message will look like the following calls
//       to the callback:
//        Call 1: SysExMsg = F0........F0
//        Call 2: SysExMsg = F7........F0
//        Call 3: SysExMsg = F7........F0
//        Call 4: SysExMsg = F7.....F7
//
void midiSysEx (byte *inArray, unsigned inSize) {
  ledOn();
  recvSysExBank (inArray, inSize);
}

void midiSysExDump (byte *inArray, unsigned inSize) {
#ifdef SYSEXDUMP
  int idx=0;
  for (int i=0; i<inSize; i++) {
    if (inArray[i] < 16) {
      DP("0");
    }
    DP(inArray[i], HEX);
    idx++;
    if ((idx & 0x3) == 0) {
      DP(" ");
    }
    if (idx >= 32) {
      DP("\n");
      idx = 0;
    }
  }
  if (idx != 0) {
    DP("\n");
  }
#else
  // Output first 4 bytes only
  if (inArray[0] < 16) DP("0");
  DP(inArray[0], HEX);
  if (inArray[1] < 16) DP("0");
  DP(inArray[1], HEX);
  if (inArray[2] < 16) DP("0");
  DP(inArray[2], HEX);
  if (inArray[3] < 16) DP("0");
  DPLN(inArray[3], HEX);
#endif
}

// SysEx banks are sent as "split bytes" where each
// byte the of the SysEx Voice Format 0 (FM 2OPERATOR VOICE DATA)
// is sent in two 4-bit pieces
#define SYSEXBANK_SIZE 33
#define SYSEXHEAD 4
#define SYSEXFOOT 2
#define SYSEX_SIZE (SYSEXBANK_SIZE*2 + SYSEXHEAD + SYSEXFOOT)
byte sysExVoice[SYSEXBANK_SIZE];

int getSysExBank () {
  return sysExVoice[0];
}

byte getSinTbl (int op) {
  if (op == 2) {
    // SIN TBL (C)
    return sysExVoice[12] >> 6;
  } else {
    // SIN TBL (M)
    return sysExVoice[11] >> 6;
  }
}

void setSinTbl (int op, byte sintbl) {
  if (sintbl > 3) {
    return;
  }
  if (op == 2) {
    // SIN TBL (C)
    sysExVoice[12] = (sysExVoice[12] & 0x3F) + (sintbl << 6);
  } else {
    // SIN TBL (M)
    sysExVoice[11] = (sysExVoice[11] & 0x3F) + (sintbl << 6);
  }
}

void recvSysExBank (byte *msg, unsigned msgsize) {
  //DP("recvSysExBank: size = ");
  //DP(msgsize);
  if (msgsize < SYSEX_SIZE) {
    //DPLN(" : SysEx too small");
    return;
  }
  if ((msg[0] != 0xF0) ||
      (msg[1] != 0x43) ||
      (msg[2] != 0x76) ||
      (msg[3] != 0x00) ||
      (msg[SYSEX_SIZE-1] != 0xF7)) {
    // Not a SysEx Voice Format Message
    //DPLN(" : SysEx not Voice Format");
    return;
  }
  //DP("\n");

  // Format has the bank number as the first data value
  // So if we get this far, only process Bank 0.
  //
  // NB: The bank number is split over the first two
  //     bytes of data, taking the lowest 4 bits of each.
  //     But as I'm after Bank 0, that is easy to check!
  //
  //     To check for one of banks 0 to 4 just check msg[5].
  //
  // NB: Should probably check the checksum, but I'm
  //     not bothering...
  //
  if ((msg[4] != 0) || (msg[5] != PSS680BANK)) {
    //DP("Ignoring Bank No ");
    //DPLN((msg[4]<<4) + (msg[5] & 0x0F));
    return;
  }

  // As soon as the message is dumped out to the serial port,
  // this function is doing too much to process further messages...
  // (you get further if debugging at 115200 rather than 9600 mind.
  midiSysExDump(msg, msgsize);

  // Now de-split and store the data!
  for (int i=0; i<SYSEXBANK_SIZE; i++) {
    int seIdx = SYSEXHEAD + i*2;
    sysExVoice[i] = (msg[seIdx]<<4) + (msg[seIdx+1] & 0x0F);
  }

  bankLoaded = true;
}

void sendSysExBank () {
  byte se[SYSEX_SIZE];

  se[0] = 0xF0;
  se[1] = 0x43;
  se[2] = 0x76;
  se[3] = 0x00;

  for (int i=0; i<SYSEXBANK_SIZE; i++) {
    se[4+i*2] = sysExVoice[i] >> 4;
    se[4+i*2+1] = sysExVoice[i] & 0x0F;
  }

  // Calculate the checksum.  Datasheet states:
  // "Two's complement of 7bits sum of all data bytes"
  // This works on the data part only (no header/footer).
  byte cs = 0;
  for (int i=SYSEXHEAD; i<SYSEX_SIZE-SYSEXFOOT; i++) {
    cs = (cs + se[i]) & 0x7F; // Ignore anything overflowing 7-bits
  }
  // Twos complement:
  //   Invert value
  //   Add 1
  //   Take 7 least significant bits
  cs = ((~cs) + 1) & 0x7F;
  se[SYSEX_SIZE-2] = cs;
  se[SYSEX_SIZE-1] = 0xF7;
  midiSysExDump (se, SYSEX_SIZE);
  MIDI.sendSysEx (SYSEX_SIZE, se, true);
  MIDI.sendProgramChange(100+PSS680BANK, MIDI_CHANNEL);
}
