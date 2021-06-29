# Pi Pico MIDI Keypad Tenori-On
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
import time
import picokeypad as keypad
import ustruct

MIDI_CH = 1      # MIDI Channel 1 to 16
MIDI_VOICE = 33  # MIDI Voice Number 1 to 128
TEMPO = 240      # beats per minute
midiNotes1 = [
    69,69,69,69,
]
midiNotes2 = [
    67,67,67,67,
]
midiNotes3 = [
    64,64,64,64,
]
midiNotes4 = [
    60,60,60,60,
]

uart = machine.UART(0,31250)

keypad.init()
keypad.set_brightness(1.0)
NUM_NOTES = len(midiNotes1)
NUM_PADS = keypad.get_num_pads()
NUM_STEPS = 4
NUM_ROWS  = NUM_PADS/NUM_STEPS

noteGrid = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
last_button_states = 0
time_ms = 0
step = 0
lastnote = 0
lastkey = -1

def noteOn(x):
    uart.write(ustruct.pack("bbb",0x90+MIDI_CH-1,x,127))
    
def noteOff(x):
    uart.write(ustruct.pack("bbb",0x80+MIDI_CH-1,x,0))
    
def progChange(x):
    if (x<1 or x>128):
        return
    uart.write(ustruct.pack("bb",0xC0+MIDI_CH-1,x-1))
    
def lightUp(x):
    keypad.illuminate(x, 0, 50, 50)

def lightOff(x):
    keypad.illuminate(x, 0, 0, 0)

while True:
    # Scan the keypad all the time
    keypad.update()
    button_states = keypad.get_button_states()
    if last_button_states != button_states:
        last_button_states = button_states
        for key in range (NUM_PADS):
            if (button_states & (1<<key)):
                if (noteGrid[key]):
                    noteGrid[key] = 0
                    lightOff(key)
                else:
                    noteGrid[key] = 1
                    lightUp(key)
            keypad.update()

    # Play the sequencer on our time schedule
    newtime_ms = time.ticks_ms()
    if (newtime_ms > time_ms):
        # Time to wake up!
        
        # Calculate the next "tick" millisecond counter from the TEMPO
        time_ms = newtime_ms + 60000/TEMPO

        # Initialise the MIDI Voice
        progChange(MIDI_VOICE)
        
        for row in range (NUM_ROWS):
            keystep = step + row*NUM_STEPS
            if (noteGrid[keystep] == 0):
                lightOff(keystep)

        step+=1
        if (step >= NUM_STEPS):
            step = 0
        
        # And turn off any last notes playing
        for note in range (NUM_NOTES):
            noteOff(midiNotes1[note])
            noteOff(midiNotes2[note])
            noteOff(midiNotes3[note])
            noteOff(midiNotes4[note])

        # Now process each "column" step
        for row in range (NUM_ROWS):
            keystep = step + row*NUM_STEPS
            if (noteGrid[keystep] == 0):
                keypad.illuminate(keystep, 2,2,2)
            else:
                lightUp(keystep)

            # Play any notes
            if (noteGrid[keystep] != 0):
                if (row == 0):
                    noteOn(midiNotes1[step])
                elif (row == 1):
                    noteOn(midiNotes2[step])
                elif (row == 2):
                    noteOn(midiNotes3[step])
                elif (row == 3):
                    noteOn(midiNotes4[step])
