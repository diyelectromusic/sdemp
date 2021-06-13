# Simple MIDI Channel Router
# for Micro Python on the Raspberry Pi Pico
#
# @diyelectromusic
# https://diyelectromusic.wordpress.com/
#
#      MIT License
#      
#      Copyright (c) 2020 diyelectromusic (Kevin)
#      
#      Permission is hereby granted, free of charge, to any person obtaining a copy of
#      this software and associated documentation files (the "Software"), to deal in
#      the Software without restriction, including without limitation the rights to
#      use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
#      the Software, and to permit persons to whom the Software is furnished to do so,
#      subject to the following conditions:
#      
#      The above copyright notice and this permission notice shall be included in all
#      copies or substantial portions of the Software.
#      
#      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
#      FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
#      COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHERIN
#      AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
#      WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# This needs the SimpleMIDIDecoder.py module from @diyelectromusic too.
#
from machine import Pin, UART
from rp2 import PIO, StateMachine, asm_pio
import SimpleMIDIDecoder

UART_BAUD = 31250
PIN_BASE = 6
NUM_UARTS = 8
MIDI_CH_BASE = 1

rx_uart = UART(0,31250)

@asm_pio(sideset_init=PIO.OUT_HIGH, out_init=PIO.OUT_HIGH, out_shiftdir=PIO.SHIFT_RIGHT)
def uart_tx():
    #; An 8n1 UART transmit program.
    #; OUT pin 0 and side-set pin 0 are both mapped to UART TX pin.

    #; Assert stop bit, or stall with line in idle state
    pull()     .side(1)       [7]
    #; Preload bit counter, assert start bit for 8 clocks
    set(x, 7)  .side(0)       [7]
    #; This loop will run 8 times (8n1 UART)
    label("bitloop")
    #; Shift 1 bit from OSR to the first OUT pin
    out(pins, 1)
    #; Each loop iteration is 8 cycles.
    jmp(x_dec, "bitloop")     [6]

# Now we add 8 UART TXs, on the requested consecutive pins at the same BAUD rate
tx_uarts = []
for i in range(NUM_UARTS):
    sm = StateMachine(
        i, uart_tx, freq=8 * UART_BAUD, sideset_base=Pin(PIN_BASE + i), out_base=Pin(PIN_BASE + i)
    )
    sm.active(1)
    tx_uarts.append(sm)

def pio_midi_send(cmd, ch, b1, b2):
    # UARTS are mapped onto consecutive MIDI channels
    # starting from MIDI_CH_BASE
    # NB: Channels are 1 to 16, but need a UART reference 0 to 7
    if ch < MIDI_CH_BASE:
        return
    
    if (ch > MIDI_CH_BASE+7):
        return

    midiuart = ch - MIDI_CH_BASE
    sm = tx_uarts[midiuart]
    
    # Build the first MIDI byte:
    #   0xCn
    #     C = MIDI command (e.g. 8 for NoteOff)
    #     n = MIDI channel (0 to 15)
    b0 = cmd + ch-1
    sm.put(b0)
    sm.put(b1)
    sm.put(b2)
    #print ("Sent ",hex(b0),b1,b2)

# Basic MIDI handling commands
def doMidiNoteOn(ch,cmd,note,vel):
    #print(ch,"\tNote On \t", note, "\t", vel)
    pio_midi_send(cmd, ch, note, vel)

def doMidiNoteOff(ch,cmd,note,vel):
    #print(ch,"\tNote Off\t", note, "\t", vel)
    pio_midi_send(cmd, ch, note, vel)

def doMidiThru(ch,cmd,d1,d2):
    if (d2 == -1):
        #print(ch,"\tThru\t", hex(cmd>>4), "\t", d1)
        pio_midi_send(cmd, ch, d1, 0)
    else:
        #print(ch,"\tThru\t", hex(cmd>>4), "\t", d1, "\t", d2)
        pio_midi_send(cmd, ch, d1, d2)

md = SimpleMIDIDecoder.SimpleMIDIDecoder()
md.cbNoteOn (doMidiNoteOn)
md.cbNoteOff (doMidiNoteOff)
md.cbThru (doMidiThru)

while True:
    if (rx_uart.any()):
        md.read(rx_uart.read(1)[0])
