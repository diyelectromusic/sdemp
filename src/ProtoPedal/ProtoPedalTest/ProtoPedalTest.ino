/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Proto Pedal Arduino Test
//  https://diyelectromusic.com/2024/09/10/proto-pedal-pcb-usage-notes/
//
      MIT License
      
      Copyright (c) 2024 diyelectromusic (Kevin)
      
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
    Secrets of Arduino PWM  - https://www.arduino.cc/en/Tutorial/SecretsOfArduinoPWM
    OpenMusic Labs Dual PWM - https://www.openmusiclabs.com/learning/digital/pwm-dac/dual-pwm-circuits/
    ATmega328PB Data Sheet  - "TC1, 3, 4 - 16-bit Timer/Counter1, 3, 4 with PWM"
*/

void initPwm() {
  // Initialise Timer 1 as follows:
  //    Output on pin 9 (OC1A) and 10 (OC1B).
  //    Run at CPU clock speed (16MHz).
  //    Use PWM Fast mode (count up then reset).
  //    Use ICR1 as the TOP value.
  //    TOP value = 63 for 6-bit resolution.
  //    PWM value is updated by writing to OCR1A and OCR1B.
  //
  // So set up PWM for Timer 1, Output A:
  //   WGM1[3:0] = 1110 = FastPWM; 16-bit; ICR1 as TOP; TOV1 set at TOP
  //   COM1A[1:0] = 10 = Clear OC1A on cmp match(up); Set OC1A at BOTTOM
  //   COM1B[1:0] = 10 = Clear OC1B on cmp match(up); Set OC1B at BOTTOM
  //   CS1[2:0] = 001 = No prescalar
  //
  pinMode(9, OUTPUT);  // OC1A output
  pinMode(10, OUTPUT); // OC1B output
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
  TCCR1C = 0;
  ICR1 = 63; // 6-bit resolution
}

// 12-bit PWM value assumed
void setPwm (uint16_t pwmval) {
  // Max for 12-bits
  if (pwmval > 0x0FFF) {
    // NB: Don't just (pwmval & 0xFFF), want to
    //     actually set to max value.
    pwmval = 0x0FFF;
  }
  OCR1A = pwmval >> 6;   // Most significant 6-bits
  OCR1B = pwmval & 0x3F; // Least significant 6-bits
}

void setup() {
//  Serial.begin(9600);
  initPwm();
}

void loop() {
  // Free running
  uint16_t val = analogRead(A0);
  // Convert 10-bit value to 12-bit value
  setPwm (val<<2);
}
