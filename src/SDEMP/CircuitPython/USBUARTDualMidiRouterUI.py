# USB to UART MIDI Router for CircuitPython with RE/SSD1306 UI
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
import displayio
import digitalio
import terminalio
import adafruit_displayio_ssd1306
from adafruit_display_text import label
from adafruit_display_shapes.rect import Rect
import rotaryio
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

uart1 = busio.UART(tx=board.GP0, rx=board.GP1, baudrate=31250, timeout=0.001)
sermidi1 = adafruit_midi.MIDI(midi_in=uart1, midi_out=uart1)
uart2 = busio.UART(tx=board.GP4, rx=board.GP5, baudrate=31250, timeout=0.001)
sermidi2 = adafruit_midi.MIDI(midi_in=uart2, midi_out=uart2)
usbmidi = adafruit_midi.MIDI(midi_in=usb_midi.ports[0], midi_out=usb_midi.ports[1])

led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT
led.value = False

# Configure a default route, to be used in the case
# of no other matches.  Set to -1 to disable.
MIDIDEF = 0

WIDTH = 128
HEIGHT = 32

displayio.release_displays()

# encoder initialisation
re = rotaryio.IncrementalEncoder(board.GP16, board.GP17)
re_position = 0
button = digitalio.DigitalInOut(board.GP18)
button.direction = digitalio.Direction.INPUT
button.pull = digitalio.Pull.UP
button_state = True

# CLK/SDA
i2c = busio.I2C(board.GP15, board.GP14)
while not i2c.try_lock():
    pass

addrs = i2c.scan()
i2c.unlock()
if not addrs:
    print ("No I2C devices found")
else:
    print (hex(addrs[0]))

display_bus = displayio.I2CDisplay(i2c, device_address=0x3c)
display = adafruit_displayio_ssd1306.SSD1306(display_bus, width=WIDTH, height=HEIGHT, rotation=180)

INPORTS = 3  # 0 = USB, 1,2 = UART0,1
intext = ["U", "0", "1"]
OUTPORTS = 3
outtext = ["U", "0", "1"]
cursor = -1
select_mode = False
selected = None

panel = displayio.Group()
rect = Rect(0, 0, WIDTH, HEIGHT, outline=0xFFFFFF, stroke=1)
panel.append(rect)

INS = 0
OUTS = 1
screen = []
ins = []
outs = []
screen.append(ins)
screen.append(outs)
for i in range(INPORTS):
    screen[INS].append(True)
for o in range(OUTPORTS):
    screen[OUTS].append(True)

MIDIRT = []

# Initialise the routing table on a 1 IN to 1 OUT basis
for i in range(INPORTS):
    outroutes = []
    MIDIRT.append(outroutes)
    for o in range(OUTPORTS):
        if (i == o):
            MIDIRT[i].append(True)
        else:
            MIDIRT[i].append(False)

PORTS = INPORTS+OUTPORTS
# Take anindex into the combined INPORT/OUTPORT list
# and return the port number in either the INPORT or OUTPORT lists
def idx2port (idx):
    if idx < INPORTS:
        return idx
    else:
        return idx-INPORTS

# Take a port number in either the INPORT or OUTPORT lists
# and return the index into the combined INPORT/OUTPORT list.
def inport2idx (port):
    return port

def outport2idx (port):
    return port+INPORTS

# Take an index into the combined INPORT/OUTPORT list
# and return True if it is an INPORT and False if an OUTPORT
def isInPort (idx):
    if idx < INPORTS:
        return True
    else:
        return False

def portOn (pidx):
    if pidx < PORTS:
        if isInPort(pidx):
            screen[INS][idx2port(pidx)] = True
        else:
            screen[OUTS][idx2port(pidx)] = True

def portOff (pidx):
    if pidx < PORTS:
        if isInPort(pidx):
            screen[INS][idx2port(pidx)] = False
        else:
            screen[OUTS][idx2port(pidx)] = False

