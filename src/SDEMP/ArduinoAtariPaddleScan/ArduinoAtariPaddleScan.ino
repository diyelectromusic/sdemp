/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino Atari Paddle Scan
//  https://diyelectromusic.com/2025/06/22/atari-2600-controller-shield-pcb-revisited-part-2/
//
      MIT License
      
      Copyright (c) 2025 diyelectromusic (Kevin)
      
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
    Arduino Atari Paddles - https://diyelectromusic.com/2025/06/16/atari-2600-controller-shield-pcb-revisited/
    Tom Almy "Far Inside the Arduino"
*/
#include <TimerOne.h>

#define NUMPADS 4
int pad_pins[NUMPADS]={A0, A1, A2, A3};

#define ALG_MAX    900  // Threshold value for "top"
#define RAW_START   10  // Equivalent TICKS count for alg=0
#define RAW_END    900  // Equivalent TICKS count for alg=1023
#define RAW_BREAK  1000 // Max #TICKs
#define RAW_TICK   100  // TICK in uS

// Output pins for timing measurements
#define T_START 2 // D2
#define T_TIMER 3 // D3
#define T_ADC   4 // D4
#define T_LOOP  5 // D5

//
// Atari paddle control reading.
//
// Everything from this point on
// is hard coded for reading 4 paddles
// on A0, A1, A2, A3 only.
//

// Note: scan all analog values together.
unsigned padState;
unsigned padCount[4];
unsigned atariValue[4];

void atariAnalogSetup() {
  Timer1.initialize(RAW_TICK);
  Timer1.attachInterrupt(atariAnalogScan);
  padState = 0;
}

// Occurs every timer interrupt.
void atariAnalogScan (void) {
  digitalWrite(T_TIMER, HIGH);
  if (padState == 0) {
    digitalWrite(T_START, HIGH);

    // Drop the lines to GND.
    // Set all to OUTPUTS.
    // Set all LOW.
    // Set all back to INPUTS.
    DDRC  = DDRC | 0x0F;     // A0-A3 set to OUTPUT
    PORTC = PORTC & ~(0x0F); // A0-A3 set to LOW (0)
    padState++;
  } else if (padState == 1) {
    DDRC  = DDRC & ~(0x0F);  // A0-A3 set to INPUT
    for (int i=0; i<4; i++) {
      padCount[i] = 0;
    }

    fastAnalogReadStart();
    padState++;
  } else if (padState < 5) {
    // Allow some time for the ADC to grab values for all pins
    for (int i=0; i<4; i++) {
      padCount[i]++;
    }
    padState++;
    if (padState==4) {
      digitalWrite(T_START, LOW);
    }
  } else if (padState > RAW_BREAK) {
    fastAnalogReadStop();

    // Times up, so assume all values are now fixed.
    for (int i=0; i<4; i++) {
      atariValue[i] = 1023 - map(constrain(padCount[i],RAW_START,RAW_END),RAW_START,RAW_END,0,1023);
    }

    // Now start the whole process off again
    padState = 0;
  } else {
    // Read the analog values
    for (int i=0; i<4; i++) {
      if (fastAnalogRead(i) < ALG_MAX) {
        // Haven't reached max value yet
        padCount[i]++;
      }
    }
    // Update overall counter
    padState++;
  }
  digitalWrite(T_TIMER, LOW);
}

// Direct analog reading taken from
// "Far Inside the Arduino" by Tom Almy.
uint8_t adcIdx;
volatile uint16_t adcVal[4];
void fastAnalogReadStart (void) {
    // Start the analog read continuous process.
    adcIdx = 0;
    DIDR0 |= 0x3F; // Turn off all digital input buffers for A0-A7
    ADMUX = (1<<REFS0);  // Use VCC Reference and set MUX=0
    ADCSRA = (1<<ADEN) | (1<<ADSC) | (1<<ADIE) | 7; // Start with interrupts and PreScalar = 7 (125kHz)
}

void fastAnalogReadStop (void) {
  // Stop ADC, stop interrupts, turn everything off
  ADCSRA = 0;
}

uint16_t fastAnalogRead (int adc) {
  if (adc >= 4)
    return 0;

  uint16_t adcvalue;
  cli(); // disable interrupts
  adcvalue = adcVal[adc];
  sei(); // reenable interrupts
  return adcvalue;
}

// ADC reading interrupt routing
ISR(ADC_vect) {
  digitalWrite(T_ADC, HIGH);
  uint16_t last = adcVal[adcIdx];
  adcVal[adcIdx] = ADCW;
  // Move onto next MUX
  adcIdx = (adcIdx+1) % 4;
  ADMUX = (1<<REFS0) + adcIdx;
  // Start new conversion
  ADCSRA |= (1<<ADSC);
  digitalWrite(T_ADC, LOW);
}

int atariAnalogRead (int pin) {
  if ((pin >= A0) && (pin <= A3)) {
    return atariValue[pin-A0];
  } else {
    return -1;
  }
}

int toggle;
void setup() {
  Serial.begin(9600);
  atariAnalogSetup();
  pinMode(T_START, OUTPUT);
  digitalWrite(T_START, LOW);
  pinMode(T_TIMER, OUTPUT);
  digitalWrite(T_TIMER, LOW);
  pinMode(T_ADC, OUTPUT);
  digitalWrite(T_ADC, LOW);
  pinMode(T_LOOP, OUTPUT);
  digitalWrite(T_LOOP, LOW);
  toggle = LOW;
}

void loop() {
  toggle = !toggle;
  digitalWrite(T_LOOP, toggle);
  Serial.print(padState);
  Serial.print("\t[ ");
  for (int i=0; i<1; i++) {
    Serial.print(atariAnalogRead(pad_pins[i]));
    Serial.print("\t");
    Serial.print(padCount[i]);
    Serial.print("\t");
    Serial.print(adcVal[i]);
    Serial.print("\t][ ");
  }
  Serial.print("\n");
}
