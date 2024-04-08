/*
  // Simple DIY Electronic Music Projects
  //    diyelectromusic.wordpress.com
  //
  //  Arduino ESP32 DAC Envelope Generator
  //  https://diyelectromusic.wordpress.com/2024/04/07/esp32-dac-envelope-generator/
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
*/

//#define TIMING_PIN 13
//#define TEST
//#define DUMPADSR
//#define TESTTONE 19

#define NUM_DAC_PINS 2  // Also the number of EGs possible
int dac_pins[NUM_DAC_PINS] = {25, 26};
#define NUM_ADC_PINS 4  // Per DAC
int adc_pins[NUM_DAC_PINS][NUM_ADC_PINS] = {
  14, 27, 33, 32,  // DAC/EG 1
  35, 34, 39, 36   // DAC/EG 2
};
uint16_t adcval[NUM_DAC_PINS][NUM_ADC_PINS];

// Trigger and gates - can be same pin!
int trig_pins[NUM_DAC_PINS] = {12, 12};
int gate_pins[NUM_DAC_PINS] = {13, 13};
int lasttrig[NUM_DAC_PINS];
int lastgate[NUM_DAC_PINS];

// Timer configuration
// NB: CPU Freq = 80MHz so dividers used to get other
//     frequencies for syncing to other operations.
//
// For a timer frequency of 100,000Hz i.e. divider of 800,
// the Timer alarms would be configured in units of 10uS.
//
// I'm after a sample rate of 10kHz, which requires a 100uS
// period, so I'm going to trigger an alarm every 10 'ticks'.
//
// NB: All ADSR timings are stored in 0.1mS units,
//     so are effectively the number of ticks required
//     for each stage of the ADSR.
//
#define SAMPLE_RATE_KHZ 10  // 10kHz (0.1mS)
#define TIMER_FREQ  100000  // 100kHz (0.01mS)
#define TIMER_RATE  10      // 0.1mS

// Use the ESP32 timer routines
hw_timer_t *timer = NULL;

//--------------------------
//    ADSR Handlng
//--------------------------

// ADSR state machine
enum adsr_t {
  adsrIdle=0,
  adsrTrigger,
  toAttack,  adsrAttack,
  toDelay,   adsrDelay,
  toSustain, adsrSustain,
  toRelease, adsrRelease,
  adsrReset
};

// One envelope per DAC output
//
// NB: Levels in 8.8 fixed point values.
//     Times in 0.1mS units.
//
// Amount to update the level for each state on each 'tick' is given by:
//   Num Steps = Time (S) / Sample Rate (Hz) = Time (mS) / Sample Rate (kHz)
//   Step = (End Level - Start Level) / Num Steps
//
// Use a signed 8.8 value for update per tick as can go up or down.
//
// NB: Must be careful env_l and steps don't wrap around, so
//     use signed 32-bit values to make testing easier.
//
struct adsrEnv_s {
  int32_t  env_l;
  int32_t  steps;
  uint16_t attack_ms;
  uint16_t attack_l;
  uint16_t delay_ms;
  int32_t  sustain_l;
  uint16_t release_ms;
  bool gate;  
  adsr_t state;
} env[NUM_DAC_PINS];

#define ATTACK_LEVEL 255  // Maximum level for attack
#define ATTACK_LEVEL_88 (256*ATTACK_LEVEL) // In 8.8 format
#define ATTACK_LEVEL_MIN 0   // Usually zero unless want a minimum value
#define ATTACK_LEVEL_MIN_88  (256*ATTACK_LEVEL_MIN)  // In 8.8 format

