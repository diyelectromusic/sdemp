# Pi Pico MIDI Matrix Decode - Part 3
#
# @diyelectromusic
# https://diyelectromusic.wordpress.com/2021/02/07/pi-pico-midi-matrix-decode-part-3/
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
from machine import Pin
from machine import UART
import utime
import ustruct

led = Pin(25, Pin.OUT)
uart = UART(0,31250)

# Details of how to make a keyboard matrix
# http://blog.komar.be/how-to-make-a-keyboard-the-matrix/

firstnote = 48 # C4
rows = []
cols = []
playnote = []
lastnote = []

def noteOn(x):
    led.value(1)
    uart.write(ustruct.pack("bbb",0x90,x,127))

def noteOff(x):
    uart.write(ustruct.pack("bbb",0x80,x,0))
    led.value(0)

# Switch OFF will be HIGH (operating in PULL_UP mode)
row_pins = [3,2,5,4,6,8,7,10,9,12,11,13]
numrows = len(row_pins)
for rp in range(0, numrows):
    rows.append(Pin(row_pins[rp], Pin.IN, Pin.PULL_UP))
    print (rows[rp])

# OPEN DRAIN mode means that when HIGH the pin is effectively disabled.
# According to the RP2 MicroPython code this is simulated on the RP2 chip.
# See https://github.com/micropython/micropython/blob/master/ports/rp2/machine_pin.c
# WARNING: At time of writing, this isn't in the MP release!
#          For now, just use standard Pin.OUT and set the unused pins to
#          Pin.IN when not being used... (which is sort out how the emulated
#          OPEN_DRAIN works anyway).
#
col_pins = [28, 27, 26, 22]
numcols = len(col_pins)
for cp in range(0, numcols):
    cols.append(Pin(col_pins[cp], Pin.IN))
    print (cols[cp])

# Initialise Columns to HIGH (i.e. disconnected)
for c in range(0, numcols):
    cols[c].value(True)
    for r in range(0, numrows):
        # initialise the note list
        playnote.append(0)
        lastnote.append(0)

while True:
    # Activate each column in turn by setting it to low
    for c in range(0, numcols):
        cols[c].init(mode=Pin.OUT) # Emulating OPEN_DRAIN
        cols[c].value(False)

        # Then scan for buttons pressed on the rows
        # Any pressed buttons will be LOW
        for r in range(0, numrows):
            if (rows[r].value() == False):
                playnote[c*numrows+r] = 1
            else:
                playnote[c*numrows+r] = 0

        # Disable it again once done
        cols[c].init(mode=Pin.IN) # Emulating OPEN_DRAIN
#        cols[c].value(True)

    print (lastnote)
    print (playnote)

    # Now see if there are any off->on transitions to trigger MIDI on
    # or on->off to trigger MIDI off
    for n in range(0, len(playnote)):
        if (playnote[n] == 1 and lastnote[n] == 0):
            noteOn(firstnote+n)
        if (playnote[n] == 0 and lastnote[n] == 1):
            noteOff(firstnote+n)
        lastnote[n] = playnote[n]

