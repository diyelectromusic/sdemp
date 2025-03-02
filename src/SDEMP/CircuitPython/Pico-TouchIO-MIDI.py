# Pico Touch Board MIDI Test
# CircuitPython and USB and UART MIDI Version
#
# @diyelectromusic
# https://diyelectromusic.wordpress.com/
#
#      MIT License
#      
#      Copyright (c) 2025 diyelectromusic (Kevin)
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
touchpins = [
    board.GP2, board.GP3, board.GP4, board.GP5,
    board.GP6, board.GP7, board.GP8, board.GP9,
    board.GP10, board.GP11, board.GP12, board.GP13,
    board.GP14, board.GP15, board.GP16, board.GP17,
    board.GP18, board.GP19, board.GP20, board.GP21,
    board.GP22
]
midibase = 60

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
            print("Touch GP", 2+i, " On")
            noteOn(midibase+i)

        if t.fell:
            # Touch ends
            print("Touch GP", 2+i, " Off")
            noteOff(midibase+i)
