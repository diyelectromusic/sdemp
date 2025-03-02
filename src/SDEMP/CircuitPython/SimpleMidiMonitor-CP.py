# Simple MIDI Monitor
# for Circuit Python on the Adafruit Circuit Playground Express
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
import busio
import usb_midi
import adafruit_midi
from adafruit_midi.note_off import NoteOff
from adafruit_midi.note_on import NoteOn

led = digitalio.DigitalInOut(board.GP25)
led.direction = digitalio.Direction.OUTPUT
midi = adafruit_midi.MIDI(midi_in=usb_midi.ports[0])
#uart = busio.UART(tx=board.GP0, rx=board.GP1, baudrate=31250, timeout=0.001)
#midi = adafruit_midi.MIDI(midi_in=uart)

while True:
    msg = midi.receive()
    if (isinstance(msg, NoteOn)):
        print (msg)
        led.value = True
        print ("Note On: \t",msg.note,"\t",msg.velocity)
    if (isinstance(msg, NoteOff)):
        led.value = False
        print ("Note Off:\t",msg.note,"\t",msg.velocity)