uint8_t nextADSR (unsigned ch) {
  if (ch >= NUM_DAC_PINS) {
    // out of range
    return 0;
  }

  struct adsrEnv_s *e = &env[ch];

  // Update the ADSR state machine
  //
  // NB: I could eliminate the transition stages by
  //     allowing fall-through of states, but I've
  //     opted to keep then as individual states for
  //     clarity.
  //
  //     The downside is that each transition adds an
  //     extra "tick" (0.1mS) to the timing of each
  //     stage of the ADSR.  But as these are probably
  //     coming from pot settings anyway, that is
  //     pretty much just "in the noise" anyway.
  //
  // Also note that removal of the Gate signal will
  // start the released regardless of what state it is in.
  //
  // Retriggering can also happen at any time and the envelope
  // will grow to the ATTACK_LEVEL from whatever level
  // is already set.
  //
  switch (e->state) {
    case adsrIdle:
      // Doing nothing until triggered
      break;

    case adsrTrigger:
    case toAttack:
      // Calculate the step value.
      // As each tick is 0.1mS and all timings
      // for stages are in 0.1mS units, the number
      // of steps per tick is always 1 and the
      // number of steps per stage is the same as the
      // stored time in 0.1mS.
      //
      // Step (8.8 format) = 256 * (End Level - Start Level) / Num Steps
      //
      // NB: Start from whatever the current level is to allow for
      //     retriggering whilst previous envelope is still running...
      //     Still using same steps/timings though.
      e->steps = (ATTACK_LEVEL_88 - ATTACK_LEVEL_MIN_88) / e->attack_ms;
      e->state = adsrAttack;
      break;

    case adsrAttack:
      if (e->gate) {
        if (e->env_l >= ATTACK_LEVEL_88) {
          // Reached top of the envelope.
          e->env_l = ATTACK_LEVEL_88;
          e->steps = 0;
          e->state = toDelay;
        }
      } else {
        // Gate Off - Jump to R stage;
        e->state = toRelease;
      }
      break;

    case toDelay:
      // Calculate next set of steps and move to D phase.
      e->steps = (e->sustain_l - e->env_l) / e->delay_ms;
      e->state = adsrDelay;
      break;

    case adsrDelay:
      if (e->gate) {
        if (e->env_l <= e->sustain_l) {
          // Reached sustain level so hold this level and move to S stage
          e->env_l = e->sustain_l;
          e->steps = 0;
          e->state = toSustain;
        }
      } else {
        // Gate Off - Jump to R stage;
        e->state = toRelease;
      }
      break;

    case toSustain:
       // Nothing extra to do, so move straight on
       e->state = adsrSustain;
    case adsrSustain:
      if (e->gate) {
        // Hold here whilst gate is ON
      } else {
        // Gate Off - Jump to R stage;
        e->state = toRelease;
      }
      break;
 
    case toRelease:
      e->steps = (ATTACK_LEVEL_MIN_88 - e->env_l) / e->release_ms;
      e->state = adsrRelease;
      break;
 
    case adsrRelease:
      // Test for zero
      if (e->env_l <= ATTACK_LEVEL_MIN_88) {
        // Envelope complete
        e->state = adsrReset;
      }
      break;

    case adsrReset:
      e->env_l = 0;
      e->steps = 0;
      e->state = adsrIdle;
      break;

    default:
      e->state = adsrReset;
  }

  // Update the envelope level according to the parameters set above.
  e->env_l = e->env_l + e->steps;
  if (e->env_l < ATTACK_LEVEL_MIN_88) {
    e->env_l = ATTACK_LEVEL_MIN_88;
  }
  if (e->env_l > ATTACK_LEVEL_88) {
    e->env_l = ATTACK_LEVEL_88;
  }
  return e->env_l >> 8;
}

void triggerADSR (unsigned ch) {
  env[ch].state = adsrTrigger;
}

void gateADSR (unsigned ch, bool gateOn) {
  if (gateOn) {
    env[ch].gate = true;
  } else {
    env[ch].gate = false;
  }
}

void setADSR (unsigned ch, unsigned pot, uint16_t potval) {
  if (ch < NUM_DAC_PINS) {
    switch (pot) {
      case 0: // A
        env[ch].attack_ms = 1 + potval * 2; // 0..8191 in 0.1mS units. NB: Cannot be zero!
        break;
      case 1: // D
        env[ch].delay_ms = 1 + potval * 2;
        break;
      case 2: // S
        env[ch].sustain_l = map (potval, 0, 4095, ATTACK_LEVEL_MIN_88, ATTACK_LEVEL_88); // 0..255 in 8.8 form: (val * 256) >> 4
        break;
      case 3: // R
        env[ch].release_ms = 1 + potval * 2;
        break;
      default:
        // Ignore...
        break;
    }
  }
}

void initADSR () {
  for (int i=0; i<NUM_DAC_PINS; i++) {
    env[i].attack_ms = 500; // 50mS
    env[i].attack_l = ATTACK_LEVEL_88;
    env[i].delay_ms = 2000;  // 200mS
    env[i].sustain_l = ATTACK_LEVEL_88 * 2 / 3;
    env[i].release_ms = 5000; // 500mS
    env[i].gate = false;    
    env[i].state = adsrReset;
    env[i].env_l = ATTACK_LEVEL_MIN_88;
  }
}

