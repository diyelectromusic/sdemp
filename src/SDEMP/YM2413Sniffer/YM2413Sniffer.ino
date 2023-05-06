/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
// Arduino YM2413 Sniffer
// https://diyelectromusic.wordpress.com/2023/05/06/arduino-opl-bus-sniffer/
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
    MD YM2413 - https://arduinoplusplus.wordpress.com/2020/02/08/making-music-with-a-yamaha-ym2413-synthesizer-part-1/
    PORT IO   - https://docs.arduino.cc/hacking/software/PortManipulation
    Interrupts- https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
*/

// The YM2413 has the following pin interface:
//   D_PIN = D0-7 eight data lines of the YM2413
//  WE_PIN = WE of the YM2413
//  A0_PIN = A0 of the YM2413
// Note CS of the YM2413 is connected to GND
//
// D_PIN chosen to use single-port access from
// PORTD and PORTB:
//   PD4-7: D4-D7
//   PB0-3: D8-D11
//
// This leaves D2/D3 free so that an external
// interrupt can be used to trigger the reading.
//
// NB: D8-D11 mapped to D0-D3 on YM2413.
//     D4-D7 mapped to D4-D7 on YM2413.
const uint8_t D_PIN[] = { 8, 9, 10, 11, 4, 5, 6, 7 };
const uint8_t WE_PIN = 2; // Will be used with an interrupt
const uint8_t A0_PIN = 3;

#define BUF 256 // allows for auto wrap around of 8-bit index
uint8_t ym_addr[BUF];
uint8_t ym_data[BUF];
volatile uint8_t widx; // Write pointer
uint8_t ridx; // Read pointer

void setup() {
  Serial.begin(9600);

  // Initialise all pins as digital INPUT pins
  for (int i=0; i<8; i++) {
    pinMode(D_PIN[i], INPUT);
  }
  pinMode(WE_PIN, INPUT);
  pinMode(A0_PIN, INPUT);

  widx = 0;
  ridx = 0;

  // Setup the external interrupt on the WE pin (D2)
  // to trigger when WE goes from HIGH to LOW.
  attachInterrupt(digitalPinToInterrupt(WE_PIN), ymSampler, FALLING);
}

// Interrupt routine to run when WE goes LOW
// signifying that data on A0/D0-D7 is significant.
//
uint8_t addr;
void ymSampler() {
  // The write sequence is as follows:
  //  Taken from: https://arduinoplusplus.wordpress.com/2020/02/22/making-music-with-a-yamaha-ym2413-synthesizer-part-2/
  //
  //  Set A0 = LOW (activate address register)
  //  Set D0-D7 to address
  //  Set /WE = LOW to latch address
  //  Wait for 12 clock cycles (approx 4uS)
  //  Set /WE = HIGH
  //
  //  Set A0 = HIGH (active data register)
  //  Set D0-D7 to data
  //  Set /WE = LOW to latch data
  //  Wait for 84 clock cycles (approx 25uS)
  //  Set /WE = HIGH
  //
  // So monitoring algorithm is:
  //   Keep reading D0-D7, A0, WE
  //   IF WE == LOW:
  //     IF A0 == LOW:
  //        Read D0-D7 as an address
  //     ELSE
  //        Read D0-D7 as data

  // Recall D0-D7 on YM2413 = D8-D11,D4-D7 on Arduino
  uint8_t pB = PINB;
  uint8_t pD = PIND;
  uint8_t rD07 = (pD & 0xF0) | (pB & 0x0F);
//  uint8_t rWE = (pD & 0x04);  // D2
  uint8_t rA0 = (pD & 0x08);  // D3

  // If this routine is running then WE has gone HIGH to LOW
  if (rA0 != 0) {
    // HIGH means this is a data byte.
    // Store the address and data together.
    // Note: It is possible to have several data writes
    //       to the same address without having to change the address.
    ym_addr[widx] = addr;
    ym_data[widx] = rD07;
    widx++; // auto wrap around
  } else {
    // LOW means this is an address byte.
    // Just store the address ready for the next data operation.
    addr = rD07;
  }
}

void loop () {
  while (widx != ridx) {
    // There has been new data:
    uint8_t adr = ym_addr[ridx];
    uint8_t dat = ym_data[ridx];
    Serial.print(widx);
    Serial.print("\t");
    Serial.print(ridx);
    Serial.print("\t0x");
    Serial.print(adr, HEX);
    Serial.print(" = 0x");
    Serial.print(dat, HEX);
    Serial.print("\n");
    ridx++; // auto wrap around
  }
}
