# Simple MIDI Decoder
# for Micro Python on the Raspberry Pi Pico
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
# Example Usage:
#
#---------------------
#    import machine
#    import SimpleMIDIDecoder
#
#    uart = machine.UART(0,31250)
#    md = SimpleMIDIDecoder.SimpleMIDIDecoder()
#
#    while True:
#        if (uart.any()):
#            md.read(uart.read(1)[0])
#---------------------
#
# To add bespoke handling, define callback functions for NoteOn, NoteOff or
# a default "Thru" handling as shown below.
#
#---------------------
#    def doMidiNoteOn(ch,cmd,note,vel):
#        print("Note On \t", note, "\t", vel)
#
#    def doMidiNoteOff(ch,cmd,note,vel):
#        print("Note Off\t", note, "\t", vel)
#
#    def doMidiThru(ch,cmd,data1,data2):
#        print("Thru\t", cmd, "\t", data1, "\t", data2)
#
#    md = SimpleMIDIDecoder.SimpleMIDIDecoder()
#    md.cbNoteOn (doMidiNoteOn)
#    md.cbNoteOff (doMidiNoteOff)
#    md.cbThru (doMidiThru)
#---------------------


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
class SimpleMIDIDecoder:
    
    def __init__(self):
        self.ch = 0
        self.cmd = 0
        self.d1 = 0
        self.d2 = 0
        self.cbThruFn = self.defThru
        self.cbNoteOnFn = self.defNoteOn
        self.cbNoteOffFn = self.defNoteOff
        
    def cbThru (self, callback):
        self.cbThruFn = callback

    def defThru (self, ch, cmd, d1, d2):
        if (d2 == -1):
            print ("Thru ", ch, ":", hex(cmd), ":", d1)
        else:
            print ("Thru ", ch, ":", hex(cmd), ":", d1, ":", d2)
        
    def cbNoteOn (self, callback):
        self.cbNoteOnFn = callback
    
    def defNoteOn (self, ch, cmd, note, level):
        print ("NoteOn ", ch, ":", note, ":", level)
        
    def cbNoteOff (self, callback):
        self.cbNoteOffFn = callback

    def defNoteOff (self, ch, cmd, note, level):
        print ("NoteOff ", ch, ":", note, ":", level)

    def read(self, mb):
        if ((mb >= 0x80) and (mb <= 0xEF)):
            # MIDI Voice Category Message.
            # Action: Start handling Running Status
            
            # Extract the MIDI command and channel (1-16)
            self.cmd = mb & 0xF0
            self.ch = 1 + mb & 0x0F
            
            # Initialise the two data bytes ready for processing
            self.d1 = 0
            self.d2 = 0
        elif ((mb >= 0xF0) and (mb <= 0xF7)):
            # MIDI System Common Category Message.
            # These are not handled by this decoder.
            # Action: Reset Running Status.
            self.cmd = 0
        elif ((mb >= 0xF8) and (mb <= 0xFF)):
            # System Real-Time Message.
            # These are not handled by this decoder.
            # Action: Ignore these.
            pass
        else:
            # MIDI Data
            if (self.cmd == 0):
                # No record of what state we're in, so can go no further
                return
            if (self.cmd == 0x80):
                # Note OFF Received
                if (self.d1 == 0):
                    # Store the note number
                    self.d1 = mb
                else:
                    # Already have the note, so store the level
                    self.d2 = mb
                    self.cbNoteOffFn (self.ch, self.cmd, self.d1, self.d2)
                    self.d1 = 0
                    self.d2 = 0
            elif (self.cmd == 0x90):
                # Note ON Received
                if (self.d1 == 0):
                    # Store the note number
                    self.d1 = mb
                else:
                    # Already have the note, so store the level
                    self.d2 = mb
                    # Special case if the level (data2) is zero - treat as NoteOff
                    if (self.d2 == 0):
                        self.cbNoteOffFn (self.ch, self.cmd, self.d1, self.d2)
                    else:
                        self.cbNoteOnFn (self.ch, self.cmd, self.d1, self.d2)
                    self.d1 = 0
                    self.d2 = 0
            elif (self.cmd == 0xC0):
                # Program Change
                # This is a single data-byte message
                self.d1 = mb
                self.cbThruFn(self.ch, self.cmd, self.d1, -1)
                self.d1 = 0
            elif (self.cmd == 0xD0):
                # Channel Pressure
                # This is a single data-byte message
                self.d1 = mb
                self.cbThruFn(self.ch, self.cmd, self.d1, -1)
                self.d1 = 0
            else:
                # All other commands are two-byte data commands
                if (self.d1 == 0):
                    # Store the first data byte
                    self.d1 = mb
                else:
                    # Store the second data byte and action
                    self.d2 = mb
                    self.cbThruFn(self.ch, self.cmd, self.d1, self.d2)
                    self.d1 = 0
                    self.d2 = 0


