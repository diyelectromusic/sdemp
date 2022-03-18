/*
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
//  Pi Day MIDI Sequence(r)
//  https://diyelectromusic.wordpress.com/2022/03/14/pi-day-midi-sequencer/
//
      MIT License
      
      Copyright (c) 2022 diyelectromusic (Kevin)
      
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
#include <MIDI.h>
#include <HT16K33.h>

//#define DEBUG
#define SOUND_MIDI
#define SOUND_TONE
#define DISPLAY_LEDS
#define DISPLAY_I2CSEGMENT

// Need one note per digit - 0 to 9
#define NUM_NOTES 10

#ifdef SOUND_MIDI
#define MIDI_CHANNEL 1
MIDI_CREATE_DEFAULT_INSTANCE();
int notes[NUM_NOTES] = {
  60, 62, 64, 65, 67, 69, 71, 72, 74, 76
};
#endif

#ifdef DISPLAY_LEDS
#define NUM_LEDS NUM_NOTES
int leds[NUM_LEDS] = {
  2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};
#endif

#ifdef SOUND_TONE
#define TONE_PIN A0
int freqs[NUM_NOTES] = {
  // Frequencies taken from toneMelody: C4 to E5
  262, 294, 330, 349, 392, 440, 494, 523, 587, 659,
};
#endif

#ifdef DISPLAY_I2CSEGMENT
// I2C 7 Segment Display
//   C/CLK = A5/SCL
//   D/DAT = A4/SDA
HT16K33 seg(0x70);  // Pass in I2C address of the display
unsigned long dspdigit;
int numdigits;
#endif

int lastdigit;
unsigned long timing;

void soundInit () {
#ifdef SOUND_MIDI
  MIDI.begin(MIDI_CHANNEL);
#endif
#ifdef SOUND_TONE
  // No initialisation required for tone()
#endif
}

void soundNoteOn (int digit) {
#ifdef SOUND_MIDI
  MIDI.sendNoteOn(notes[digit], 127, MIDI_CHANNEL);  
#endif
#ifdef SOUND_TONE
  tone(TONE_PIN, freqs[digit]);
#endif
}

void soundNoteOff (int digit) {
#ifdef SOUND_MIDI
  MIDI.sendNoteOff(notes[digit], 0, MIDI_CHANNEL);  
#endif
#ifdef SOUND_TONE
  noTone(TONE_PIN);
#endif
}

void displayInit() {
#ifdef DISPLAY_LEDS
  for (int i=0; i<NUM_LEDS; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }
#endif
#ifdef DISPLAY_I2CSEGMENT
  // Initialise the I2C display
  seg.begin();
  Wire.setClock(100000);
  seg.displayOn();
  seg.brightness(2);
  seg.displayClear();
  dspdigit = 0;
  numdigits = 1;
#endif
}

void displayNoteOn (int digit) {
#ifdef DISPLAY_LEDS
  digitalWrite(leds[digit], HIGH);
#endif
#ifdef DISPLAY_I2CSEGMENT
  dspdigit = (dspdigit*10 + digit) % 10000;
  seg.setDigits(numdigits);
  if (numdigits<4) numdigits++;

  seg.displayInt(dspdigit);
#endif
}

void displayNoteOff (int digit) {
#ifdef DISPLAY_LEDS
  digitalWrite(leds[digit], LOW);
#endif
#ifdef DISPLAY_I2CSEGMENT
  // Nothing to do
#endif
}

void displayClear () {
#ifdef DISPLAY_LEDS
  for (int i=0; i<NUM_LEDS; i++) {
    digitalWrite(leds[i], LOW);
  }
  dspdigit = 0;
  numdigits = 0;
#endif
#ifdef DISPLAY_I2CSEGMENT
  seg.displayClear();
#endif
  
}

void setup() {
  displayInit();
  soundInit();

  for (int i=0; i<NUM_NOTES; i++) {
    displayNoteOn(i);
    soundNoteOn(i);
    delay(300);
    displayNoteOff(i);
    soundNoteOff(i);
    delay(100);
  }
  displayClear();
  displayNoteOn(3);
  delay(2000);

#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Calculating Pi");
  delay(2000);
  Serial.print("3.");
#endif
  timing = millis();
}

int decpos=1;
void loop() {
  int digit = calcPi(decpos);
  displayNoteOff(lastdigit);
  soundNoteOff(lastdigit);
  displayNoteOn(digit);
  soundNoteOn(digit);
  lastdigit = digit;

#ifdef DEBUG
  Serial.print(digit);
  if (!(decpos%8)) {
    Serial.print(" ");
  }
  if (!(decpos%32)){
    Serial.print("\t");
    unsigned long newtiming = millis();
    Serial.print(newtiming - timing);
    Serial.print("mS\n");
    Serial.print("  ");
    timing = millis();
  }
#endif

  decpos++;
}

/*
 * Code taken from a discussion on StackOverflow from here:
 * https://stackoverflow.com/questions/5905822/function-to-find-the-nth-digit-of-pi
 */
 /*
  * Computation of the n'th decimal digit of \pi with very little memory.
  * Written by Fabrice Bellard on January 8, 1997.
  * 
  * We use a slightly modified version of the method described by Simon
  * Plouffe in "On the Computation of the n'th decimal digit of various
  * transcendental numbers" (November 1996). We have modified the algorithm
  * to get a running time of O(n^2) instead of O(n^3log(n)^3).
  * 
  * This program uses mostly integer arithmetic. It may be slow on some
  * hardwares where integer multiplications and divisons must be done
  * by software. We have supposed that 'int' has a size of 32 bits. If
  * your compiler supports 'long long' integers of 64 bits, you may use
  * the integer version of 'mul_mod' (see HAS_LONG_LONG).  
  */
