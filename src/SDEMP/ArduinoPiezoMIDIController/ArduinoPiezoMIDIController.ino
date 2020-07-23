/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Piezo MIDI Controller
//  https://diyelectromusic.wordpress.com/2020/07/19/arduino-piezo-midi-controller/
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
    Knock                   - https://www.arduino.cc/en/Tutorial/Knock
    MIDI                    - https://www.arduino.cc/en/Tutorial/Midi
    Serial Plotter          - https://arduinogetstarted.com/tutorials/arduino-serial-plotter
    DIY MIDI Percussion Kit - https://arduinoplusplus.wordpress.com/2020/05/06/diy-midi-percussion-kit-part-1/

*/
//#define DEBUG     1   // Will print MIDI information to serial port rather than send over MIDI.
//#define CALIBRATE 1   // NB: Need both DEBUG and CALIBRATE to calibrate sensors.

#define THRESHOLD 300  // Value at which the drum triggers
#define EXCLUDED  100   // Time in mS to ignore the signal (to debounce)
#define MAXDRUMS  5    // Number of drums for which we have sensors (max 6)

int excluded[MAXDRUMS];
unsigned long timeon[MAXDRUMS];
uint8_t drums[MAXDRUMS] = {
  // These are the MIDI drums (notes) to be played for each sensor.
  // General MIDI defines channel 10 as the drum channel.
  // Choose some of the following as required (up to a total of MAXDRUMS).

  // Drum 1 - Hi-hat
  42,  // Closed hi-hat
//  44,  // Pedal hi-hat
//  46,  // Open hi-hat

  // Drum 2 - Bass
  35,  // Acoustic bass drum
//  36,  // Bass drum 1

  // Drum 3 - Snare
  38,  // Acoustic snare
//  40,  // Electric snare

  // Drum 4 - Tom
//  41,  // Low floor tom
//  43,  // High floor tom
//  45,  // Low tom
  47,  // Low-mid tom
//  48,  // Hi-mid tom
//  50,  // Hi tom

  // Drum 5 - Cymbal
  49,  // Crash cymbal 1
//  55,  // Splash cymbal
//  57,  // Crash cymbal 2
//  51,  // Ride cymbal 1
//  53,  // Ride bell
//  59,  // Ride cymbal 2  
};
#define DRUM_CHANNEL  10
#define NOTE_VELOCITY 90

int sensorPin;
#ifdef CALIBRATE
#define SENSBUFFER 500
int sensordata[SENSBUFFER];
#endif

void setup() {
  sensorPin = 0;

  // Initialise the MIDI handling for the serial port
  // MIDI is transmitted using a baud rate of 31250
#ifndef DEBUG
  Serial.begin (31250);
#else
  Serial.begin (115200);
#endif
}

void loop() {
  // Each scan through the loop we want to:
  //   Read one of the sensors
  //   Process the "time out" values of the other sensors
  //   If the sensor is triggered, send the noteOn event
  //   Process any noteOff events
  sensorPin++;
  if (sensorPin >= MAXDRUMS) sensorPin = 0;
#ifdef CALIBRATE
  calibrateSensor(sensorPin);
  return; // Don't do anything else if we are calibrating the sensor
#endif
  int value = analogRead (A0+sensorPin);

  // Now check the status of every sensor we are monitoring
  for (int sensor=0; sensor<MAXDRUMS; sensor++) {
    // First check to see if we are still timing out
    if (timeon[sensor] != 0) {
      // This sensor was previously activated
      if (millis() > timeon[sensor]+EXCLUDED) {
        // It has now reached its end time though
        noteOff (sensor);
        timeon[sensor] = 0;
      }
    }
    else {
      // This sensor can be read
      if (sensor == sensorPin) {
        // and has been read
        if (value > THRESHOLD) {
          timeon[sensor] = millis();
          noteOn(sensor);
        }
      }
    }
  }
}

void noteOn (int drum) {
  byte cmd = 0x90 + DRUM_CHANNEL - 1;
#ifndef DEBUG
  // Send a MIDI "note on" command (0x90-0x9F)
  Serial.write (cmd);
  Serial.write (drums[drum]);
  Serial.write (NOTE_VELOCITY);
#else
  Serial.print ("NoteOn:  ");
  Serial.print (cmd, HEX);
  Serial.print ("  ");
  Serial.println (drums[drum]);
#endif
}

void noteOff (int drum) {
  byte cmd = 0x80 + DRUM_CHANNEL - 1;
#ifndef DEBUG
  // Send a MIDI "note of" command (0x80-0x8F)
  Serial.write (cmd);
  Serial.write (drums[drum]);
  Serial.write (0); 
#else
  Serial.print ("NoteOff: ");
  Serial.print (cmd, HEX);
  Serial.print ("  ");
  Serial.println (drums[drum]);
#endif
}

#ifdef CALIBRATE
void calibrateSensor (int sensor) {
  int value = analogRead (sensor);
  if (value < THRESHOLD) {
    // ignore this one
    return;
  }

  // Keep reading from the sensor
  for (int i=0; i<SENSBUFFER; i++) {
    sensordata[i] = analogRead(sensor);
  }

  // Now dump the lot to the terminal.
  // This can be read using the Arduino "Serial.Plotter" function from the IDE.
  for (int i=0; i<SENSBUFFER; i++) {
    Serial.println (sensordata[i]);
  }
}
#endif
