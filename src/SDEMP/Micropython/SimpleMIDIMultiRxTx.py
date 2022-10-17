# Simple MIDI Multi-RX-TX Router
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
import utime
import ustruct
import SimpleMIDIDecoder

ledpin = machine.Pin(25, machine.Pin.OUT)

def ledFlash():
    ledpin.value(1)
    utime.sleep_ms(100)
    ledpin.value(0)
    utime.sleep_ms(50)

def ledOn():
    ledpin.value(1)

def ledOff():
    ledpin.value(0)

UART_BAUD = 31250
RX_PIN_BASE = 6
RX_NUM_UARTS = 4
TX_PIN_BASE = 10
TX_NUM_UARTS = 4
HW_NUM_UARTS = 2

# Define the MIDI routing table specifying groups of
#    [MIDI CH, MIDI CMD, source port, destination port]
#
# NB: -1 for CH, CMD or SRC means "any"
#
MIDIRT = [
    [-1, -1, 2, 2],  # Anything on port 2 to port 2
    [-1, -1, 3, 3],  # Anything on port 3 to port 3
    [-1, -1, 4, 4],  # Anything on port 4 to port 4
    [-1, -1, 5, 0],  # Anything on port 5 to port 0
    [-1, -1, 0, 1],  # Anything on port 0 to port 1
    [-1, -1, 0, 5],  # Anything on port 0 to port 5
    [-1, -1, 1, 1],  # Anything on port 1 to port 1
    [-1, -1, 1, 5],  # Anything on port 1 to port 5
    [6, 0x80, 0, 4], # NoteOff on CH 6 on port 0 to port 5
    [6, 0x90, 0, 4], # NoteOn on CH 6 on port 0 to port 5
     ]

def midiRouter(s_ch,s_cmd,s_src):
    d_dst = []
    
    # Return the destination serial ports to use
    # based on the channel, command, and source
    # serial port that received the message.
    #
    # This will return all routes that match,
    # regardless of how specific the route is.
    #
    for r in MIDIRT:
        ch,cmd,src,dst = r
        if (ch == -1) or (s_ch == ch):
            if (cmd == -1) or (s_cmd == cmd):
                if (src == -1) or (s_src == src):
                    print ("[",s_ch,",",s_cmd,",",s_src,"]\tMatched route: [",ch,",",cmd,",",src,",",dst,"] --> ", dst)
                    d_dst.append(dst)
    
    # Return the destinations found, eliminating duplicates
    return set(d_dst)

hw_uarts = []
for i in range(HW_NUM_UARTS):
    t_uart = machine.UART(i,31250)
    hw_uarts.append(t_uart)
    ledFlash()

# PIO code taken from 
# https://github.com/micropython/micropython/blob/master/examples/rp2/pio_uart_rx.py
@rp2.asm_pio(in_shiftdir=rp2.PIO.SHIFT_RIGHT)
def uart_rx():
    # fmt: off
    label("start")
    # Stall until start bit is asserted
    wait(0, pin, 0)
    # Preload bit counter, then delay until halfway through
    # the first data bit (12 cycles incl wait, set).
    set(x, 7)                 [10]
    label("rbitloop")
    # Shift data bit into ISR
    in_(pins, 1)
    # Loop 8 times, each loop iteration is 8 cycles
    jmp(x_dec, "rbitloop")     [6]
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

# PIO code taken from 
# https://github.com/micropython/micropython/blob/master/examples/rp2/pio_uart_tx.py
@rp2.asm_pio(sideset_init=rp2.PIO.OUT_HIGH, out_init=rp2.PIO.OUT_HIGH, out_shiftdir=rp2.PIO.SHIFT_RIGHT)
def uart_tx():
    #; An 8n1 UART transmit program.
    #; OUT pin 0 and side-set pin 0 are both mapped to UART TX pin.

    #; Assert stop bit, or stall with line in idle state
    pull()     .side(1)       [7]
    #; Preload bit counter, assert start bit for 8 clocks
    set(x, 7)  .side(0)       [7]
    #; This loop will run 8 times (8n1 UART)
    label("tbitloop")
    #; Shift 1 bit from OSR to the first OUT pin
    out(pins, 1)
    #; Each loop iteration is 8 cycles.
    jmp(x_dec, "tbitloop")     [6]


