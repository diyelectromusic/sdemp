/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Xiao 8Angle Tone MIDI Step Sequencer
//  https://diyelectromusic.wordpress.com/2023/04/12/xiao-samd21-arduino-and-midi-part-7/
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
  Using principles from the following Arduino tutorials:
    Analog Input  - https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogInput
    Button        - https://www.arduino.cc/en/Tutorial/Button
    Tone Melody   - https://www.arduino.cc/en/Tutorial/BuiltInExamples/toneMelody
    M5 8Angle     - https://docs.m5stack.com/en/unit/UNIT%208Angle
    Arduino MIDI Library - https://github.com/FortySevenEffects/arduino_midi_library
    Xiao Getting Started - https://wiki.seeedstudio.com/Seeeduino-XIAO/
*/
#include <MIDI.h>
#include <USB-MIDI.h>
#include <M5_ANGLE8.h>

// Comment out for no speaker output
#define SPEAKER  3  // D3 on the XIAO Expander board is the speaker

// Comment out for no tempo control
#define TEMPO_POT A0 // Xiao Expander board Grove connector
#define TEMPO_DEFAULT 120  // bbm

// comment out for no mode switch
#define MODE_SW 1 // Xiao Expander board switch

#define MIDI_CHANNEL 1

// This is required to set up the MIDI library on the default serial port.
MIDI_CREATE_DEFAULT_INSTANCE();

// This initialised USB MIDI.
USBMIDI_CREATE_INSTANCE(0, UMIDI);

M5_ANGLE8 angle8;

#define NUM_POTS ANGLE8_TOTAL_ADC
#define MIN_POT_READING 16 // Value for the lowest note

#define MIDI_BASE_NOTE 23 // B0

// Frequences for each note are taken from toneMelody
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978

// Provide a list of the notes we want to be able play
int notes[] = {
 NOTE_B0,
 NOTE_C1, NOTE_CS1, NOTE_D1, NOTE_DS1, NOTE_E1, NOTE_F1, NOTE_FS1, NOTE_G1, NOTE_GS1, NOTE_A1, NOTE_AS1, NOTE_B1,
 NOTE_C2, NOTE_CS2, NOTE_D2, NOTE_DS2, NOTE_E2, NOTE_F2, NOTE_FS2, NOTE_G2, NOTE_GS2, NOTE_A2, NOTE_AS2, NOTE_B2,
 NOTE_C3, NOTE_CS3, NOTE_D3, NOTE_DS3, NOTE_E3, NOTE_F3, NOTE_FS3, NOTE_G3, NOTE_GS3, NOTE_A3, NOTE_AS3, NOTE_B3,
 NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
 NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,
 NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,
 NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7,
 NOTE_C8, NOTE_CS8, NOTE_D8, NOTE_DS8
};

int numNotes;
int playingStep;
int lastStep;
int nextStep;
int playingNote;
unsigned tempo;
int mode;
int lastSwitch;
bool speaker;

#define LED LED_BUILTIN
bool ledon;

void ledInit () {
#ifdef LED
  pinMode(LED, OUTPUT);
#endif
  ledOff();
}

void ledOn () {
#ifdef LED
  digitalWrite(LED, LOW);
#endif
  ledon = true;
}

void ledOff () {
#ifdef LED
  digitalWrite(LED, HIGH);
#endif
  ledon = false;
}

void ledCount (int count) {
  for (int i=0; i<count; i++) {
    ledOn();
    delay(200);
    ledOff();
    delay(200);
  }
}

void ledToggle () {
  if (ledon) {
    ledOff();
  } else {
    ledOn();
  }
}

void midiNoteOn (int note) {
  playingNote = MIDI_BASE_NOTE + note;
  MIDI.sendNoteOn(playingNote, 64, MIDI_CHANNEL);
  UMIDI.sendNoteOn(playingNote, 64, MIDI_CHANNEL);
}

void midiNoteOff () {
  if (playingNote != 0) {
    MIDI.sendNoteOff(playingNote, 0, MIDI_CHANNEL);
    UMIDI.sendNoteOff(playingNote, 0, MIDI_CHANNEL);
  }
  playingNote = 0;
}

