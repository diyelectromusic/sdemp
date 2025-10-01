/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Mini Listening Post Experiments
//  
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
#ifndef __lcd2004font_h
#define __lcd2004font_h

// Large fonts are made up of 4x4 blocks
// using the range of block/triangle
// characters determined by the numbers
// 0 to 9 as follows
//
// Encoding of the blocks - mapped onto ASCII numbers
//   0 clear/space
//   1 full block
//   2 left block
//   3 right block
//   4 top block
//   5 bottom block
//   6 bottom left triangle
//   7 bottom right triangle
//   8 top left triangle
//   9 top right triangle
//
// But this uses a "compressed" format, where the
// characters are mapped onto 4-bit values.
// This means that 4 16-bit values will fully described
// a character.

// Define the 4x4 combintions using ASCII index starting
// at the numbers, then skipping a few, moving to upper
// case letters only.
#define LCD2004FONT_START 48
#define LCD2004FONT_SIZE  43
const uint16_t lcd2004font[LCD2004FONT_SIZE][4] = {
  {0x7446,0x1001,0x1001,0x9558}, // 0
  {0x0720,0x0320,0x0320,0x0110}, // 1
  {0x7446,0x0078,0x0780,0x7111}, // 2
  {0x7446,0x0078,0x0096,0x9558}, // 3
  {0x0710,0x7810,0x1111,0x0010}, // 4
  {0x1444,0x1116,0x0001,0x5558}, // 5
 // {0x7120,0x1000,0x1446,0x9558}, // 6
  {0x7440,0x1116,0x1001,0x9558}, // 6
  {0x1111,0x0078,0x0780,0x7800}, // 7
  {0x7446,0x9558,0x7446,0x9558}, // 8
  {0x7446,0x9551,0x0001,0x9558}, // 9
  {0,0,0,0}, // :
  {0,0,0,0}, // ;
  {0,0,0,0}, // <
  {0,0,0,0}, // =
  {0,0,0,0}, // >
  {0,0,0,0}, // ?
  {0,0,0,0}, // @
  {0x0760,0x7896,0x1441,0x1001}, // A
  {0x1446,0x1558,0x1446,0x1558}, // B
  {0x7446,0x1000,0x1000,0x9558}, // C
  {0x1446,0x1001,0x1001,0x1558}, // D
  {0x1444,0x1550,0x1000,0x1555}, // E
  {0x1444,0x1550,0x1000,0x1000}, // F
  {0x7446,0x1055,0x1041,0x9558}, // G
  {0x1001,0x1551,0x1001,0x1001}, // H
  {0x0110,0x0320,0x0320,0x0110}, // I
  {0x1111,0x0320,0x0320,0x9120}, // J
  {0x1078,0x1780,0x1960,0x1096}, // K
  {0x1000,0x1000,0x1000,0x1555}, // L
  {0x6007,0x1671,0x1981,0x1001}, // M
  {0x1601,0x1961,0x1091,0x1001}, // N
  {0x7446,0x1001,0x1001,0x9558}, // O
  {0x1446,0x1558,0x1000,0x1000}, // P
  {0x7446,0x1001,0x1011,0x9516}, // Q
  {0x1446,0x1078,0x1960,0x1096}, // R
  {0x7446,0x9600,0x0096,0x9558}, // S
  {0x1111,0x0320,0x0320,0x0320}, // T
  {0x1001,0x1001,0x1001,0x9558}, // U
  {0x1001,0x1001,0x9678,0x0980}, // V
  {0x6007,0x1001,0x1761,0x9898}, // W
  {0x6007,0x9678,0x7896,0x8009}, // X
  {0x9678,0x0980,0x0320,0x0320}, // Y
  {0x4441,0x0078,0x7800,0x1555}, // Z
};


#endif