# Now we add 4 UART RXs and 4 TXs, on the requested pins at the same BAUD rate
rx_uarts = []
for i in range(RX_NUM_UARTS):
    rsm = rp2.StateMachine(
        i, uart_rx, freq=8 * UART_BAUD, in_base=machine.Pin(RX_PIN_BASE + i), jmp_pin=machine.Pin(RX_PIN_BASE + i)
    )
    rsm.active(1)
    rx_uarts.append(rsm)
    ledFlash()

tx_uarts = []
for i in range(TX_NUM_UARTS):
    tsm = rp2.StateMachine(
        RX_NUM_UARTS+i, uart_tx, freq=8 * UART_BAUD, sideset_base=machine.Pin(TX_PIN_BASE + i), out_base=machine.Pin(TX_PIN_BASE + i)
    )
    tsm.active(1)
    tx_uarts.append(tsm)
    ledFlash()

def pio_midi_send(pio_uart, cmd, ch, b1, b2):
    sm = tx_uarts[pio_uart]
    
    # Build the first MIDI byte:
    #   0xCn
    #     C = MIDI command (e.g. 8 for NoteOff)
    #     n = MIDI channel (0 to 15)
    b0 = cmd + ch-1
    sm.put(b0)
    sm.put(b1)
    sm.put(b2)

def uart_midi_send(uart, cmd, ch, b1, b2):
    if (b2 == -1):
        hw_uarts[uart].write(ustruct.pack("bb",cmd+ch-1,b1))
    else:
        hw_uarts[uart].write(ustruct.pack("bbb",cmd+ch-1,b1,b2))

def midi_send(uart, cmd, ch, b1, b2):
    print ("Sending (",ch,cmd,b1,b2,") to port ", uart)
    if (uart < HW_NUM_UARTS):
        # Use hardware serial port
        uart_midi_send(uart, cmd, ch, b1, b2)
    else:
        # Use PIO serial port
        pio_midi_send(uart-HW_NUM_UARTS, cmd, ch, b1, b2)

# Basic MIDI handling commands.
# These will only be called when a MIDI decoder
# has a complete MIDI message to send.
#
# The src parameter indicates which serial port
# the message was received on, so it will use the
# routing information to work out where to send it
# and pass it on to the right output serial port.
def doMidiNoteOn(ch,cmd,note,vel,src):
    ledOn()
    dst = midiRouter(ch, cmd, src)
    if dst:
        for d in dst:
            midi_send(d, cmd, ch, note, vel)

def doMidiNoteOff(ch,cmd,note,vel,src):
    ledOff()
    dst = midiRouter(ch, cmd, src)
    if dst:
        for d in dst:
            midi_send(d, cmd, ch, note, vel)

def doMidiThru(ch,cmd,d1,d2,src):
    dst = midiRouter(ch, cmd, src)
    if dst:
        for d in dst:
            midi_send(d, cmd, ch, d1, d2)

md = []
for i in range(HW_NUM_UARTS+RX_NUM_UARTS):
    # Set up one MIDI decoder per hardware UARTs
    md_t = SimpleMIDIDecoder.SimpleMIDIDecoder(i)
    md_t.cbNoteOn (doMidiNoteOn)
    md_t.cbNoteOff (doMidiNoteOff)
    md_t.cbThru (doMidiThru)
    md.append(md_t)

while True:
    for i in range(HW_NUM_UARTS):
        if (hw_uarts[i].any()):
            md[i].read(hw_uarts[i].read(1)[0])

    for i in range(RX_NUM_UARTS):
        if (rx_uarts[i].rx_fifo()):
            md[HW_NUM_UARTS+i].read(rx_uarts[i].get() >> 24)