def portToggle (pidx):
    if pidx < PORTS:
        if isInPort(pidx):
            if screen[INS][idx2port(pidx)]:
                screen[INS][idx2port(pidx)] = False
            else:
                screen[INS][idx2port(pidx)] = True
        else:
            if screen[OUTS][idx2port(pidx)]:
                screen[OUTS][idx2port(pidx)] = False
            else:
                screen[OUTS][idx2port(pidx)] = True

FONT = terminalio.FONT
TEXTCOL = 0xFFFFFF
BACKCOL = 0x000000
SCALE   = 1
TXT_Y_IN  = 8
TXT_Y_OUT = 22
LAB_X_START = 5
TXT_X_START = 40
TXT_X_GAP   = 10

INlab = label.Label(FONT, text="IN", color=TEXTCOL, background_color=BACKCOL, scale=SCALE)
INlab.x = LAB_X_START
INlab.y = TXT_Y_IN
panel.append(INlab)

OUTlab = label.Label(FONT, text="OUT", color=TEXTCOL, background_color=BACKCOL, scale=SCALE)
OUTlab.x = LAB_X_START
OUTlab.y = TXT_Y_OUT
panel.append(OUTlab)

inlabs = []
for l in range(INPORTS):
    portlab = label.Label(FONT, text=intext[l], color=TEXTCOL, background_color=BACKCOL, scale=SCALE)
    portlab.x = TXT_X_START + TXT_X_GAP*l
    portlab.y = TXT_Y_IN
    inlabs.append(portlab)
    panel.append(portlab)
    portOn(inport2idx(l))

outlabs = []
for l in range(OUTPORTS):
    portlab = label.Label(FONT, text=outtext[l], color=TEXTCOL, background_color=BACKCOL, scale=SCALE)
    portlab.x = TXT_X_START + TXT_X_GAP*l
    portlab.y = TXT_Y_OUT
    outlabs.append(portlab)
    panel.append(portlab)
    portOn(outport2idx(l))

display.show(panel)

def displayReset ():
    for i in range(INPORTS):
        screen[INS][i] = True
    for o in range(OUTPORTS):
        screen[OUTS][o] = True

def displayPortOn (pidx):
    global inlabs, outlabs, cursor
    if cursor == pidx:
        if isInPort(pidx):
            inlabs[idx2port(pidx)].color=BACKCOL
            inlabs[idx2port(pidx)].background_color=TEXTCOL
        else:
            outlabs[idx2port(pidx)].color=BACKCOL
            outlabs[idx2port(pidx)].background_color=TEXTCOL
    else:
        if isInPort(pidx):
            inlabs[idx2port(pidx)].color=TEXTCOL
            inlabs[idx2port(pidx)].background_color=BACKCOL
        else:
            outlabs[idx2port(pidx)].color=TEXTCOL
            outlabs[idx2port(pidx)].background_color=BACKCOL

def displayPortOff (pidx):
    global inlabs, outlabs, cursor
    if cursor == pidx:
        if isInPort(pidx):
            inlabs[idx2port(pidx)].color=TEXTCOL
            inlabs[idx2port(pidx)].background_color=TEXTCOL
        else:
            outlabs[idx2port(pidx)].color=TEXTCOL
            outlabs[idx2port(pidx)].background_color=TEXTCOL
    else:
        if isInPort(pidx):
            inlabs[idx2port(pidx)].color=BACKCOL
            inlabs[idx2port(pidx)].background_color=BACKCOL
        else:
            outlabs[idx2port(pidx)].color=BACKCOL
            outlabs[idx2port(pidx)].background_color=BACKCOL

def displayUpdate (inports, outports):
    global selected, select_mode
    if select_mode:
        for p in range(INPORTS):
            if p == selected:
                displayPortOn(inport2idx(p))
            else:
                displayPortOff(inport2idx(p))
        for p in range(OUTPORTS):
            if outports[p]:
                displayPortOn(outport2idx(p))
            else:
                displayPortOff(outport2idx(p))
    else:
        for p in range(INPORTS):
            if inports[p]:
                displayPortOn(inport2idx(p))
            else:
                displayPortOff(inport2idx(p))
        for p in range(OUTPORTS):
            if outports[p]:
                displayPortOn(outport2idx(p))
            else:
                displayPortOff(outport2idx(p))

