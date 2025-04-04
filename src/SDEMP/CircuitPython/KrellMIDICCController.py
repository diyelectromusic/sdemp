# Krell MIDI CC Controller
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
import neopixel
from analogio import AnalogIn
import busio
import usb_midi
import adafruit_midi
from adafruit_midi.note_off import NoteOff
from adafruit_midi.note_on import NoteOn
from adafruit_midi.polyphonic_key_pressure import PolyphonicKeyPressure
from adafruit_midi.control_change import ControlChange
from adafruit_midi.program_change import ProgramChange
from adafruit_midi.channel_pressure import ChannelPressure
from adafruit_midi.pitch_bend import PitchBend
from adafruit_midi.midi_message import MIDIUnknownEvent

midicc1 = 1  # Modulation
midicc2 = 7  # Channel volume

alg1_in = AnalogIn(board.A3)
alg2_in = AnalogIn(board.A2)

pixel_pin1 = board.GP2
pixel_pin2 = board.GP3
num_pixels = 5

pixels1 = neopixel.NeoPixel(pixel_pin1, num_pixels, brightness=0.3, auto_write=False, pixel_order=neopixel.RGB)
pixels2 = neopixel.NeoPixel(pixel_pin2, num_pixels, brightness=0.3, auto_write=False, pixel_order=neopixel.RGB)

midiusb = adafruit_midi.MIDI(midi_out=usb_midi.ports[1])
uart = busio.UART(tx=board.GP0, rx=board.GP1, baudrate=31250, timeout=0.001)
midiuart = adafruit_midi.MIDI(midi_in=uart, midi_out=uart)

col = (80, 35, 0)

def algmap(val, minin, maxin, minout, maxout):
    if (val < minin):
        val = minin
    if (val > maxin):
        val = maxin
    return minout + (((val - minin) * (maxout - minout)) / (maxin - minin))

midicc1val = 0
midicc2val = 0
lastalg1 = -1
lastalg2 = -1
while True:
    alg1in = alg1_in.value
    alg1 = int(algmap(alg1in,256,65530,0,127))

    if (lastalg1 != alg1):
        lastalg1 = alg1
        led = alg1 / 25
        for pix in range(5):
            if (pix < led):
                pixels1[pix] = col
            else:
                pixels1[pix] = 0
            pixels1.show()
        midiusb.send(ControlChange(midicc1,alg1))
        midiuart.send(ControlChange(midicc1,alg1))

    alg2in = alg2_in.value
    alg2 = int(algmap(alg2in,256,65530,0,127))
    if (lastalg2 != alg2):
        lastalg2 = alg2
        led = alg2 / 25
        for pix in range(5):
            if (pix < led):
                pixels2[pix] = col
            else:
                pixels2[pix] = 0            
            pixels2.show()
        midiusb.send(ControlChange(midicc2,alg2))
        midiuart.send(ControlChange(midicc2,alg2))
            
    # Perform MIDI THRU funcionality on the serial interface
    # Unfortunately, this doesn't work particularly well...
    msg = midiuart.receive()
    while (msg is not None):
        if (not isinstance(msg, MIDIUnknownEvent)):
            # Ignore unknown MIDI events
            # This filters out the ActiveSensing from my UMT-ONE for example!
            midiuart.send(msg)
    
        msg = midiuart.receive()
