# Pi Pico Poly Tone Keypad Tenori-On
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
import machine
import time
import picokeypad as keypad
import ustruct
import PIOBeep

#
# Definitions for the notes played by the grid
#
TEMPO = 240      # beats per minute
midiNotes1 = [
    69,69,69,69,
]
midiNotes2 = [
    67,67,67,67,
]
midiNotes3 = [
    64,64,64,64,
]
midiNotes4 = [
    60,60,60,60,
]

keypad.init()
keypad.set_brightness(1.0)
NUM_NOTES = len(midiNotes1)
NUM_PADS = keypad.get_num_pads()
NUM_STEPS = 4
NUM_ROWS = NUM_PADS/NUM_STEPS

noteGrid = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
last_button_states = 0
time_ms = 0
step = 0
lastnote = 0
lastkey = -1

#
# Definitions for the frequencies the tone() side will respond to
#
lownote = 33   # A1 MIDI note number
lowfreq = 55   # A1 Frequency
hinote  = 105  # A7 Last MIDI note supported
playnote = []
osc = []
oscuse = []
midi2osc = []
numnotes = hinote-lownote+1

def noteOn(x):
    if (x<lownote) or (x>hinote):
        return

    # Find a free oscillator
    for o in range(0, numosc):
        if (oscuse[o] == 0):
            oscuse[o] = x
            osc[o].note_on(midi2osc[x-lownote])
            return

def noteOff(x):
    if (x<lownote) or (x>hinote):
        return

    # Find the playing oscillator and turn it off
    for o in range(0, numosc):
        if (oscuse[o] == x):
            osc[o].note_off()
            oscuse[o] = 0

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
    # Formula for MIDI to frequency conversion from http://newt.phys.unsw.edu.au/jw/notes.html
    midi2osc.append(osc[0].calc_pitch(lowfreq*2**(n/12)))

def lightUp(x):
    keypad.illuminate(x, 0, 50, 50)

def lightOff(x):
    keypad.illuminate(x, 0, 0, 0)

while True:
    # Scan the keypad all the time
    keypad.update()
    button_states = keypad.get_button_states()
    if last_button_states != button_states:
        last_button_states = button_states
        for key in range (NUM_PADS):
            if (button_states & (1<<key)):
                if (noteGrid[key]):
                    noteGrid[key] = 0
                    lightOff(key)
                else:
                    noteGrid[key] = 1
                    lightUp(key)
            keypad.update()

    # Play the sequencer on our time schedule
    newtime_ms = time.ticks_ms()
    if (newtime_ms > time_ms):
        # Time to wake up!
        
        # Calculate the next "tick" millisecond counter from the TEMPO
        time_ms = newtime_ms + 60000/TEMPO
        
        for row in range (NUM_ROWS):
            keystep = step + row*NUM_STEPS
            if (noteGrid[keystep] == 0):
                lightOff(keystep)

        step+=1
        if (step >= NUM_STEPS):
            step = 0
        
        # And turn off any last notes playing
        for note in range (NUM_NOTES):
            noteOff(midiNotes1[note])
            noteOff(midiNotes2[note])
            noteOff(midiNotes3[note])
            noteOff(midiNotes4[note])

        # Now process each "column" step
        for row in range (NUM_ROWS):
            keystep = step + row*NUM_STEPS
            if (noteGrid[keystep] == 0):
                keypad.illuminate(keystep, 2,2,2)
            else:
                lightUp(keystep)

            # Play any notes
            if (noteGrid[keystep] != 0):
                if (row == 0):
                    noteOn(midiNotes1[step])
                elif (row == 1):
                    noteOn(midiNotes2[step])
                elif (row == 2):
                    noteOn(midiNotes3[step])
                elif (row == 3):
                    noteOn(midiNotes4[step])
