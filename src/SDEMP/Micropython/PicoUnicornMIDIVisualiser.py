# Pico Unicorn MIDI Visualiser
# for Micro Python on the Raspberry Pi Pico
#
# @diyelectromusic
# https://diyelectromusic.wordpress.com/
#
#      MIT License
#      
#      Copyright (c) 2021 diyelectromusic (Kevin)
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
import picounicorn
import time
import machine
import ustruct
import SimpleMIDIDecoder

# Initialise the serial MIDI handling
uart = machine.UART(0,31250)

# MIDI callback routines
def doMidiNoteOn(ch, cmd, note, vel):
#    print ("Note On\t", note, "\t", vel)
    setMidiLed(note)
    uart.write(ustruct.pack("bbb",cmd+ch,note,vel))

def doMidiNoteOff(ch, cmd, note, vel):
#    print ("Note Off\t", note, "\t", vel)
    clearMidiLed(note)
    uart.write(ustruct.pack("bbb",cmd+ch,note,vel))

def doMidiThru(ch, cmd, d1, d2):
    if (d2 == -1):
        #print(ch,"\tThru\t", hex(cmd>>4), "\t", d1)
        uart.write(ustruct.pack("bb",cmd+ch,d1))
    else:
        #print(ch,"\tThru\t", hex(cmd>>4), "\t", d1, "\t", d2)
        uart.write(ustruct.pack("bbb",cmd+ch,d1,d2))

md = SimpleMIDIDecoder.SimpleMIDIDecoder()
md.cbNoteOn (doMidiNoteOn)
md.cbNoteOff (doMidiNoteOff)
md.cbThru (doMidiThru)

# Initialise the Pimoroni Unicorn Pack LED display
picounicorn.init()
w = picounicorn.get_width()
h = picounicorn.get_height()

# Set dimensions of the MIDI display
MIDI_W  = 12
MIDI_H  = 7
# And position it against the LED display
MIDI_WS = 2
MIDI_HS = -2

print ("Width=", w, " Height=", h)

# From CPython Lib/colorsys.py
def hsv_to_rgb(h, s, v):
    if s == 0.0:
        return v, v, v
    i = int(h * 6.0)
    f = (h * 6.0) - i
    p = v * (1.0 - s)
    q = v * (1.0 - s * f)
    t = v * (1.0 - s * (1.0 - f))
    i = i % 6
    if i == 0:
        return v, t, p
    if i == 1:
        return q, v, p
    if i == 2:
        return p, v, t
    if i == 3:
        return p, q, v
    if i == 4:
        return t, p, v
    if i == 5:
        return v, p, q

def clearDisplay():
    for x in range(w):
        for y in range(h):
            picounicorn.set_pixel(x, y, 0, 0, 0)

def midi2pixel(note):
    y = (int)(note/MIDI_W)
    x = note-y*MIDI_W
    y = y + MIDI_HS
    x = x + MIDI_WS
    # need to reverse the y-coordinate
    y = MIDI_H - 1 - y
    return x, y

def ledOn (x, y, r, g, b):
    if (x < 0) or (x >= w) or (y < 0) or (y >= h):
        return
    picounicorn.set_pixel(x, y, r, g, b)

def ledOff (x, y):
    if (x < 0) or (x >= w) or (y < 0) or (y >= h):
        return
    picounicorn.set_pixel(x, y, 0, 0, 0)

def setMidiLed (mnote):
    x, y = midi2pixel(mnote)
    r, g, b = [int(c * 255) for c in hsv_to_rgb(((x + y) / MIDI_W / 4), 1.0, 1.0)]
#    print (mnote, "=\t(", x, ",", y, ")\t[", r, g, b, "]")
    ledOn(x, y, r, g, b)

def clearMidiLed (mnote):
    x, y = midi2pixel(mnote)
    ledOff(x, y)

# Initialise the display
clearDisplay()
# Show a test pattern
for mnote in [24, 36, 48, 60, 72, 84, 96, 97, 106, 107, 95, 83, 71, 59, 47, 35, 34, 25]:
    setMidiLed(mnote)

time.sleep(1.0)
clearDisplay()

while True:
    # Check for MIDI messages
    if (uart.any()):
        md.read(uart.read(1)[0])

