/*
  // Simple DIY Electronic Music Projects
  //    diyelectromusic.wordpress.com
  //
  //  Arduino ESP32 PWM - Part 3
  //  https://diyelectromusic.wordpress.com/2024/04/03/esp32-and-pwm-part-3/
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
    ESP32 for Arduino     - https://docs.espressif.com/projects/arduino-esp32/
    ESP32 Arduino LEDC    - https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/ledc.html
*/

//#define TIMING_PIN 12
#define NUM_PWM_PINS 4
int pwm_pins[NUM_PWM_PINS] = {13, 12, 14, 27};
#define NUM_ADC_PINS 2
int adc_pins[NUM_ADC_PINS] = {36, 39};
uint16_t adcval[NUM_ADC_PINS];

// NB: Rely on fact we're using an 8-bit index to auto wrap for our 256 samples
#define NUM_SAMPLES   256
uint8_t sinedata[NUM_SAMPLES] = {
  128, 131, 134, 137, 140, 144, 147, 150, 153, 156, 159, 162, 165, 168, 171, 174,
  177, 179, 182, 185, 188, 191, 193, 196, 199, 201, 204, 206, 209, 211, 213, 216,
  218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 239, 240, 241, 243, 244,
  245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255,
  255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251, 250, 250, 249, 248, 246,
  245, 244, 243, 241, 240, 239, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220,
  218, 216, 213, 211, 209, 206, 204, 201, 199, 196, 193, 191, 188, 185, 182, 179,
  177, 174, 171, 168, 165, 162, 159, 156, 153, 150, 147, 144, 140, 137, 134, 131,
  128, 125, 122, 119, 116, 112, 109, 106, 103, 100,  97,  94,  91,  88,  85,  82,
  79,  77,  74,  71,  68,  65,  63,  60,  57,  55,  52,  50,  47,  45,  43,  40,
  38,  36,  34,  32,  30,  28,  26,  24,  22,  21,  19,  17,  16,  15,  13,  12,
  11,  10,   8,   7,   6,   6,   5,   4,   3,   3,   2,   2,   2,   1,   1,   1,
  1,   1,   1,   1,   2,   2,   2,   3,   3,   4,   5,   6,   6,   7,   8,  10,
  11,  12,  13,  15,  16,  17,  19,  21,  22,  24,  26,  28,  30,  32,  34,  36,
  38,  40,  43,  45,  47,  50,  52,  55,  57,  60,  63,  65,  68,  71,  74,  77,
  79,  82,  85,  88,  91,  94,  97, 100, 103, 106, 109, 112, 116, 119, 122, 125
};

// Following wavetables are calculated...
uint8_t sawdata[NUM_SAMPLES];
uint8_t tridata[NUM_SAMPLES];
uint8_t sqdata[NUM_SAMPLES];

// Define which pot controls which PWM channel
int pot2pwm[NUM_PWM_PINS] = {0,0,1,1};

// Use a SAMPLE_RATE that is a multiple of the basic wavetable
#define BASE_FREQ_MULT 128
#define SAMPLE_RATE (NUM_SAMPLES*BASE_FREQ_MULT)  // i.e. 256*128 = 32768 Hz

// 10 bit resoution for PWM will mean 78277 Hz PWM frequency
// 9 bit resoution for PWM will mean 156555 Hz PWM frequency
// 8 bit resoution for PWM will mean 313111 Hz PWM frequency
#define PWM_RESOLUTION 8
#define PWM_FREQUENCY  313111

// Timer configuration
// NB: CPU Freq = 80MHz so dividers used to get other
//     frequencies for syncing to other operations.
//
// For a timer frequency of 1000000Hz i.e. divider of 80,
// the Timer alarms would be configured in uS.
// So need to convert required sample rate into uS.
//    Period = 1 / Sample Rate = 1 / 32768 = 30.5uS
//
// However, if up the timer frequency to 10MHz then
// alarms will be set in 0.1uS.
//
#define TIMER_FREQ 10000000  // 1MHz * 10
#define TIMER_RATE 305       // 30.5uS * 10