void dumpADSR (int ch) {
  Serial.print(env[ch].gate);
  Serial.print(":");
  Serial.print(env[ch].state);
  Serial.print("\t");
  Serial.print(env[ch].steps);
  Serial.print("\t[");
  Serial.print(env[ch].env_l);
  Serial.print("]\t ADSR=");
  Serial.print(env[ch].attack_ms);
  Serial.print(",\t");
  Serial.print(env[ch].delay_ms);
  Serial.print(",\t");
  Serial.print(env[ch].sustain_l);
  Serial.print(",\t");
  Serial.print(env[ch].release_ms);
  Serial.print("\n");  
}

//--------------------------
//    Timer ISR
//--------------------------

uint8_t sample[NUM_DAC_PINS];
void ARDUINO_ISR_ATTR timerIsr (void) {
#ifdef TIMING_PIN
  digitalWrite(TIMING_PIN, HIGH);
#endif

  // Write out last value first to remove
  // any jitter from the timing of the ISR.
  for (unsigned i=0; i<NUM_DAC_PINS; i++) {
    dacWrite(dac_pins[i], sample[i]);
  }
  
  // Then calculate the new samples for next time
  for (unsigned i=0; i<NUM_DAC_PINS; i++) {
    sample[i] = nextADSR (i);
  }
  
#ifdef TIMING_PIN
  digitalWrite(TIMING_PIN, LOW);
#endif
}

//--------------------------
//    Setup and Loop
//--------------------------

void setup () {
  Serial.begin(115200);
#ifdef TIMING_PIN
  pinMode(TIMING_PIN, OUTPUT);
  digitalWrite(TIMING_PIN, LOW);
#endif

#ifdef TESTTONE
  tone(TESTTONE, 440);
#endif

  for (int i=0; i<NUM_DAC_PINS; i++) {
    pinMode (trig_pins[i], INPUT);
    lasttrig[i] = LOW;
    pinMode (gate_pins[i], INPUT);
    lastgate[i] = LOW;

    // Start with output at zero
    sample[i] = ATTACK_LEVEL_MIN;
    dacWrite(dac_pins[i], ATTACK_LEVEL_MIN);
    
    // Force a read of the ADCs on power up
    for (int p=0; p<NUM_ADC_PINS; p++) {
      adcval[i][p] = 32768;
    }
  }

  timer = timerBegin(TIMER_FREQ);
  timerAttachInterrupt(timer, &timerIsr);
  timerAlarm(timer, TIMER_RATE, true, 0);
  Serial.print("Timer frequency: ");
  Serial.println(timerGetFrequency(timer));
  
  initADSR();
}

#ifdef TEST
void loop () {
  for (int i=0; i<NUM_DAC_PINS; i++) {
    triggerADSR(i);
    gateADSR(i, true);
  }

  for (int i=0; i<100; i++) {
    Serial.println(env[0].env_l);
    delay(10);
  }

  for (int i=0; i<NUM_DAC_PINS; i++) {
    gateADSR (i, false);
  }

  for (int i=0; i<200; i++) {
    Serial.println(env[0].env_l);
    delay(10);
  }

  delay(5000);
}
#else
void loop () {
#ifdef DUMPADSR
  dumpADSR(0);
#endif

  // Check gate/triggers
  for (unsigned i=0; i<NUM_DAC_PINS; i++) {
    int trigval = digitalRead(trig_pins[i]);
    if ((trigval == HIGH) && (lasttrig[i] == LOW)) {
      triggerADSR(i);
    }
    else {
      // No change
    }
    lasttrig[i] = trigval;

    int gateval = digitalRead(gate_pins[i]);
    if ((gateval == HIGH) && (lastgate[i] == LOW)) {
      // Gate is ON
      gateADSR(i, true);
    }
    else if ((gateval == LOW) && (lastgate[i] == HIGH)) {
      // Gate is OFF
      gateADSR(i, false);
    }
    else {
      // No change
    }
    lastgate[i] = gateval;

    // cycle through each defined potentiometer
    for (unsigned p=0; p<NUM_ADC_PINS; p++) {
        uint16_t algval = analogRead(adc_pins[i][p]);
        if (algval != adcval[i][p]) {
          setADSR(i, p, algval);
        }
        adcval[i][p] = algval;
    }
  }
}
#endif