def cursorUpdate (inc: bool):
    global cursor, selected, select_mode
    if not select_mode:
        if inc:
            cursor += 1
            # NB: Allow one extra position for a "blank"
            if cursor > PORTS:
                cursor = 0
        else:
            cursor -= 1
            if cursor < 0:
                cursor = PORTS
    else:
        # Select mode, only move cursor among the OUTPORTS and
        # the one selected INPORT
        if inc:
            if cursor == selected:
                cursor = INPORTS
            else:
                cursor += 1
                if cursor >= PORTS:
                    cursor = selected
        else:
            if cursor == selected:
                cursor = PORTS-1
            else:
                cursor -= 1
                if cursor < INPORTS:
                    cursor = selected        

def routes2display (selected):
    # Find the routing table for selected and set
    # the display accordingly
    for i in range (OUTPORTS):
        screen[OUTS][i] = MIDIRT[selected][i]

def display2routes (selected):
    # take the display settings and turn them
    # into the routing table for selected
    for i in range (OUTPORTS):
        MIDIRT[selected][i] = screen[OUTS][i]    

displayReset()
displayUpdate(screen[INS], screen[OUTS])

display.show(panel)


def midiRouter(s_src, s_ch):
    d_dst = []
    
    # Simple routing, just return the list of destinations
    # from the routing table (we don't do anything with channel yet)
    for o in range(OUTPORTS):
        if MIDIRT[s_src][o]:
            d_dst.append(o)

    # Return the destinations found, eliminating duplicates
    return set(d_dst)

def routeMidi (src, msg):
    # NB: Adafruit MIDI channels go 0 to 15, convert to 1 to 16
    dst = midiRouter(src, msg.channel + 1)
    print (src, " -> ", dst)
    if dst:
        # The adafruit MIDI library will update the channel
        # in the message if you don't tell it what channel to use...
        channel = msg.channel
        for d in dst:
            if d == 2:
                sermidi2.send(msg, channel=channel)
            elif d == 1:
                sermidi1.send(msg, channel=channel)
            else:
                usbmidi.send(msg, channel=channel)

def ledOn():
    led.value = True

def ledOff():
    led.value = False

while True:
    msg = usbmidi.receive()
    if (isinstance(msg, MIDIUnknownEvent)):
        # Ignore unknown MIDI events
        # This filters out the ActiveSensing from my UMT-ONE for example!
        pass
    elif msg is not None:
        ledOn()
        routeMidi(0, msg)
    else:
       # Ignore "None"
       pass

    msg = sermidi1.receive()
    if (isinstance(msg, MIDIUnknownEvent)):
        # Ignore unknown MIDI events
        pass
    elif msg is not None:
        ledOn()
        routeMidi(1, msg)
    else:
       # Ignore "None"
       pass

    msg = sermidi2.receive()
    if (isinstance(msg, MIDIUnknownEvent)):
        # Ignore unknown MIDI events
        pass
    elif msg is not None:
        ledOn()
        routeMidi(2, msg)
    else:
       # Ignore "None"
       pass

    ledOff()

    button_value = button.value
    if not button_value and button_state:
        if select_mode:
            if (cursor < INPORTS):
                display2routes(selected)
                select_mode = False
                selected = None
                displayReset()
            else:
                portToggle(cursor)        
        else:
            if cursor >= 0 and cursor < PORTS:
                select_mode = True
                selected = cursor
                cursor = INPORTS
                routes2display(selected)
#        print ("Button\t", select_mode)
    button_state = button_value

    position = re.position
    if position > re_position:
        cursorUpdate(True)
#        print(position, "\t>>>>")
    elif position < re_position:
        cursorUpdate(False)
#        print(position, "\t<<<<")
    re_position = position

    displayUpdate(screen[INS], screen[OUTS])
