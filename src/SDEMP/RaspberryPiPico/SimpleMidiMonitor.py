# Simple MIDI Monitor
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
import machine
import utime
import ustruct

pin = machine.Pin(25, machine.Pin.OUT)
uart = machine.UART(1,31250)

# Basic MIDI handling commands
def doMidiNoteOn(note,vel):
    pin.value(1)
    print("Note On \t", note, "\t", vel)

def doMidiNoteOff(note,vel):
    pin.value(0)
    print("Note Off\t", note, "\t", vel)

# Implement a simple MIDI decoder.
#
# MIDI supports the idea of Running Status.
#
# If the command is the same as the previous one, 
# then the status (command) byte doesn't need to be sent again.
#
# The basis for handling this can be found here:
#  http://midi.teragonaudio.com/tech/midispec/run.htm
# Namely:
#   Buffer is cleared (ie, set to 0) at power up.
#   Buffer stores the status when a Voice Category Status (ie, 0x80 to 0xEF) is received.
#   Buffer is cleared when a System Common Category Status (ie, 0xF0 to 0xF7) is received.
#   Nothing is done to the buffer when a RealTime Category message is received.
#   Any data bytes are ignored when the buffer is 0.
#
MIDICH = 1
MIDIRunningStatus = 0
MIDINote = 0
MIDILevel = 0
def doMidi(mb):
    global MIDIRunningStatus
    global MIDINote
    global MIDILevel
    if ((mb >= 0x80) and (mb <= 0xEF)):
        # MIDI Voice Category Message.
        # Action: Start handling Running Status
        MIDIRunningStatus = mb
        MIDINote = 0
        MIDILevel = 0
    elif ((mb >= 0xF0) and (mb <= 0xF7)):
        # MIDI System Common Category Message.
        # Action: Reset Running Status.
        MIDIRunningStatus = 0
    elif ((mb >= 0xF8) and (mb <= 0xFF)):
        # System Real-Time Message.
        # Action: Ignore these.
        pass
    else:
        # MIDI Data
        if (MIDIRunningStatus == 0):
            # No record of what state we're in, so can go no further
            return
        if (MIDIRunningStatus == (0x80|(MIDICH-1))):
            # Note OFF Received
            if (MIDINote == 0):
                # Store the note number
                MIDINote = mb
            else:
                # Already have the note, so store the level
                MIDILevel = mb
                doMidiNoteOff (MIDINote, MIDILevel)
                MIDINote = 0
                MIDILevel = 0
        elif (MIDIRunningStatus == (0x90|(MIDICH-1))):
            # Note ON Received
            if (MIDINote == 0):
                # Store the note number
                MIDINote = mb
            else:
                # Already have the note, so store the level
                MIDILevel = mb
                if (MIDILevel == 0):
                    doMidiNoteOff (MIDINote, MIDILevel)
                else:
                    doMidiNoteOn (MIDINote, MIDILevel)
                MIDINote = 0
                MIDILevel = 0
        else:
            # This is a MIDI command we aren't handling right now
            pass

while True:
    if (uart.any()):
        doMidi(uart.read(1)[0])
