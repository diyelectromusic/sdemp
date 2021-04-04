# Pi Pico Polytone Keypad Sequencer - Part 2
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
# Definitions for the IO connections on the "tone pack"
#
button1 = machine.Pin(6, machine.Pin.IN, machine.Pin.PULL_UP)
button2 = machine.Pin(7, machine.Pin.IN, machine.Pin.PULL_UP)
pot1 = machine.ADC (machine.Pin(26))
pot2 = machine.ADC (machine.Pin(27))
pot3 = machine.ADC (machine.Pin(28))
btn1val = True
btn2val = True

#
# Definitions for the notes played by the grid
#
TEMPO = 240      # beats per minute
midiNotes1 = [
    0,   # 0 means "don't play" so ignore the first value
#    60,61,62,63, 64,65,66,67, 68,69,70,71, 72,73,74,75 # Normal chromatic scale
    60,62,64,67,69,72,74,76,79,81,84 # Pentatonic
]
midiNotes2 = [
    0,   # Dummy entry
    64,67,69,72,74,76,79,81,84,86,88 # Pentatonic
]
midiNotes3 = [
    0,   # Dummy entry
    67,69,72,74,76,79,81,84,86,88,91 # Pentatonic
]

keypad.init()
keypad.set_brightness(1.0)
NUM_NOTES = len(midiNotes1)
NUM_PADS = keypad.get_num_pads()

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

def chordOn(x):
    noteOn(midiNotes1[x])
    noteOn(midiNotes2[x])
    noteOn(midiNotes3[x])

def chordOff(x):
    noteOff(midiNotes1[x])
    noteOff(midiNotes2[x])
    noteOff(midiNotes3[x])

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

#
# Functions for update the IO and acting on the results
#
def ioUpdate():
    global TEMPO, btn1val
    # Read the tempo from the pot. Need to scale it to
    # a value in range tens to hundres.  Read values seem to be
    # in range 400-65535.
    #
    TEMPO = (pot3.read_u16()>>6)+30;
    if TEMPO > 800:
        TEMPO = 800
    if TEMPO < 60:
        TEMPO = 60
    
    # If the button is pressed, then reset the grid
    btn1 = button1.value()
    if (btn1val == True) and (btn1 == False):
        for key in range (NUM_PADS):
            chordOff(noteGrid[key])
            noteGrid[key] = 0
    btn1val = btn1

#
# Now for the code to manage the colours and the keypad
#

# Based on code from https://github.com/sandyjmacdonald
def colourwheel(pos):
    if pos <= 0:
        return (0,0,0)
    if pos >= 250:
        return (255,255,255)
    if pos < 85:
        return (255 - pos * 3, pos * 3, 0)
    if pos < 170:
        pos -= 85
        return (0, 255 - pos * 3, pos * 3)
    pos -= 170
    return (pos * 3, 0, 255 - pos * 3)

def lightUp(x):
    rgb = colourwheel(noteGrid[x]*16)
    keypad.illuminate(x, rgb[0], rgb[1], rgb[2])

while True:
    # Scan the io
    ioUpdate();

    # Scan the keypad all the time
    keypad.update()
    button_states = keypad.get_button_states()
    if last_button_states != button_states:
        last_button_states = button_states
        for key in range (NUM_PADS):
            if (button_states & (1<<key)):
                lastkey = key
                noteGrid[key]+=1
                if (noteGrid[key] >= NUM_NOTES):
                    noteGrid[key] = 0
                chordOn(noteGrid[key])
                lightUp(key)
            keypad.update()

    # Play the sequencer on our time schedule
    newtime_ms = time.ticks_ms()
    if (newtime_ms > time_ms):
        # Time to wake up!
        
        # Calculate the next "tick" millisecond counter from the TEMPO
        time_ms = newtime_ms + 60000/TEMPO

        # Turn off the last step unless the key has just been pressed,
        # in which case leave it on for one more step...
        if (step != lastkey):
            keypad.illuminate(step,0,0,0)
        lastkey = -1

        step+=1
        if (step >= NUM_PADS):
            step = 0
        
        # And turn off any last notes playing
        for note in range (NUM_NOTES):
            chordOff(note)

        # Illuminate the new step
        if (noteGrid[step] == 0):
            keypad.illuminate(step, 2,2,2)
        else:
            lightUp(step)

        # Play the next note
        if (noteGrid[step] != 0):
            chordOn(noteGrid[step])
            lastnote = noteGrid[step]
