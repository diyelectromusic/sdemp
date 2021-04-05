# Pi Pico Phone Keypad Decoder
# CircuitPython and USB MIDI Version
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
import board
import digitalio
import usb_midi
import adafruit_midi
from adafruit_midi.note_off import NoteOff
from adafruit_midi.note_on import NoteOn
from adafruit_midi.program_change import ProgramChange

led = digitalio.DigitalInOut(board.GP25)
led.direction = digitalio.Direction.OUTPUT
usb_midi = adafruit_midi.MIDI(
    midi_out=usb_midi.ports[1],
    out_channel=0
    )

def ledOn():
    led.value = True

def ledOff():
    led.value = False

# General MIDI program voice list
# Note: voicesets and voices are numbered from 0
#       which is converted into a number from 0 to 127
#       for the program change message:
#          program change msg byte = voiceset * 8 + voice
#
#       In all documentation however program numbers
#       are listed as 1 to 128, so "looked up" instrument
#       name = (voiceset * 8 + voice) + 1
#
# (this list is largely for reference - it isn't used here yet)
#
gmvoiceset = [
    "Piano",
    "Chromatic Percussion",
    "Organ",
    "Guitar",
    "Bass",
    "Strings",
    "Ensemble",
    "Brass",
    "Reed",
    "Pipe",
    "Synth Lead",
    "Synth Pad",
    "Synth Effects",
    "Ethnic",
    "Percussive",
    "Sound Effects"
    ]


# Details of how to make a keyboard matrix
# http://blog.komar.be/how-to-make-a-keyboard-the-matrix/

rows = []
cols = []
playnote = []
lastnote = []

# This is the specific keypad mappings of columns and rows
# for the Doro x20 "Retro" phone I have.
#
notelist = [
    "S", "1", "2", "3",
    " ", " ", " ", "M",
    "8", "9", "0", "*",
    "4", "5", "6", "7",
    "#", "F", " ", " ",
    "R", " ", "U", "D"
    ]

def midinote(x):
    return {
        "1": 36,
        "2": 37,
        "3": 38,
        "4": 39,
        "5": 40,
        "6": 41,
        "7": 42,
        "8": 43,
        "9": 44,
        "*": 45,
        "0": 46,
        "#": 47
        }.get(x,0)

octave = 2
voiceset = 0
voice = 0

def midiNoteOn(x):
    ledOn()
    usb_midi.send(NoteOn(x+octave*12,127))
def midiNoteOff(x):
    usb_midi.send(NoteOff(x+octave*12,127))
    ledOff()
    
def midiProgramChange():
    global voiceset, voice
    program = voiceset*8 + voice
    usb_midi.send(ProgramChange(program))

def keypadOn(x):
    global octave, voiceset, voice
    note = midinote(notelist[x])
    if (note != 0):
        midiNoteOn(note)
    else:
        # handle MIDI command
        cmd = notelist[x]
        if (cmd == "U"):
            #change octave up
            octave += 1
            if (octave > 4):
                octave = 4
        elif (cmd == "D"):
            #change octave down
            octave -= 1
            if (octave < 0):
                octave = 0
        elif (cmd == "R"):
            voice += 1
            if (voice > 7):
                voice = 0
            midiProgramChange ()
        elif (cmd == "F"):
            voice -= 1
            if (voice < 0):
                voice = 7
            midiProgramChange ()
        elif (cmd == "M"):
            voiceset -= 1
            voice = 0
            if (voiceset < 0):
                voiceset = 15
            midiProgramChange ()
        elif (cmd == "S"):
            voiceset += 1
            voice = 0
            if (voiceset > 15):
                voiceset = 0
            midiProgramChange ()
            
def keypadOff(x):
    note = midinote(notelist[x])
    if (note != 0):
        midiNoteOff(note)
    
ledOn()

# Switch OFF will be HIGH (operating in PULL_UP mode)
row_pins = [board.GP20,board.GP21,board.GP22,board.GP26,board.GP27,board.GP28]
numrows = len(row_pins)
for rp in range(0, numrows):
    rows.append(digitalio.DigitalInOut(row_pins[rp]))
    rows[rp].switch_to_input(pull=digitalio.Pull.UP)

# OPEN DRAIN mode means that when HIGH the pin is effectively disabled.
col_pins = [board.GP11,board.GP10,board.GP9,board.GP8]
numcols = len(col_pins)
for cp in range(0, numcols):
    cols.append(digitalio.DigitalInOut(col_pins[cp]))
    cols[cp].switch_to_output(drive_mode = digitalio.DriveMode.OPEN_DRAIN)

# Initialise Columns to HIGH (i.e. disconnected)
for c in range(0, numcols):
    cols[c].value = True
    for r in range(0, numrows):
        # initialise the note list
        playnote.append(0)
        lastnote.append(0)

ledOff()

while True:
    # Activate each column in turn by setting it to low
    for c in range(0, numcols):
        cols[c].value = False

        # Then scan for buttons pressed on the rows
        # Any pressed buttons will be LOW
        for r in range(0, numrows):
            if (rows[r].value == False):
                playnote[r*numcols+c] = 1
            else:
                playnote[r*numcols+c] = 0

        # Disable it again once done
        cols[c].value = True

    # Now see if there are any off->on transitions to trigger MIDI on
    # or on->off to trigger MIDI off
    for n in range(0, len(playnote)):
        if (playnote[n] == 1 and lastnote[n] == 0):
            keypadOn(n)
        if (playnote[n] == 0 and lastnote[n] == 1):
            keypadOff(n)
        lastnote[n] = playnote[n]

