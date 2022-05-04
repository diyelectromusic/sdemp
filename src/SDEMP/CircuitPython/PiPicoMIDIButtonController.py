# Pi Pico MIDI Button Controller
# CircuitPython and USB MIDI
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
import digitalio
import busio
import usb_midi
import adafruit_midi
from adafruit_midi.control_change import ControlChange
from adafruit_midi.program_change import ProgramChange

# Set the values to be sent in the Bank Select messages
# for each button.
#   [MSB, LSB] = CC0, CC32
midiBanks = [
    [0, 0],   # Button 1
    [0, 1],   # Button 2
    [0, 2],   # Button 3
    [0, 3]    # Button 4
]

led = digitalio.DigitalInOut(board.GP25)
led.direction = digitalio.Direction.OUTPUT
usb_midi = adafruit_midi.MIDI(
    midi_out=usb_midi.ports[1],
    out_channel=0,
    debug=True
    )

uart = busio.UART(board.GP0, board.GP1, baudrate=31250)
uart_midi = adafruit_midi.MIDI(
    midi_out=uart,
    out_channel=0
    )

def ledOn():
    led.value = True

def ledOff():
    led.value = False

MIDI_CC_BANKSEL_MSB = 0
MIDI_CC_BANKSEL_LSB = 32
def midiBankSelect(bankIdx):
    ledOn()
    # Send MSB, LSB followed by a ProgramChange to Voice 0 over USB MIDI
    usb_midi.send(ControlChange(MIDI_CC_BANKSEL_MSB, midiBanks[bankIdx][0]))
    usb_midi.send(ControlChange(MIDI_CC_BANKSEL_LSB, midiBanks[bankIdx][1]))
    usb_midi.send(ProgramChange(0))
    # Then over serial MIDI
    uart_midi.send(ControlChange(MIDI_CC_BANKSEL_MSB, midiBanks[bankIdx][0]))
    uart_midi.send(ControlChange(MIDI_CC_BANKSEL_LSB, midiBanks[bankIdx][1]))
    uart_midi.send(ProgramChange(0))
    ledOff()

ledOn()

# Switch OFF will be HIGH (operating in PULL_UP mode)
btns = []
lastbtns = []
btn_pins = [board.GP3,board.GP7,board.GP11,board.GP15]
numbtns = len(btn_pins)
for bp in range(0, numbtns):
    btns.append(digitalio.DigitalInOut(btn_pins[bp]))
    btns[bp].switch_to_input(pull=digitalio.Pull.UP)
    lastbtns.append(True)

ledOff()

while True:
    # Scan for buttons pressed on the rows
    # Any pressed buttons will be LOW
    for b in range(0, numbtns):
        if (btns[b].value == False) and (lastbtns[b] == True):
            # Button Pressed
            midiBankSelect(b)

        lastbtns[b] = btns[b].value
