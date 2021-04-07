/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Arduino MT-32 Program Changer
//  https://diyelectromusic.wordpress.com/2021/04/07/arduino-mt-32-program-changer/
//
      MIT License
      
      Copyright (c) 2021 diyelectromusic (Kevin)
      
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
#define MT32VOICES  128
#define MT32GROUPS  17
#define MT32SIZE    11
#define MT32MAXVING 11

const char PROGMEM mt32groups[MT32GROUPS][MT32SIZE]={
"Piano",
"Organ",
"Keybrd",
"S-Brass",
"SynBass",
"Synth-1",
"Synth-2",
"Strings",
"Guitar",
"Bass",
"Wind-1",
"Wind-2",
"Brass",
"Mallet",
"Special",
"Percusn",
"Effects"
};

uint8_t mt32VinG[MT32GROUPS]={
    8,8,8,4,4,8,8,11,5,8,6,10,9,8,7,10,6
};

const char PROGMEM mt32voices[MT32VOICES][MT32SIZE]={
/* 8x Piano */     "AcouPiano1","AcouPiano2","AcouPiano3","ElecPiano1","ElecPiano2","ElecPiano3","ElecPiano4","Honkytonk",
/* 8x Organ */     "Elec Org 1","Elec Org 2","Elec Org 3","Elec Org 4","Pipe Org 1","Pipe Org 2","Pipe Org 3","Accordion",
/* 8x Keybrd */    "Harpsi 1","Harpsi 2","Harpsi 3","Clavi 1","Clavi 2","Clavi 3","Celesta 1","Celesta 2",
/* 4x SynBrass */  "Syn Brass1","Syn Brass2","Syn Brass3","Syn Brass4",
/* 4x SynBass */   "Syn Bass 1","Syn Bass 2","Syn Bass 3","Syn Bass 4",
/* 8x Synth 1 */   "Fantasy","Harmo Pan","Chorale","Glasses","Soundtrack","Atmosphere","Warm Bell","Funny Vox",
/* 8x Synth 2 */   "Echo Bell","Ice Rain","Oboe 2001","Echo Pan","DoctorSolo","Schooldaze","BellSinger","SquareWave",
/* 11x Strings */  "Str Sect 1","Str Sect 2","Str Sect 3","Pizzicato","Violin 1","Violin 2","Cello 1","Cello 2","Contrabass","Harp 1","Harp 2",
/* 5x Guitar */    "Guitar 1","Guitar 2","Elec Gtr 1","Elec Gtr 2","Sitar",
/* 8x Bass */      "Acou Bass1","Acou Bass2","Elec Bass1","Elec Bass2","Slap Bass1","Slap Bass2","Fretless 1","Fretless 2",
/* 6x Wind 1 */    "Flute 1","Flute 2","Piccolo 1","Piccolo 2","Recorder","Pan Pipes",
/* 10x Wind 2 */   "Sax 1","Sax 2","Sax 3","Sax 4","Clarinet 1","Clarinet 2","Oboe","Engl Horn","Bassoon","Harmonica",
/* 9x Brass */     "Trumpet 1","Trumpet 2","Trombone 1","Trombone 2","Fr Horn 1","Fr Horn 2","Tuba","Brs Sect 1","Brs Sect 2",
/* 8x Mallet */    "Vibe 1","Vibe 2","Syn Mallet","Wind Bell","Glock","Tube Bell","Xylophone","Marimba",
/* 7x Special */   "Koto","Sho","Shakuhachi","Whistle 1","Whistle 2","BottleBlow","BreathPipe",
/* 10x Percusn" */ "Timpani","MelodicTom","Deep Snare","Elec Perc1","Elec Perc2","Taiko","Taiko Rim","Cymbal","Castanets","Triangle",
/* 6x Effects */   "Orche Hit","Telephone","Bird Tweet","OneNoteJam","WaterBells","JungleTune",
};
