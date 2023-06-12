# Simple TouchIO MIDI Controller
# CircuitPython and USB and UART MIDI Version
#
# @diyelectromusic
# https://diyelectromusic.wordpress.com/
#
#      MIT License
#      
#      Copyright (c) 2023 diyelectromusic (Kevin)
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
import time
import board
import busio
import touchio
import digitalio
import usb_midi
import adafruit_midi
from adafruit_midi.note_off import NoteOff
from adafruit_midi.note_on import NoteOn
from adafruit_debouncer import Debouncer, Button

# Must be at least a MIDI note for each pin
touchpins = [board.GP12, board.GP13]
midinotes = [60, 62]

MIDI_CHANNEL = 1 # 1 to 16

uart = busio.UART(board.GP0, board.GP1, baudrate=31250)
usb_midi = adafruit_midi.MIDI(
    midi_out=usb_midi.ports[1],
    out_channel=MIDI_CHANNEL-1
    )
ser_midi = adafruit_midi.MIDI(
    midi_out=uart,
    out_channel=MIDI_CHANNEL-1
    )

led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT
def ledOn():
    led.value = True

def ledOff():
    led.value = False

def noteOn(x):
    ledOn()
    usb_midi.send(NoteOn(x,127))
    ser_midi.send(NoteOn(x,127))

def noteOff(x):
    usb_midi.send(NoteOff(x,0))
    ser_midi.send(NoteOff(x,0))
    ledOff()

THRESHOLD = 1000
touchpads = []
for pin in touchpins:
    t = touchio.TouchIn(pin)
    t.threshold = t.raw_value + THRESHOLD
    touchpads.append(Button(t, value_when_pressed=True))

print ("Starting...")

while True:
    for i in range (len(touchpads)):
        t = touchpads[i]
        t.update()
        
        if t.rose:
            # Touch starts
            print("Touch ", i, " On")
            noteOn(midinotes[i])

        if t.fell:
            # Touch ends
            print("Touch ", i, " Off")
            noteOff(midinotes[i])
