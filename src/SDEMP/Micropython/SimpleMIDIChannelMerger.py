# Simple MIDI Channel Merger
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
import machine
import rp2
import ustruct
import SimpleMIDIDecoder

UART_BAUD = 31250
PIN_BASE = 6
NUM_UARTS = 8

tx_uart = machine.UART(0,31250)
pin = machine.Pin(25, machine.Pin.OUT)

# PIO code taken from 
# https://github.com/micropython/micropython/blob/master/examples/rp2/pio_uart_rx.py
@rp2.asm_pio(
    in_shiftdir=rp2.PIO.SHIFT_RIGHT,
)
def uart_rx():
    # fmt: off
    label("start")
    # Stall until start bit is asserted
    wait(0, pin, 0)
    # Preload bit counter, then delay until halfway through
    # the first data bit (12 cycles incl wait, set).
    set(x, 7)                 [10]
    label("bitloop")
    # Shift data bit into ISR
    in_(pins, 1)
    # Loop 8 times, each loop iteration is 8 cycles
    jmp(x_dec, "bitloop")     [6]
    # Check stop bit (should be high)
    jmp(pin, "good_stop")
    # Either a framing error or a break. Set a sticky flag
    # and wait for line to return to idle state.
#    irq(block, 4)  - Ignore signalling the break
    wait(1, pin, 0)
    # Don't push data if we didn't see good framing.
    jmp("start")
    # No delay before returning to start; a little slack is
    # important in case the TX clock is slightly too fast.
    label("good_stop")
    push(block)
    # fmt: on

# Now we add 8 UART RXs, on the requested consecutive pins at the same BAUD rate
rx_uarts = []
for i in range(NUM_UARTS):
    sm = rp2.StateMachine(
        i, uart_rx, freq=8 * UART_BAUD, in_base=machine.Pin(PIN_BASE + i), jmp_pin=machine.Pin(PIN_BASE + i)
    )
#    sm.irq(handler) - We're ignoring any "break" indications from the state machine
    sm.active(1)
    rx_uarts.append(sm)

def midi_send(cmd, ch, b1, b2):
    if (b2 == -1):
        tx_uart.write(ustruct.pack("bb",cmd+ch-1,b1))
    else:
        tx_uart.write(ustruct.pack("bbb",cmd+ch-1,b1,b2))

# Basic MIDI handling commands.
# These will only be called when a MIDI decoder
# has a complete MIDI message to send.
def doMidiNoteOn(ch,cmd,note,vel):
    #print(ch,"\tNote On \t", note, "\t", vel)
    pin.value(1)
    midi_send(cmd, ch, note, vel)

def doMidiNoteOff(ch,cmd,note,vel):
    #print(ch,"\tNote Off\t", note, "\t", vel)
    pin.value(0)
    midi_send(cmd, ch, note, vel)

def doMidiThru(ch,cmd,d1,d2):
    midi_send(cmd, ch, d1, d2)

md = []
for i in range(NUM_UARTS):
    # Set up one MIDI decoder per UART
    md_t = SimpleMIDIDecoder.SimpleMIDIDecoder()
    md_t.cbNoteOn (doMidiNoteOn)
    md_t.cbNoteOff (doMidiNoteOff)
    md_t.cbThru (doMidiThru)
    md.append(md_t)

while True:
    for i in range(NUM_UARTS):
        if (rx_uarts[i].rx_fifo()):
            md[i].read(rx_uarts[i].get() >> 24)
