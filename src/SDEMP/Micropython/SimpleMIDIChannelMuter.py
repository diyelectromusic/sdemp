# MIDI Channel Muter
# for Micro Python on the Raspberry Pi Pico
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
# This needs the SimpleMIDIDecoder.py module from @diyelectromusic too.
#
from machine import Pin, UART
import ustruct
import SimpleMIDIDecoder
import picokeypad as keypad

UART_BAUD = 31250

# Allow for MIDI channels
MIDICH = [
    True, True, True, True,
    True, True, True, True,
    True, True, True, True,
    True, True, True, True,
]

led = Pin(25, machine.Pin.OUT)
uart = UART(0,31250)

def midiAllNotesOff (ch):
    # Send the MIDI CC Channel Message for All Notes OFf
    uart.write(ustruct.pack("bbb",0xB0+ch-1,123, 0))

def midiChannelMuter (ch, cmd, d1, d2):
    if (not MIDICH[ch-1]):
        return

    if (d2 == -1):
        uart.write(ustruct.pack("bb",cmd+ch-1,d1))
    else:
        uart.write(ustruct.pack("bbb",cmd+ch-1,d1,d2))

# Basic MIDI handling commands
def doMidiNoteOn(ch,cmd,note,vel):
    #print(ch,"\tNote On \t", note, "\t", vel)
    led.value(1)
    midiChannelMuter(ch, cmd, note, vel)

def doMidiNoteOff(ch,cmd,note,vel):
    #print(ch,"\tNote Off\t", note, "\t", vel)
    led.value(0)
    midiChannelMuter(ch, cmd, note, vel)

def doMidiThru(ch,cmd,d1,d2):
    midiChannelMuter(ch, cmd, d1, d2)

md = SimpleMIDIDecoder.SimpleMIDIDecoder()
md.cbNoteOn (doMidiNoteOn)
md.cbNoteOff (doMidiNoteOff)
md.cbThru (doMidiThru)

keypad.init()
keypad.set_brightness(1.0)
last_button_states = 0
colour_index = 0
NUM_PADS = keypad.get_num_pads()

while True:
    if (uart.any()):
        md.read(uart.read(1)[0])

    button_states = keypad.get_button_states()
    if last_button_states != button_states:
        last_button_states = button_states
        if button_states > 0:
                button = 0
                for find in range (0, NUM_PADS):
                    if button_states & (1<<find):
                        if (MIDICH[find]):
                            # Disable this channel
                            MIDICH[find] = False
                            
                            # Send the All Notes Off to this MIDI channel
                            # using ch = 1 to 16 indexing
                            midiAllNotesOff(find+1)
                        else:
                            MIDICH[find] = True
    
    for i in range (0, NUM_PADS):
        if MIDICH[i]:
            keypad.illuminate(i, 0x00, 0x10, 0x00)
        else:
            keypad.illuminate(i, 0x05, 0x05, 0x05)
            
    keypad.update()
