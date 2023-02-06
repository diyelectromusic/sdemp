# Simple MIDI Note Balancer
# for Micro Python on the Raspberry Pi Pico
#
# @diyelectromusic
# https://diyelectromusic.wordpress.com/
#
#      MIT License
#      
#      Copyright (c) 2023 diyelectromusic (Kevin)
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

# Basic hardware parameters
UART_BAUD = 31250
HW_UART = 0
PIN_BASE = 6
NUM_UARTS = 8
MIDI_THRU = True
MIDI_LED = 25

# Specify the number of OUTPUT ports to use
NUM_PORTS = 6

# For each received MIDI channel, specify:
#    List of ports for possible output on the channel
#    Level of polyphony per port via number of elements in 'playing' list
MIDIRT = [
    {'ch':1, 'ports':[
        # Three ports, each with 2-note polyphony
        {'port':0, 'playing':[0,0]},
        {'port':1, 'playing':[0,0]},
        {'port':2, 'playing':[0,0]}
        ]
     },
    {'ch':2, 'ports':[
        # One port, with 4-note polyphony
        {'port':3, 'playing':[0,0,0,0]}
        ]
     },
    {'ch':3, 'ports':[
        # Two ports, with single note polyphony
        {'port':4, 'playing':[0]},
        {'port':5, 'playing':[0]}
        ]
     }
    ]

ledpin = Pin(MIDI_LED, machine.Pin.OUT)
hw_uart = UART(HW_UART,UART_BAUD)

# MIDI routing function
def midi2port (cmd, ch, note):
    # NoteOn
    if cmd == 0x90:
        for r in MIDIRT:
            if r['ch'] == ch:
                for p in r['ports']:
                    for n in range(len(p['playing'])):
                        if p['playing'][n] == 0:
                            # There are free playing slots on this port
                            p['playing'][n] = note
                            # Only return first free port
                            return p['port']

    # NoteOff
    elif cmd == 0x80:
        for r in MIDIRT:
            if r['ch'] == ch:
                for p in r['ports']:
                    for n in range(len(p['playing'])):
                        if p['playing'][n] == note:
                            # Found our note, so free the slot
                            p['playing'][n] = 0
                            # Only return first matched port
                            return p['port']

    # Can't route
    return -1

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

# port = PIO port to use (0 to NUM_PORTS-1)
#  cmd = MIDI command (0x8n-0xEn)
#   ch = MIDI channel (0-15)
#   b1 = MIDI data byte 1
#   b2 = MIDI data byte 2 (-1 = NOT USED)
def pio_midi_send(port, cmd, ch, b1, b2):
    if port >= NUM_PORTS or port < 0:
        return

    sm = tx_uarts[port]
    
    # Build the first MIDI byte:
    #   0xCn
    #     C = MIDI command (e.g. 8 for NoteOff)
    #     n = MIDI channel (0 to 15)
    b0 = cmd + ch-1
    sm.put(b0)
    sm.put(b1)
    if (b2 != -1):
        sm.put(b2)
    #print ("Sent ",hex(b0),b1,b2)

# Basic MIDI handling commands
def doMidiNoteOn(ch,cmd,note,vel):
    #print(ch,"\tNote On \t", note, "\t", vel)
    port = midi2port (cmd, ch, note)
    if port != -1:
        ledpin.value(1)
        pio_midi_send(port, cmd, ch, note, vel)

def doMidiNoteOff(ch,cmd,note,vel):
    #print(ch,"\tNote Off\t", note, "\t", vel)
    port = midi2port (cmd, ch, note)
    if port != -1:
        ledpin.value(0)
        pio_midi_send(port, cmd, ch, note, vel)

def doMidiThru(ch,cmd,d1,d2):
    #print(ch,"\tThru\t", hex(cmd>>4), "\t", d1, "\t", d2)
    # Ignore any other MIDI commands
    return

md = SimpleMIDIDecoder.SimpleMIDIDecoder()
md.cbNoteOn (doMidiNoteOn)
md.cbNoteOff (doMidiNoteOff)
md.cbThru (doMidiThru)

count=0
while True:
    if (hw_uart.any()):
        data = hw_uart.read(1)
        md.read(data[0])
        if MIDI_THRU:
            hw_uart.write(data)
        
    count = count + 1
    if count > 1000:
        print (MIDIRT)
        count = 0
