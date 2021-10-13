#ifndef _CCMIDI_H
#define _CCMIDI_H

// List of MIDI CC controllers to use for the 8 switches of the JC Pro Macro 2.
//
// Any of the defined CC controllers can be used, not just those listed here.
// For the full list see:
// https://www.midi.org/specifications-old/item/table-3-control-change-messages-data-bytes-2
#define MODWHEEL   1
#define BREATH     2
#define FOOTCTRL   4
#define VOLUME     7
#define BALANCE    8
#define PAN        10
#define EXPRESSION 11
#define EFFECT1    12
#define EFFECT2    13
#define GP1        16
#define GP2        17
#define GP3        18
#define GP4        19
#define SUSTAIN    64
#define PORTAMENTO 65
#define SOSTENUTO  66
#define SOFTPEDAL  67
#define LEGATO     68
#define SC1        70
#define SC2        71
#define SC3        72
#define SC4        73
#define SC5        74
#define SC6        75
#define SC7        76
#define SC8        77
#define SC9        78
#define SC10       79
#define GP5        80
#define GP6        81
#define GP7        82
#define GP8        83
#define RESETALL   121
#define NOTESOFF   123

// This is a special value used to indicate the button/encoder
// should be used for a MIDI Program Change command not a CC command.
#define PROGRAM    255

// Format:
//   MIDI CC number, Minimum Value, Maximum Value
//
//   Set the Min to 0 and Max to CCSW if it is an
//   ON/OFF value rather than a continuous controller.
//   Switches will be mapped to the JCPro switches directly
//   where pressing the switch turns the value ON and pressing
//   it again turns if OFF.
//   Example:
//       { SUSTAIN, 0, CCSW}   // Sustain is ON or OFF
//
//   Set the Min to CCSW for a single value "one-shot"
//   controller.  Set the Max to the value to sent.
//   Example:
//       { NOTESOFF, CCSW, 0}  // Reset all Notes sends just 0
//
//   Otherwise controllers can have the values 0-127 and are
//   selected using the switches but adjusted using the rotary
//   encoder.
//
//   14-bit value are not currently supported, so only
//   controller MSB values (i.e. 7-bit) are assumed.
//
//   Switch Map:
//     __        +--+ +--+
//    /  \       | 6| | 7|
//    \__/       +--+ +--+
//               | 5| | 8|
//   +--+  +--+  +--+ +--+
//   | 2|  | 3|  | 4| | 9|
//   +--+  +--+  +--+ +--+
//
#define NUM_MIDICC 8
#define MCC  0
#define MMIN 1
#define MMAX 2

#define CCSW 255

// The values to use for ON/OFF controllers
#define MIDICC_ON   64
#define MIDICC_OFF  0

#if 0  // This is a sample functional set of controls
const uint8_t midiCC[NUM_MIDICC][3] = {
{NOTESOFF,   CCSW, 0},  // SW 2
{SUSTAIN,    0, CCSW},  // SW 3
{EXPRESSION, 0, 127},   // SW 4
{MODWHEEL,   0, 127},   // SW 5
{PROGRAM,    0, 127},   // SW 6
{VOLUME,     0, 127},   // SW 7
{BALANCE,    0, 127},   // SW 8
{BREATH,     0, 127},   // SW 9
};
#else  // This is a generic set of controllers
const uint8_t midiCC[NUM_MIDICC][3] = {
{GP1,  0, 127},  // SW 2
{GP2,  0, 127},  // SW 3
{GP3,  0, 127},  // SW 4
{GP5,  0, CCSW}, // SW 5
{GP6,  0, CCSW}, // SW 6
{GP7,  0, CCSW}, // SW 7
{GP8,  0, CCSW}, // SW 8
{GP4,  0, 127},  // SW 9  
};
#endif


#endif // _MIDICC_H