// For direct digital synthesis from a wavetable
// we have an accumulator to store the index into
// the table and an increment based on the sample
// rate and frequency.
//    Increment = Frequency * (Number of Samples in wavetable / Sample Rate)
//    Increment = Frequency * (256 / 32768)
//    Increment = Frequency / 128
//
// But using a 8.8 fixed-point accumulator and increment:
//   Increment = 256 * Frequency / 128
//   Increment = Frequency * 2
//
#define FREQ2INC(f) (f*2)
uint16_t acc[NUM_PWM_PINS];
uint16_t inc[NUM_PWM_PINS];
#if 1
uint16_t mul[NUM_PWM_PINS] = {1,1,1,1};
uint8_t *pWT[NUM_PWM_PINS] = {sinedata, sawdata, tridata, sqdata};
#else
uint16_t mul[NUM_PWM_PINS] = {1,2,3,4}; // Optional frequency multipliers
uint8_t *pWT[NUM_PWM_PINS] = {sinedata, sinedata, sinedata, sinedata};
#endif
// Set the PWM frequency for one channel
void setPwmFreq (int ch, unsigned freq) {
  if (ch < NUM_PWM_PINS) {
    // First apply any optional frequency multipliers
    freq = freq * mul[ch];
    inc[ch] = FREQ2INC(freq);
  }
}

// Set the PWM frequency for all channels associated with a specific pot
void setPotFreq (int pot, unsigned freq) {
  // Find all PWM channels for this pot
  for (int i=0; i<NUM_PWM_PINS; i++) {
    if (pot2pwm[i] == pot) {
      setPwmFreq(i, freq);
    }
  }
}

// Set the PWM frequency for all channels
void setAllFreq (unsigned freq) {
  for (int i=0; i<NUM_PWM_PINS; i++) {
    setPwmFreq(i, freq);
  }
}

// Assumed to be running at SAMPLE_RATE
void ddsUpdate (int ch) {
  if (ch < NUM_PWM_PINS) {
    acc[ch] += inc[ch];
    ledcWrite (pwm_pins[ch], pWT[ch][acc[ch] >> 8]);
  }
}

// Use the ESP32 timer routines
hw_timer_t *timer = NULL;

int toggle;
void ARDUINO_ISR_ATTR timerIsr (void) {
#ifdef TIMING_PIN
  toggle = !toggle;
  digitalWrite(TIMING_PIN, toggle);
#endif
  for (int i=0; i<NUM_PWM_PINS; i++) {
    ddsUpdate(i);
  }
}

void setupWavetables () {
  for (int i=0; i<NUM_SAMPLES; i++) {
    sawdata[i] = i;
    if (i<NUM_SAMPLES/2) {
      tridata[i] = i*2;
      sqdata[i] = 255;
    } else {
      tridata[i] = 255-(i-128)*2;
      sqdata[i] = 0;
    }
  }
}

void setup () {
  Serial.begin(115200);
  setupWavetables();
#ifdef TIMING_PIN
  pinMode(TIMING_PIN, OUTPUT);
  digitalWrite(TIMING_PIN, LOW);
#endif
  timer = timerBegin(TIMER_FREQ);
  timerAttachInterrupt(timer, &timerIsr);
  timerAlarm(timer, TIMER_RATE, true, 0);
  Serial.print("Timer frequency: ");
  Serial.println(timerGetFrequency(timer));
  for (int i=0; i<NUM_PWM_PINS; i++) {
    ledcAttach(pwm_pins[i], PWM_FREQUENCY, PWM_RESOLUTION);
  }
  setAllFreq(440);
}

void loop () {
  // cycle through each defined potentiometer
  for (int i=0; i<NUM_ADC_PINS; i++) {
      uint16_t algval = analogRead(adc_pins[i]);
      if (algval != adcval[i]) {
        setPotFreq(i, algval);
      }
      adcval[i] = algval;
  }
}
