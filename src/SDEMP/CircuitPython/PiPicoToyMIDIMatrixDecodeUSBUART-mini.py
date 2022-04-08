# Pi Pico USB UART MIDI Matrix Decode
# CircuitPython and USB and UART MIDI Version
# with optional sound output via PWM audio out
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
import array
import math
import time
import board
import digitalio
import usb_midi
import busio
import audiopwmio
import audiocore
import adafruit_midi
from adafruit_midi.midi_message import note_parser
from adafruit_midi.note_off import NoteOff
from adafruit_midi.note_on import NoteOn

# 0 = Off; 1 = Saw; 2 = Sine; 3 = Square
sample_wave = 0
octave = 1

uart = busio.UART(board.GP0, board.GP1, baudrate=31250)
dac = audiopwmio.PWMAudioOut(board.GP2)

wavsw = digitalio.DigitalInOut(board.GP3)
wavsw.direction = digitalio.Direction.INPUT
wavsw.pull = digitalio.Pull.UP

octsw = digitalio.DigitalInOut(board.GP4)
octsw.direction = digitalio.Direction.INPUT
octsw.pull = digitalio.Pull.UP

led = digitalio.DigitalInOut(board.GP25)
led.direction = digitalio.Direction.OUTPUT
usb_midi = adafruit_midi.MIDI(
    midi_out=usb_midi.ports[1],
    out_channel=0
    )

# 440Hz is the standard frequency for A4 (A above middle C)
# MIDI defines middle C as 60
A4refhz = 440
midi_note_C4 = note_parser("C4")
midi_note_A4 = note_parser("A4")
twopi = 2 * math.pi

# Defining 64 samples per wave cycle
sample_len = 256
sample_rate = A4refhz * sample_len
sample_bias = 32768  # "NULL" value for the DC bias

def ledOn():
    led.value = True

def ledOff():
    led.value = False

# Details of how to make a keyboard matrix
# http://blog.komar.be/how-to-make-a-keyboard-the-matrix/

# Octave 0=G2 (43) through to 3=G5 (79)
firstnote = 43 # G2
rows = []
cols = []
playnote = []
lastnote = []

ledOn()

# Switch OFF will be HIGH (operating in PULL_UP mode)
row_pins = [board.GP14,board.GP9,board.GP10,board.GP8]
numrows = len(row_pins)
for rp in range(0, numrows):
    rows.append(digitalio.DigitalInOut(row_pins[rp]))
    rows[rp].switch_to_input(pull=digitalio.Pull.UP)

# OPEN DRAIN mode means that when HIGH the pin is effectively disabled.
col_pins = [board.GP12,board.GP13,board.GP11,board.GP6,board.GP7,board.GP5]
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

# A sawtooth function like math.sin(angle)
# 0 returns 1.0, pi returns 0.0, 2*pi returns -1.0
def sawtooth(angle):
    return 1.0 - angle % twopi / twopi * 2

# A sawtooth function like math.sin(angle)
# 0 returns 1.0, pi returns 0.0, 2*pi returns -1.0
def square(angle):
    if angle < math.pi:
        return 1.0
    else:
        return -1.0

# Generate an array that is sample_len long of values for our waves
w_saw = array.array("H", [0] * sample_len)
for i in range(sample_len):
    w_saw[i] = int(sample_bias + round(32767 * sawtooth(math.pi + twopi * (i+0.5) / sample_len)))

w_sin = array.array("H", [0] * sample_len)
for i in range(sample_len):
    w_sin[i] = int(sample_bias + round(32767 * math.sin(twopi * (i+0.5) / sample_len)))

w_squ = array.array("H", [0] * sample_len)
for i in range(sample_len):
    w_squ[i] = int(sample_bias + round(32767 * square(twopi * (i+0.5) / sample_len)))

#for i in range(sample_len):
#    print (w_sin[i])

wave = []
wave.append(audiocore.RawSample(w_saw, sample_rate=sample_rate))
wave.append(audiocore.RawSample(w_sin, sample_rate=sample_rate))
wave.append(audiocore.RawSample(w_squ, sample_rate=sample_rate))

def dacNoteOn(note, vel):
    global last_note
    last_note = note
    note_freq = note_frequency(note)
    note_sample_rate = round(sample_rate * note_freq / A4refhz)
    
    if sample_wave != 0:
       wave[sample_wave-1].sample_rate = note_sample_rate  # must be integer
       dac.play(wave[sample_wave-1], loop=True)

def dacNoteOff(note, vel):
    global last_note
    # Our monophonic "synth module" needs to ignore keys that lifted on
    # overlapping presses
    if note == last_note:
        dac.stop()
        last_note = None

# Calculate the note frequency from the midi_note
# Returns float
def note_frequency(midi_note):
    # 12 semitones in an octave
    return A4refhz * math.pow(2, (midi_note - midi_note_A4) / 12.0)

last_note = None

def noteOn(x):
    ledOn()
    dacNoteOn(x, 127)
    usb_midi.send(NoteOn(x,127))
    uart.write(bytes([0x90,x,127]))

def noteOff(x):
    dacNoteOff(x, 0)
    usb_midi.send(NoteOff(x,0))
    uart.write(bytes([0x80,x,0]))
    ledOff()

while True:
    # Check any additional IO
    if not wavsw.value:
        # Switch is active low
        sample_wave = sample_wave + 1
        # Recall wave = 0 means "off" so isn't in the wave array
        if sample_wave > len(wave):
            sample_wave = 0
        # crude debouncing...
        time.sleep(0.2)

    if not octsw.value:
        # Switch is active low
        octave = octave + 1
        if octave > 3:
            octave = 0
        # crude debouncing...
        time.sleep(0.2)

    # Activate each column in turn by setting it to low
    for c in range(0, numcols):
        cols[c].value = False

        # Then scan for buttons pressed on the rows
        # Any pressed buttons will be LOW
        for r in range(0, numrows):
            if (rows[r].value == False):
                playnote[c*numrows+r] = 1
                #print ("C: ", c, "R: ", r, "idx: ", (c*numrows+r))
            else:
                playnote[c*numrows+r] = 0

        # Disable it again once done
        cols[c].value = True

    #print (lastnote)
    #print (playnote)

    # Now see if there are any off->on transitions to trigger MIDI on
    # or on->off to trigger MIDI off
    for n in range(0, len(playnote)):
        if (playnote[n] == 1 and lastnote[n] == 0):
            noteOn(firstnote+n+octave*12)
        if (playnote[n] == 0 and lastnote[n] == 1):
            noteOff(firstnote+n+octave*12)
        lastnote[n] = playnote[n]


