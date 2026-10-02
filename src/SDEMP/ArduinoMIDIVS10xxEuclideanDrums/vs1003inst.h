/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  MIDI Instrument Definitions for the VS1003 Synth Module
//  https://diyelectromusic.wordpress.com/2021/01/09/arduino-midi-vs1003-synth/
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
#ifndef VS1003INST_H
#define VS1003INST_H

/*
 * MIDI Instrument Definitions for the VS1003
 *   Defined in terms of GM Level 1 Voices (1 to 128)
 *   
 * Note: The VS1003 will respond to all GM Level 1 voices
 *       with one of these instruments.
 *       
 * Mappings:
 *    Piano:
 *      1 -  8  Piano
 *      
 *    Chromatic Percussion:
 *      9 -  16 Vibraphone
 *      
 *    Organ:
 *     17 -  24 Organ
 *     
 *    Guitar:
 *     25 -  29 Guitar
 *     30 -  31 Distortion Guitar
 *     
 *    Bass:
 *     33 -  40 Bass
 *     
 *    Strings:
 *     41 -  44 Violin
 *     45 -     Strings
 *     46 -  47 Vibes
 *     48 -     Steeldrum
 *     
 *    Strings (continued):
 *     49 -  52 Strings
 *     53 -  56 Pad
 *     
 *    Brass:
 *     57 -  64 Trumpet
 *     
 *    Reed:
 *     65 -  72 Sax
 *     
 *    Pipe:
 *     73 -  80 Flute
 *     
 *    Synth Lead:
 *     81 -  88 Lead
 *     
 *    Synth Pad:
 *     89 -  96 Pad
 *     
 *    Synth Effects:
 *     97 -  98 Pad
 *     99 -     Vibes
 *    100 - 104 Pad
 *    
 *    Ethnic:
 *    105 - 108 Guitar
 *    109 -     Vibes
 *    110 -     Organ
 *    111 -     Violin
 *    
 *    Percussive:
 *    112 - 119 Steeldrum
 *    
 *    Sound Effects:
 *    120 - 128 As themselves
 */

#define VS1003_PIANO       1  // 1-8
#define VS1003_VIBES       9  // 9-16, 46-47, 99, 109
#define VS1003_ORGAN      17  // 17-24, 110
#define VS1003_GUITAR     25  // 25-29, 32, 105-108
#define VS1003_DISTGTR    31  // 30-31
#define VS1003_BASS       33  // 33-40
#define VS1003_VIOLIN     41  // 41-44, 111
#define VS1003_STRINGS    49  // 45, 49-52
#define VS1003_TRUMPET    57  // 57-64
#define VS1003_SAX        66  // 65-72, 112
#define VS1003_FLUTE      74  // 73-80
#define VS1003_LEAD       81  // 81-88
#define VS1003_PAD        89  // 53-55, 89-96, 97-98, 100-104
#define VS1003_STEELDRUM 115  // 48, 56, 113-119

#define VS1003_RVSCYMB   120
#define VS1003_FRET      121
#define VS1003_BREATH    122
#define VS1003_SEA       123
#define VS1003_BIRD      124
#define VS1003_PHONE     125
#define VS1003_HELI      126
#define VS1003_CLAPS     127
#define VS1003_GUN       128

#define VS1003VOICES 23
uint8_t vs1003voices[VS1003VOICES] = {
  VS1003_PIANO,  VS1003_VIBES, VS1003_ORGAN,  VS1003_GUITAR, VS1003_DISTGTR,
  VS1003_BASS,   VS1003_VIOLIN,VS1003_STRINGS,VS1003_TRUMPET,VS1003_SAX,
  VS1003_FLUTE,  VS1003_LEAD,  VS1003_PAD,    VS1003_STEELDRUM,
  VS1003_RVSCYMB,VS1003_FRET,  VS1003_BREATH, VS1003_SEA,
  VS1003_BIRD,   VS1003_PHONE,  VS1003_HELI,  VS1003_CLAPS,  VS1003_GUN
};

/*
 * MIDI Instrument Definitions for the VS1003
 *   Defined in terms of GM Level 1 Drums (35 to 81)
 *   
 * Note: The VS1003 will respond to all GM Level 1 drum notes
 *       on the drum channel with one of these instruments.
 * 
 *       The actual GM Range is 35 to 81 but the VS1003 Responds 
 *       to the range 27 to 87 as in the comments below, which is
 *       a common extension.
 * 
 * Also note that percussion uses MIDI note indexing,
 * so is represented on the 0 to 127 MIDI "note" scale here,
 * where MIDI note 60 = C.
 * 
 * Note: some interpretations seem to treat drum note numbers
 *       in the 1-128 range, but 0-127 has more usual in my
 *       experience...
 */
#define VS1003_D_BASS     35   // 35 36 86 87
#define VS1003_D_SNARE    38   // 38 40
#define VS1003_D_HHC      42   // 42 44 71 80
#define VS1003_D_HHO      46   // 46 55 58 72 74 81
#define VS1003_D_HITOM    50   // 48 50
#define VS1003_D_LOTOM    45   // 41 43 45 47
#define VS1003_D_CRASH    57   // 49 57
#define VS1003_D_RIDE     51   // 34 51 52 53 59
#define VS1003_D_TAMB     54   // 39 54
#define VS1003_D_HICONGA  62   // 60 62 65 78
#define VS1003_D_LOCONGA  64   // 61 63 64 66 79
#define VS1003_D_MARACAS  70   // 27 29 30 69 70 73 82 
#define VS1003_D_CLAVES   75   // 28 31 32 33 37 56 67 68 75 76 77 83 84 85


#endif /* VS1003INST_H */
