# Pi Pico USB UART MIDI Pedal Keyboard
# CircuitPython and USB and UART MIDI Version
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
import busio
import adafruit_midi
from adafruit_midi.note_off import NoteOff
from adafruit_midi.note_on import NoteOn

uart = busio.UART(board.GP0, board.GP1, baudrate=31250)

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

firstnote = 36 # C2
keys = []
playnote = []
lastnote = []

def noteOn(x):
    ledOn()
    usb_midi.send(NoteOn(x,127))
    uart.write(bytes([0x90,x,127]))

def noteOff(x):
    usb_midi.send(NoteOff(x,0))
    uart.write(bytes([0x80,x,0]))
    ledOff()

ledOn()

# Switch OFF will be HIGH (operating in PULL_UP mode)
key_pins = [board.GP6,board.GP21,board.GP7,board.GP20,board.GP8,board.GP19,board.GP9,board.GP18,board.GP10,board.GP17,board.GP11,board.GP16,board.GP12]
numkeys = len(key_pins)
for kp in range(0, numkeys):
    keys.append(digitalio.DigitalInOut(key_pins[kp]))
    keys[kp].switch_to_input(pull=digitalio.Pull.UP)

for k in range(0, numkeys):
    # initialise the note list
    playnote.append(0)
    lastnote.append(0)

ledOff()

while True:
    # Scan for buttons pressed on the rows
    # Any pressed buttons will be LOW
    for k in range(0, numkeys):
        if (keys[k].value == False):
            playnote[k] = 1
            print ("Key: ", k)
        else:
            playnote[k] = 0

    #print (lastnote)
    #print (playnote)

    # Now see if there are any off->on transitions to trigger MIDI on
    # or on->off to trigger MIDI off
    for n in range(0, len(playnote)):
        if (playnote[n] == 1 and lastnote[n] == 0):
            noteOn(firstnote+n)
        if (playnote[n] == 0 and lastnote[n] == 1):
            noteOff(firstnote+n)
        lastnote[n] = playnote[n]
