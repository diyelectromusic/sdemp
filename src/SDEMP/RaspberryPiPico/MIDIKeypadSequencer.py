# Pi Pico MIDI Keypad Sequencer
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
midiNotes = [
    0,   # 0 means "don't play" so ignore the first value
    60,61,62,63, 64,65,66,67, 68,69,70,71, 72,73,74,75
]

uart = machine.UART(0,31250)

keypad.init()
keypad.set_brightness(1.0)
NUM_NOTES = len(midiNotes)
NUM_PADS = keypad.get_num_pads()

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
    
# Based on code from https://github.com/sandyjmacdonald
def colourwheel(pos):
    if pos <= 0:
        return (0,0,0)
    if pos >= 250:
        return (255,255,255)
    if pos < 85:
        return (255 - pos * 3, pos * 3, 0)
    if pos < 170:
        pos -= 85
        return (0, 255 - pos * 3, pos * 3)
    pos -= 170
    return (pos * 3, 0, 255 - pos * 3)

def lightUp(x):
    rgb = colourwheel(noteGrid[x]*16)
    keypad.illuminate(x, rgb[0], rgb[1], rgb[2])

while True:
    # Scan the keypad all the time
    keypad.update()
    button_states = keypad.get_button_states()
    if last_button_states != button_states:
        last_button_states = button_states
        for key in range (NUM_PADS):
            if (button_states & (1<<key)):
                lastkey = key
                noteGrid[key]+=1
                if (noteGrid[key] >= NUM_NOTES):
                    noteGrid[key] = 0
                noteOn(midiNotes[noteGrid[key]])
                lightUp(key)
            keypad.update()

    # Play the sequencer on our time schedule
    newtime_ms = time.ticks_ms()
    if (newtime_ms > time_ms):
        # Time to wake up!
        
        # Calculate the next "tick" millisecond counter from the TEMPO
        time_ms = newtime_ms + 60000/TEMPO

        # Turn off the last step unless the key has just been pressed,
        # in which case leave it on for one more step...
        if (step != lastkey):
            keypad.illuminate(step,0,0,0)
        lastkey = -1

        # Initialise the MIDI Voice
        progChange(MIDI_VOICE)

        step+=1
        if (step >= NUM_PADS):
            step = 0
        
        # And turn off any last notes playing
        for note in range (NUM_PADS):
            noteOff(midiNotes[note])

        # Illuminate the new step
        if (noteGrid[step] == 0):
            keypad.illuminate(step, 2,2,2)
        else:
            lightUp(step)

        # Play the next note
        if (noteGrid[step] != 0):
            noteOn(midiNotes[noteGrid[step]])
            lastnote = noteGrid[step]
