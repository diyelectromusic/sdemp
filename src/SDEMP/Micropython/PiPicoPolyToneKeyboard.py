# Pi Pico Polytone Keyboard
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
from machine import Pin
import utime
import ustruct
import PIOBeep

# Details of how to make a keyboard matrix
# http://blog.komar.be/how-to-make-a-keyboard-the-matrix/

firstnote = 48     # C3 MIDI note number
firstfreq = 130.81 # C3 Frequency
rows = []
cols = []
playnote = []
lastnote = []
osc = []
oscuse = []
midi2osc = []

def noteOn(x):
    # Find a free oscillator
    for o in range(0, numosc):
        if (oscuse[o] == 0):
            oscuse[o] = x
            osc[o].note_on(midi2osc[x-firstnote])
            print (o, ": Note On:  ", x, " (", midi2osc[x-firstnote], ")")
            return

def noteOff(x):
    # Find the playing oscillator and turn it off
    for o in range(0, numosc):
        if (oscuse[o] == x):
            osc[o].note_off()
            oscuse[o] = 0    
            print (o, ": Note Off: ", x)

# Initialise the oscillators...
osc_pins = [14,15,16,17,18,19,20,21]
numosc = len(osc_pins)
for o in range(0, numosc):
    osc.append(PIOBeep.PIOBeep(o,osc_pins[o]))
    oscuse.append(0)

# Switch OFF will be HIGH (operating in PULL_UP mode)
row_pins = [3,2,5,4,6,8,7,10,9,12,11,13]
numrows = len(row_pins)
for rp in range(0, numrows):
    rows.append(Pin(row_pins[rp], Pin.IN, Pin.PULL_UP))

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

# Initialise Columns to HIGH (i.e. disconnected)
for c in range(0, numcols):
    cols[c].value(True)
    for r in range(0, numrows):
        # initialise the note list
        playnote.append(0)
        lastnote.append(0)
        # and pre-calculate the frequency values to use for the oscillators
        # Formula for MIDI to frequency conversion from http://newt.phys.unsw.edu.au/jw/notes.html
        midi2osc.append(osc[0].calc_pitch(firstfreq*2**((c*numrows+r)/12)))

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

    # Now see if there are any off->on transitions to trigger MIDI on
    # or on->off to trigger MIDI off
    for n in range(0, len(playnote)):
        if (playnote[n] == 1 and lastnote[n] == 0):
            noteOn(firstnote+n)
        if (playnote[n] == 0 and lastnote[n] == 1):
            noteOff(firstnote+n)
        lastnote[n] = playnote[n]
    
