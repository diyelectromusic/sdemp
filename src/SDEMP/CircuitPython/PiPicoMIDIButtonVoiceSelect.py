# Pi Pico MIDI Button Voice Selector
# CircuitPython and USB and UART MIDI Version
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
import board
import digitalio
import usb_midi
import busio
import adafruit_midi
from adafruit_midi.note_off import NoteOff
from adafruit_midi.note_on import NoteOn
from adafruit_midi.polyphonic_key_pressure import PolyphonicKeyPressure
from adafruit_midi.control_change import ControlChange
from adafruit_midi.program_change import ProgramChange
from adafruit_midi.channel_pressure import ChannelPressure
from adafruit_midi.pitch_bend import PitchBend
from adafruit_midi.midi_message import MIDIUnknownEvent

# Need to state where the PC/CC messages from the buttons will go
# Set to 1 to enable PC/CC over this interface
PCCC_USB = 1
PCCC_SER = 1

# NOTE: First "BANKS" buttons are assumed to be, well, banks
BANKS = 8
VOICES = 32

BANKSELMSB = 0
BANKSELLSB = 32
# Define the BANKSEL CC values for each BANK switch.
# Switch is indexed as follows:
#    7, 5, 3, 1
#    8, 6, 4, 2
#
# So need bank select messages to match this order too.
#
#  Format: [MSB, LSB],
ccs = [
    [0, 3], [0, 7], [0, 2], [0, 6], [0, 1], [0, 5], [0, 0], [0, 4]
    ]

# Define Program Change values for each VOICE switch
# Note: This is highly dependant on the order of decoding the switches!
#       (It didn't have to be, but that is a lot more code than just
#        listing the program numbers in switch order!)
#
# Switch physical (wiring) order (32 switches, four banks of 8):
#   07 05 03 01  15 13 11 09  23 21 19 17  31 29 27 25
#   08 06 04 02  16 14 12 10  24 22 20 18  32 30 28 26
#
# So to map (say) the first eight switches on the "top row"
# (across two switch modules) to voice numbers 0 to 7:
#     07=0; 05=1; 03=2; 01=3; 15=4; 13=5; 11=6; 09=7
#
pcs = [
     3, 19,  2, 18,  1, 17,  0, 16,
     7, 23,  6, 22,  5, 21,  4, 20,
    11, 27, 10, 26,  9, 25,  8, 24,
    15, 31, 14, 30, 13, 29, 12, 28
]

uart = busio.UART(board.GP0, board.GP1, baudrate=31250, timeout=0.001)
sermidi = adafruit_midi.MIDI(midi_in=uart, midi_out=uart)
usbmidi = adafruit_midi.MIDI(midi_in=usb_midi.ports[0], midi_out=usb_midi.ports[1])

led = digitalio.DigitalInOut(board.GP25)
led.direction = digitalio.Direction.OUTPUT

def ledOn():
    led.value = True

def ledOff():
    led.value = False

def midiPC(x):
    ledOn()
    if (PCCC_USB):
        usbmidi.send(ProgramChange(x))
    if (PCCC_SER):
        sermidi.send(ProgramChange(x))
    ledOff()

def midiCC(x, y):
    ledOn()
    if (PCCC_USB):
        usbmidi.send(ControlChange(x,y))
    if (PCCC_SER):
        sermidi.send(ControlChange(x,y))
    ledOff()

def buttonPressed(b):
    if (b<BANKS):
        midiCC(BANKSELMSB, ccs[b][0])
        midiCC(BANKSELLSB, ccs[b][1])
    elif (b<(VOICES+BANKS)):
        midiPC(pcs[b-BANKS])

# Details of how to make a keyboard matrix
# http://blog.komar.be/how-to-make-a-keyboard-the-matrix/

rows = []
cols = []
btn = []
lastbtn = []

ledOn()

# Switch OFF will be HIGH (operating in PULL_UP mode)
row_pins = [board.GP16,board.GP17,board.GP18,board.GP19,board.GP20,board.GP21,board.GP26,board.GP27]
numrows = len(row_pins)
for rp in range(0, numrows):
    rows.append(digitalio.DigitalInOut(row_pins[rp]))
    rows[rp].switch_to_input(pull=digitalio.Pull.UP)

# OPEN DRAIN mode means that when HIGH the pin is effectively disabled.
col_pins = [board.GP10,board.GP11,board.GP12,board.GP13,board.GP14]
numcols = len(col_pins)
for cp in range(0, numcols):
    cols.append(digitalio.DigitalInOut(col_pins[cp]))
    cols[cp].switch_to_output(drive_mode = digitalio.DriveMode.OPEN_DRAIN)

# Initialise Columns to HIGH (i.e. disconnected)
for c in range(0, numcols):
    cols[c].value = True
    for r in range(0, numrows):
        # initialise the note list
        btn.append(0)
        lastbtn.append(0)

ledOff()

# Column and button counters
c = 0
b = 0
numbtns = len(btn)

while True:
    # Activate each column in turn by setting it to low
    if (c > numcols):
        # NB: c = numcols means scan button array, not the IO pins...
        c = 0
    if (b >= numbtns):
        c = 0
        b = 0

    if (c < numcols):
        # Scan the IO pins, one at a time
        cols[c].value = False

        # Then scan for buttons pressed on the rows
        # Any pressed buttons will be LOW
        for r in range(0, numrows):
            if (rows[r].value == False):
                btn[c*numrows+r] = 1
                print ("C: ", c, "R: ", r, "idx: ", (c*numrows+r))
            else:
                btn[c*numrows+r] = 0

        # Disable it again once done
        cols[c].value = True
        
        # Move on to next column
        c = c+1

    if (c == numcols):
        # Now see if there are any button transitions to trigger MIDI events
        if (btn[b] == 1 and lastbtn[b] == 0):
            buttonPressed(b)
        if (btn[b] == 0 and lastbtn[b] == 1):
            # Button released
            pass
        lastbtn[b] = btn[b]
        
        # Move on to next button to check
        b = b+1

    # Now handle the MIDI THRU/Routing between USB and serial MIDI
    msg = usbmidi.receive()
    if (isinstance(msg, MIDIUnknownEvent)):
        # Ignore unknown MIDI events
        # This filters out the ActiveSensing from my UMT-ONE for example!
        pass
    elif msg is not None:
        ledOn()
        sermidi.send(msg)
    else:
       # Ignore "None"
       pass

    msg = sermidi.receive()
    if (isinstance(msg, MIDIUnknownEvent)):
        # Ignore unknown MIDI events
        pass
    elif msg is not None:
        ledOn()
        usbmidi.send(msg)
    else:
       # Ignore "None"
       pass

    ledOff()
