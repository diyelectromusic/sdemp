/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Light Tone Generator
//  https://diyelectromusic.wordpress.com/2020/12/28/arduino-light-tone-generator/
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
    Analog Input        - https://www.arduino.cc/en/Tutorial/Potentiometer
    Tone Pitch Follower - https://www.arduino.cc/en/Tutorial/BuiltInExamples/tonePitchFollower
*/

int potPin = A5;
int SPEAKER = 10;
int DISABLE = 12;
int MAX_SENSOR_VALUE = 800; // Set to 1023 for full range
int MIN_SENSOR_VALUE = 200; // Set to 0 for full range

void setup() {
  // put your setup code here, to run once:
  pinMode (DISABLE, INPUT_PULLUP);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  int potReading = analogRead (potPin);
  Serial.println (potReading);

  // if the reading is zero or if the "switch" is active
  // (which may just include linking the DISABLE pin to ground)
  // then switch it off.
  if ((potReading == 0) || (digitalRead(DISABLE) == LOW)) {
    noTone (SPEAKER);
  } else {
    // This is a special function that will take one range of values,
    // in this case the analogue input reading between 0 and 1023, and
    // produce an equivalent point in another range of numbers, in this
    // case a range of frequencies to use for the pitch (120 to 1500 Hz).
    int pitch = map(potReading, MIN_SENSOR_VALUE, MAX_SENSOR_VALUE, 120, 1500);

    tone (SPEAKER, pitch);
  }
}
