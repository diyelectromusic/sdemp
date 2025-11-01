/*
      MIT License
      
      Copyright (c) 2025 emalliab.wordpress.com (Kevin)
      
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
#include "LEDPanel.h"

// Structure of buffer is designed for optimal output, so matches
// the expected structure of the panel comms.
//
// Format:
//   8 bit, mapping onto D1/D2 for four panels with assumed order:
//       b7 6 5 4 3 2 1 0
//       D8D7D6D5D4D3D2D1
//       P4  P3  P2  P1
//
//   Array of 4 "banks" where each bank is one of the address combinations
//      A1/A0 00=bank 0; 01=bank 1; 10=bank 2; 11=bank 3
//
//   384 bytes per bank representing 384 bits for LEDs across the four panels.
//
//   Bit pattern as per:
//     https://wiki.london.hackspace.org.uk/view/LED_tiles_V2
//     https://wiki.hackhitchin.org.uk/index.php?title=LED_panel_interface
//
// This means that plotting a single RGB pixel will take a fair bit of
// massaging to get it onto the right bit values for the three colours
// on the correct bank, on the correct half, of the correct panel.
//
#define FB_BANKS 4
#define FB_BITS  384
uint8_t fb[FB_BANKS][FB_BITS];
volatile bool scanning;

// Map D1/D2 for up to four panels onto consecutive data
// pins to support direct port manipulation.
//
//    Panel =  D8  D7  D6  D5  D4  D3  D2  D1
//  Arduino =   7   6   5   4  11  10   9   8
//   PORTIO = PD7 PD6 PD5 PD4 PB3 PB2 PB1 PB0
//
inline void setDataOutput (uint8_t data) {
  PORTB = (PORTB & (~0x0F)) | (data & 0x0F);
  PORTD = (PORTD & (~0xF0)) | (data & 0xF0);
}

void initDataOutput (void) {
  DDRB = DDRB | 0x0F;
  DDRD = DDRD | 0xF0;
  PORTB = PORTB & (~0x0F);
  PORTD = PORTD & (~0xF0);  
}

// All control pins are set/cleared using PORT IO too.
// This is hard-coded in following macros.
#define PANEL_A0   A0  // PC0
#define PANEL_A1   A1  // PC1
#define PANEL_CLK  A2  // PC2
#define PANEL_LAT  A3  // PC3
#define PANEL_OE   A4  // PC4
#define TIMING     A5  // PC5

#define SETADDR(x) {PORTC = (PORTC & (~0x03)) | (x & 0x03);} // A0/A1
#define A0SET  {PORTC = (PORTC & (~0x01)) | 0x01;}  // A0
#define A0CLR  {PORTC = (PORTC & (~0x01))       ;}
#define A1SET  {PORTC = (PORTC & (~0x02)) | 0x02;}  // A1
#define A1CLR  {PORTC = (PORTC & (~0x02))       ;}
#define CLKSET {PORTC = (PORTC & (~0x04)) | 0x04;}  // A2
#define CLKCLR {PORTC = (PORTC & (~0x04))       ;}
#define LATSET {PORTC = (PORTC & (~0x08)) | 0x08;}  // A3
#define LATCLR {PORTC = (PORTC & (~0x08))       ;}
#define OESET  {PORTC = (PORTC & (~0x10)) | 0x10;}  // A4
#define OECLR  {PORTC = (PORTC & (~0x10))       ;}
#define TIMSET {PORTC = (PORTC & (~0x20)) | 0x20;}  // A5
#define TIMCLR {PORTC = (PORTC & (~0x20))       ;}

void initControl (void) {
  DDRC = DDRC | 0x3F;
  PORTC = PORTC & ~0x3F;
}

//  The panel is addressed as a string of bits, representing 64 R,G,B values.
//  but the ordering is a little odd.
//
//  For each address (A1/A0) a string of bits represents two rows of LEDs.
//     64 * 2 * 3 = 384 bits of information.
//
//  Ordering is as follows:
//    [ BBBBBBBB BBBBBBBB GGGGGGGG GGGGGGGG RRRRRRRR RRRRRRRR ] * 16
//      Row0 0-7 Row4 0-7 Row0 0-7 Row4 0-7 Row0 0-7 Row4 0-7     Repeat for 8-15, 16-23, 24-31, etc...
//
//  This addresses rows 0 and 4 when A1/A0 = 00:
//      A1 A0 1st Row 2nd Row
//       0  0    0       4
//       0  1    1       5
//       1  0    2       6
//       1  1    3       7
//
//  The bit data is streamed out on D1 for the top half of the panel.
//  The same principle and format bit stream is streamed out on D2 for the second half of the panel.
//  Any additional panels will have their own D1/D2 lines and the same format bit streams.
//
//  All this means that the (x,y) coordinates must map onto the frame buffer as follows:
//     y = b76543210
//                ++--- A1/A0 lines 0-3
//               +----- First or second row
//              +------ D1 or D2
//            ++------- Panel number for four panels 0-3
//          ++--------- Only required if more than four panels (up to 16)
//
//    x = b76543210
//              +++--- Position within a block of 8 LEDs
//           +++------ Which block of 8 to address
//
//    Mapping the x coord is more complex though as the position relates to:
//        - 1st/2nd Row determines which block of 8 within each colour.
//        - Colour B/G/R determines which pair of blocks of 8.
//
//    Led Idx within a block = x[2:0]
//    Block Idx in frame buffer = x[5:3] * (16+16+16)
//    So LED position = BlockIdx + 8 (if 2nd Row) + 16 (if G) + 32 (if R) + LedIdx
//
// One final complication.  The framebuffer is a set of bytes that maps onto
// the different halves of different panels, so which bit in the framebuffer
// gets set depends on the panel number and D1/D2 selection.
//
// Assuming a simple D8-D1 mapping, this means the following:
//     b76543210
//            ++--- D2/D1 Panel 1
//          ++----- D2/D1 Panel 2
//        ++------- D2/D1 Panel 3
//      ++--------- D2/D1 Panel 4
//
void setPixel (int x, int y, panelcolour col) {
  if ((x<0) || (x>=64) || (y<0) || (y>=64)) {
    // Only 4x 16*64 panels supported
    return;
  }
  int a1a0 = y & 0x03;  // bank
  int row4 = y & 0x04 ? 8 : 0;
  int d1d2 = y & 0x08;
  int panel = y >> 4;   // panel 0..15; only 0..4 supported
  int bitIdx = y >> 3;  // panel no + d1/d2
  int led8 = x & 0x07;
  int block = x >> 3;

  // Determine the bit to act on for the four panels D1-D2
  uint8_t bit = (1<<bitIdx);
  int off = block * 48  + row4 + led8;  // Location of B LED to act on
  if (col & BITBLUE) {
    fb[a1a0][off] |= bit;
  } else {
    fb[a1a0][off] &= ~bit;
  }
  if (col & BITGREEN) {
    fb[a1a0][off+16] |= bit;
  } else {
    fb[a1a0][off+16] &= ~bit;
  }
  if (col & BITRED) {
    fb[a1a0][off+32] |= bit;
  } else {
    fb[a1a0][off+32] &= ~bit;
  }
}

// Uses above logic to return the colour value of (x,y)
panelcolour getPixel (int x, int y) {
  if ((x<0) || (x>=64) || (y<0) || (y>=64)) {
    return;
  }
  int a1a0 = y & 0x03;  // bank
  int row4 = y & 0x04 ? 8 : 0;
  int d1d2 = y & 0x08;
  int panel = y >> 4;   // panel 0..15; only 0..4 supported
  int bitIdx = y >> 3;  // panel no + d1/d2
  int led8 = x & 0x07;
  int block = x >> 3;

  // Determine the bit to act on for the four panels D1-D2
  uint8_t bit = (1<<bitIdx);
  int off = block * 48  + row4 + led8;  // Location of B LED to act on
  int col = 0;
  if ((fb[a1a0][off] & bit) != 0) {
    col |= BITBLUE;
  }
  if ((fb[a1a0][off+16] & bit) != 0) {
    col |= BITGREEN;
  }
  if ((fb[a1a0][off+32] & bit) != 0) {
    col |= BITRED;
  }

  return (panelcolour)col;
}

void fbFill (uint8_t data) {
  for (int j=0; j<FB_BANKS; j++) {
    for (int i=0; i<FB_BITS; i++) {
      fb[j][i] = data;
    }
  }
}

void panelClear (bool on) {
  if (on) {
    fbFill(0xFF);
  } else {
    fbFill(0);
  }
}

int bank;
void panelInit (void) {
  scanning = false;
  bank = 0;
  fbFill(0);
  initDataOutput();
  initControl();
  scanning = true;
  setBrightness(BRIGHTNESS_MIN);
}

// Algorithm for writing out to the panels:
//    Set A0/A1 as per the "bank"
//    Write out full 384 bitstream to D1-D8 in parallel
//    Update bank number for next time.
//
// Four calls to panelScan are require before the
// whole panel is displayed as only one set of A0/A1
// can be active at any one time so it relies
// on persistence of vision effects between scans.
//
// Full protocol is defined here:
//   https://wiki.hackhitchin.org.uk/index.php?title=LED_panel_interface
//
// I'm using the approach:
//   Load in the data
//   Blank screen and latch data
//   Set A0/A1
//   Unblank the screen
//
void panelScan (void) {
  TIMSET;

  if (scanning) {
    // Shift out each bit from the framebuffer across
    // all four panel D1/D2 lines...
    for (int i=0; i<FB_BITS; i++) {
      setDataOutput(fb[bank][i]);
      // Pulse the clock
      CLKSET;
      CLKCLR;
    }

    // Blank display
    OESET;  // active LOW

    // Set address
    SETADDR(bank);

    // Toggle latch
    LATSET;
    LATCLR;

    // Unblank display
    OECLR;  // Active LOW
  }

  bank++;
  if (bank >= FB_BANKS) bank = 0;
  
  TIMCLR;
}

// There is a special control word that will set the brightness.
//  Details here:
//     https://wiki.hackhitchin.org.uk/index.php?title=LED_panel_interface
//
//  12.5% is b000000 = 0
// 100.0% is b100000 = 32
// 200.0% is b111111 = 63
//
// It is actually a bit more complicated than this as b5 actually determines
// the use of high-current mode (1) or low-current mode (0) which means the
// graph of gain vs b[4:0] is different.
//
// This is all described in the "current gain adjustment" part of the MBI5034 datasheet.
//
// I'm only supporting up to the default value, which is actually b101011 = 0x2B
// (not b100000 as suggested above).
//
// The default value for the whole configuration register (see datasheet):
//  bit=FEDCBA9876543210
//     b0111000101101011
//
// Only last 6 bits = brightness, hence default = b101011 = 0x2B
//
// When sending, need to clock these 16 bits to each chip, but
// LAT needs to be set for the last 4 bits.
//
void setBrightness (uint8_t brightpercent) {
  uint8_t bright;
  if (brightpercent <= 12) {
    bright = 0;
  } else if ((brightpercent > 12) && (brightpercent < 100)) {
    // Scale 12-100% between 0 and default value
    bright = (brightpercent - 12) * 0x2B / 88;
  } else {
    bright = 0x2B;
  }
  scanning = false;

  uint16_t ctrl = 0b0111000101000000 | bright;

  // For each address (bank):
  //   For each chip (24 per half-panel):
  //      Toggle out ctrl value on D1 and D2
  //   Latch last 4 bits of whole transfer

  // Blank display
  OESET;  // active LOW

  // Reset everything
  LATCLR;
  CLKCLR;

  for (int a=0; a<FB_BANKS; a++) {
    // Set address
    SETADDR(a);

    for (int c=0; c<24; c++) {
      for (int b=15; b>=0; b--) {
        // Stream out MSB first
        if ((ctrl & (1<<b)) == 0) {
          setDataOutput(0);
        } else {
          // Set D1/D2 for all panels at same time
          setDataOutput(0xFF);
        }

        // Need to set LAT for last chip and last 4 bits
        if ((c == 23) && (b < 4)) {
          LATSET;
        } else {
          LATCLR;
        }

        // Pulse the clock
        CLKSET;
        CLKCLR;
      }
    }
  }
  LATCLR;

  scanning = true;
}
