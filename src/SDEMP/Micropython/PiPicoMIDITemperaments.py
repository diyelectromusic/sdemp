# Pi Pico MIDI Temperaments
#
# @diyelectromusic
# https://diyelectromusic.wordpress.com/
#
#      MIT License
#      
#      Copyright (c) 2022 diyelectromusic (Kevin)
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
import utime
import ustruct
import PIOBeep
import SimpleMIDIDecoder

# Serial port handling for MIDI
pin = machine.Pin(25, machine.Pin.OUT)
uart = machine.UART(0,31250)

mode = 0
button1 = machine.Pin(6, machine.Pin.IN, machine.Pin.PULL_UP)
button2 = machine.Pin(7, machine.Pin.IN, machine.Pin.PULL_UP)
btn1val = True
btn2val = True

lownote = 33   # A1 MIDI note number
lowfreq = 55   # A1 Frequency
hinote  = 105  # A7 Last MIDI note supported
playnote = []
lastnote = []
osc = []
oscuse = []
midi2tet = []
midi2jst = []
numnotes = hinote-lownote+1

# Uses ratios for Just Tunings from here: https://en.wikipedia.org/wiki/Music_and_mathematics
justratios = [1, 16/15, 9/8, 6/5,5/4, 4/3, 45/32, 3/2, 8/5, 5/3, 9/5, 15/8]

def noteOn(x):
    if (x<lownote) or (x>hinote):
        return

    # Find a free oscillator
    for o in range(0, numosc):
        if (oscuse[o] == 0):
            oscuse[o] = x
            if (mode):
                freq = midi2jst[x-lownote]
            else:
                freq = midi2tet[x-lownote]
            osc[o].note_on(freq)
            print (o, ": Note On:  ", x, " (", freq, ")")
            return

def noteOff(x):
    if (x<lownote) or (x>hinote):
        return

    # Find the playing oscillator and turn it off
    for o in range(0, numosc):
        if (oscuse[o] == x):
            osc[o].note_off()
            oscuse[o] = 0    
            print (o, ": Note Off: ", x)

# Initialise the oscillators...
# Note: This uses pins that don't clash with the Pimoroni Keypad or Audio Packs
osc_pins = [9,12,13,14,15,16,20,21]
numosc = len(osc_pins)
for o in range(0, numosc):
    osc.append(PIOBeep.PIOBeep(o,osc_pins[o]))
    oscuse.append(0)

# Pre-calculate the frequency values to use for the oscillators
for n in range(0, numnotes):
    # initialise the note list
    playnote.append(0)
    lastnote.append(0)
    # Formula for MIDI to frequency conversion from http://newt.phys.unsw.edu.au/jw/notes.html
    midi2tet.append(osc[0].calc_pitch(lowfreq*2**(n/12)))
    # "Just tuning" uses predefined ratios from a base note
    # The base note doubles on each octave.
    base = lowfreq * 2 ** (int (n/12))
    midi2jst.append(osc[0].calc_pitch(base * justratios[n % 12]))
    #print (int (n/12), "\t", base, "\t", n % 12, "\t", justratios[n % 12]) 

for n in range(0, numnotes):
    print (n, "\t", n+lownote, "\t", midi2tet[n], "\t", midi2jst[n], "\t", midi2tet[n]-midi2jst[n])


# Basic MIDI handling commands.
# These will only be called when a MIDI decoder
# has a complete MIDI message to send.
def doMidiNoteOn(ch,cmd,note,vel):
    if (vel == 0):
        noteOff(note)
        return
    
    pin.value(1)
    noteOn(note)

def doMidiNoteOff(ch,cmd,note,vel):
    pin.value(0)
    noteOff(note)

md = SimpleMIDIDecoder.SimpleMIDIDecoder()
md.cbNoteOn (doMidiNoteOn)
md.cbNoteOff (doMidiNoteOff)

while True:
    # Now check for MIDI messages too
    if (uart.any()):
        md.read(uart.read(1)[0])

    # Check the buttons to see what mode we're in
    btn1 = button1.value()
    if (btn1val == True) and (btn1 == False):
        print ("Even Temperament")
        mode = 0
    btn1val = btn1

    btn2 = button2.value()
    if (btn2val == True) and (btn2 == False):
        print ("Just Temperament")
        mode = 1
    btn2val = btn2
