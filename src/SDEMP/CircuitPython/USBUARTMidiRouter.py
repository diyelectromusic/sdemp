# USB to UART MIDI Router for CircuitPython 
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
import busio
import digitalio
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

uart = busio.UART(tx=board.TX, rx=board.RX, baudrate=31250, timeout=0.001)
sermidi = adafruit_midi.MIDI(midi_in=uart, midi_out=uart)
usbmidi = adafruit_midi.MIDI(midi_in=usb_midi.ports[0], midi_out=usb_midi.ports[1])

led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT
led.value = True
#led.value = False

def ledOn():
    led.value = False
#    led.value = True

def ledOff():
    led.value = True
#    led.value = False

while True:
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
