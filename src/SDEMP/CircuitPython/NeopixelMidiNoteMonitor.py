# MIDI Note Monitor
# for Circuit Python on the Waveshare RP2040 Matrix
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
import neopixel
import usb_midi
import adafruit_midi
from adafruit_midi.note_off import NoteOff
from adafruit_midi.note_on import NoteOn

MIN_NOTE=48       # MIDI Note C3
MAX_NOTE=(48+25-1)  # MIDI Note C#5

pixel_pin = board.GP16
num_pixels = 25

def midi2pix (midinote):
    # Pick one, depending on order required
    return num_pixels - (msg.note - MIN_NOTE) - 1
    #return msg.note - MIN_NOTE

# Serial MIDI
uart = busio.UART(tx=board.TX, rx=board.RX, baudrate=31250, timeout=0.001)
midi = adafruit_midi.MIDI(midi_in=uart)

# USB MIDI
# midi = adafruit_midi.MIDI(midi_in=usb_midi.ports[0])

pixels = neopixel.NeoPixel(pixel_pin, num_pixels, brightness=0.1, auto_write=False, pixel_order=neopixel.RGB)

print ("Ready...")

OFF = (0,0,0)
RED = (64, 0, 0)
YELLOW = (64, 35, 0)
GREEN = (0, 64, 0)
CYAN = (0, 64, 64)
BLUE = (0, 0, 64)
PURPLE = (45, 0, 64)

pixels.fill(OFF)

while True:
    msg = midi.receive()
    if (msg is not None):
        if (isinstance(msg, NoteOn)):
            if (msg.note >= MIN_NOTE and msg.note <= MAX_NOTE):
                pixels[midi2pix(msg.note)] = GREEN;

        if (isinstance(msg, NoteOff)):
            if (msg.note >= MIN_NOTE and msg.note <= MAX_NOTE):
                pixels[midi2pix(msg.note)] = OFF;

    pixels.show()