void speakerOn (int freq) {
#ifdef SPEAKER
  if (speaker) {
    tone (SPEAKER, freq);
  }
#endif
}

void speakerOff () {
#ifdef SPEAKER
  noTone (SPEAKER);
#endif
}

void setup() {
  // This is a programming trick to work out the number of notes we've listed
  numNotes = sizeof(notes)/sizeof(notes[0]);

  ledInit();

  playingStep = 0;
  lastStep = 0;
  nextStep = 1;
  tempo = TEMPO_DEFAULT;
  playingNote = 0;

#ifdef MODE_SW
  pinMode(MODE_SW, INPUT_PULLUP);
#endif
  mode = 0;
  speaker = true;

  // Ensure we call speakerOn prior to the first call to speakerOff
  // otherwise it will crash!
  speakerOn(20);
  speakerOff();

  while (!angle8.begin(ANGLE8_I2C_ADDR)) {
      delay(500);
      ledToggle();
  }
  ledOff();
  delay(1000);

  MIDI.begin(MIDI_CHANNEL_OFF);
  UMIDI.begin(MIDI_CHANNEL_OFF);
}

void loop() {
  // Loop through each note in the sequence in turn, playing
  // the note defined by the potentiomer for that point in the sequence.

  if (playingStep >= NUM_POTS) {
    playingStep = 0;
  } else if (playingStep < 0) {
    playingStep = NUM_POTS-1;
  }
  
  angle8.setLEDColor(lastStep, 0, 0);
  angle8.setLEDColor(playingStep, 0x808080, 128);
  lastStep = playingStep;

  if (angle8.getDigitalInput()) {
    // NB: Need to reverse the reading so that clockwise increases note
    int potReading = 4095 - angle8.getAnalogInput(playingStep, _12bit);

    // if the reading is zero (or almost zero), turn it off
    if (potReading < MIN_POT_READING) {
      angle8.setLEDColor(playingStep, 0, 0);
      speakerOff();
      midiNoteOff();
    } else {
      // This is a special function that will take one range of values,
      // in this case the analogue input reading between 0 and 4095, and
      // produce an equivalent point in another range of numbers, in this
      // case choose one of the notes in our list.
      int pitch = map(potReading, MIN_POT_READING, 4095, 0, numNotes-1);

      speakerOn(notes[pitch]);
      midiNoteOff();
      midiNoteOn(pitch);
    }
  } else {
    // Sequencer has stopped
    speakerOff();
    midiNoteOff();
    angle8.setLEDColor(playingStep, 0, 0);
  }

#ifdef TEMPO_POT
  // Check tempo pot
  tempo = 60 + (analogRead (TEMPO_POT)>>2);  // 0 to 255 added to 60
#else
  tempo = TEMPO_DEFAULT;
#endif

  // Move on to the next note after a short delay.
  // Tempo = beats per minute or beats per 60 seconds.
  // So time per beat = 60 / Tempo
  // So mS delay per beat = 60000 / Tempo
  // 250 = 1/4 of a second so this is same as 240 beats per minute.
  unsigned delval = 60000UL / (unsigned)tempo;
  delay(delval);

#ifdef MODE_SW
  // Check mode switch
  int modesw = digitalRead(MODE_SW);
  if ((modesw == LOW) && (lastSwitch == HIGH)) {
    // HIGH to LOW means switch was pressed
    mode++;
    switch (mode) {
      case 1:
        speaker = false;
        speakerOff();
        nextStep = 1;
        break;
      case 2:
        // Reverse
        speaker = true;
        nextStep = -1;
        break;
      case 3:
        speaker = false;
        speakerOff();
        nextStep = -1;
        break;
      default:
        // Default mode = normal running
        mode = 0;
        speaker = true;
        nextStep = 1;
        break;
    }
    ledCount(mode+1);
  }
  lastSwitch = modesw;
#endif
  playingStep += nextStep;
}