#include <math.h>

#define mul_mod(a,b,m) fmod( (double) a * (double) b, m)

/* return the inverse of x mod y */
int inv_mod(int x, int y)
{
    int q, u, v, a, c, t;

    u = x;
    v = y;
    c = 1;
    a = 0;
    do {
      q = v / u;
  
      t = c;
      c = a - q * c;
      a = t;
  
      t = u;
      u = v - q * u;
      v = t;
    } while (u != 0);
    a = a % y;
    if (a < 0)
      a = y + a;
    return a;
}

/* return (a^b) mod m */
int pow_mod(int a, int b, int m)
{
    int r, aa;

    r = 1;
    aa = a;
    while (1) {
      if (b & 1)
          r = mul_mod(r, aa, m);
      b = b >> 1;
      if (b == 0)
          break;
      aa = mul_mod(aa, aa, m);
    }
    return r;
}

/* return true if n is prime */
int is_prime(int n)
{
    int r, i;
    if ((n % 2) == 0)
      return 0;

    r = (int) (sqrt(n));
    for (i = 3; i <= r; i += 2)
    if ((n % i) == 0)
        return 0;
    return 1;
}

/* return the prime number immediatly after n */
int next_prime(int n)
{
    do {
      n++;
    } while (!is_prime(n));
    return n;
}

int calcPi(int digit)
{
    int av, a, vmax, N, n, num, den, k, kq, kq2, t, v, s, i;
    double sum;

    n = digit;

    N = (int) ((n + 20) * log(10) / log(2));

    sum = 0;

    for (a = 3; a <= (2 * N); a = next_prime(a)) {
      vmax = (int) (log(2 * N) / log(a));
      av = 1;
      for (i = 0; i < vmax; i++)
          av = av * a;
  
      s = 0;
      num = 1;
      den = 1;
      v = 0;
      kq = 1;
      kq2 = 1;
  
      for (k = 1; k <= N; k++) {  
          t = k;
          if (kq >= a) {
            do {
                t = t / a;
                v--;
            } while ((t % a) == 0);
            kq = 0;
          }
          kq++;
          num = mul_mod(num, t, av);
  
          t = (2 * k - 1);
          if (kq2 >= a) {
            if (kq2 == a) {
                do {
                t = t / a;
                v++;
                } while ((t % a) == 0);
            }
            kq2 -= a;
          }
          den = mul_mod(den, t, av);
          kq2 += 2;
  
          if (v > 0) {
            t = inv_mod(den, av);
            t = mul_mod(t, num, av);
            t = mul_mod(t, k, av);
            for (i = v; i < vmax; i++)
                t = mul_mod(t, a, av);
            s += t;
            if (s >= av)
                s -= av;
          }  
      }
  
      t = pow_mod(10, n - 1, av);
      s = mul_mod(s, t, av);
      sum = fmod(sum + (double) s / (double) av, 1.0);
    }
    return (int)(sum*10.0);
}
