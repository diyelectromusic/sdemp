/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Single Pin MIDI Channel Selector
//  https://diyelectromusic.wordpress.com/2021/02/06/single-pin-midi-channel-selector/
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
    Arduino Analog Input - https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogInput
    Read Multiple Switches using ADC - https://www.edn.com/read-multiple-switches-using-adc/
*/
#define MIDI_CHANNEL_PIN A0
#define MIDI_CHANNELS 16

// The following analog values were calculated for four switches
// using resistor values of 1K, 2K, 4.7K, 8.2K.
//
// See the blog post for details.
//
#define ALG_ERR 8
int algValues[MIDI_CHANNELS] = {
  0,111,179,257, 330,383,417,458, 512,541,561,585, 610,629,643,659
};

uint8_t readChannel (void) {
  int val = analogRead (MIDI_CHANNEL_PIN);
  Serial.print("Read Value: ");
  Serial.print(val, DEC);
  Serial.print("\t");

  // As the exact value may change slightly due to errors
  // or inaccuracies in the readings, we take the calculated
  // values and look for a range of values either side.
  //
  // To save having to have lots of "if >MIN and <MAX" statements
  // I just start at the highest and look for matches and if found, stop.
  //
  // Count down from highest to lowest, but note that
  // we don't handle "0" in the loop - that is left as a default
  // if nothing else is found...
  //
  for (int i=MIDI_CHANNELS-1; i>0; i--) {
    // Start at the highest and count backwards
    if (val  > (algValues[i] - ALG_ERR)) {
      // We have our match
      // Recall that MIDI channels are 1-indexed
      return i+1;
    }
  }

  // If we get this far, we had no match further up the list
  // so the default position is to assume MIDI channel 1.
  return 1;
}

void setup() {
   Serial.begin(9600);
   Serial.println("MIDI Switch Channel Test Reader");
}

void loop() {
  uint8_t ch = readChannel();

  Serial.print("MIDI Channel Selected: ");
  Serial.println(ch, DEC);

  // Read it once every 5 seconds
  delay(1000);
}
