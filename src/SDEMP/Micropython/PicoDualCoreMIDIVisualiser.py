# Pico Dual-Core MIDI Visualiser
# for Micro Python on the Raspberry Pi Pico and
#   Waveshare 8-Segment LED - https://www.waveshare.com/wiki/Pico-8SEG-LED
#   Waveshare RGB LED - https://www.waveshare.com/wiki/Pico-RGB-LED
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
import time
import machine
import ustruct
import _thread
import gc
import SimpleMIDIDecoder
from PicoRGBLED import NeoPixel
from Pico8SEGLED import LED_8SEG, KILOBIT, HUNDREDS, TENS, UNITS, Dot

#
# This code will do the following:
#    Read data from the serial port and interpret it as MIDI data.
#    Respond to MIDI NoteOn and NoteOff events.
#    Light up an LED on the 16x10 matrix corresponding to any notes playing.
#    Display the hex MIDI command and first piece of data on a 7 Segment Display.
#
# It is using the "8-Segment LED" and "RGB LED" modules from Waveshare (links above).
#
# To make this happen, it is using the multi-core properties of the Raspberry Pi Pico.
#
#   The MIDI and RGB LED matrix updating is triggered by events, eg:
#      1) Receiving a character, handling off to the MIDI decoding, etc.
#      2) Decoding NoteOn and NoteOff, resulting in callbacks being called.
#      3) Updating the active LEDs and 8SEG display in response to the NoteOn/NoteOff callbacks.
#   
#   There is also an element of (largely non time critical) scanning of the
#   serial port (for MIDI) and refreshing the LED matrix (resulting in sending
#   160 numbers over the data pin to the RGB LED module.
#
#   However there are two time-critical elements too:
#      a) The RGB LED WS2812 protocol.
#      b) The updating of the 8SEG LED display.
#
#   The RGB LED module uses the WS2812 protocol, so once data starts to be sent and
#   until all transfers are complete, that must not be interrupted and has to proceed
#   in a predictable manner to avoid corruptions or "glitches" in the update.
#
#   But once the RGB LED module has the data, no further processing by the Pico
#   is needed.  Note that the RGB LED library uses the Pico's PIO statemachine
#   to actually drive the LED protocol, so there is a degree of independence there
#   as long as the it is kept fed with the data to be sent.
#
#   The 8SEG-LED is different.  That uses SPI to instruct the module which LEDs
#   to illuminate, but it will only cope with illuminating one digit at a time.
#   To illuminate all four digits, relies on updating each digita in turn (the
#   non-updating digits will turn off while this is happening), and assuming
#   persistence of vision will give the illusion of a continuous display.
#
#   This does, however, mean that if the Pico is busy doing something else (like
#   updating 160 LEDs elsewhere!) then the display will fade or appear uneven.
#
#   One approach is to use a Timer to ensure a periodic refresh of the LED segment,
#   but it is hard to balance the periodicity of the timer - fast enough that
#   persistance of vision works - with time spent away from other processing,
#   especially ensuring that the WS2812 protocol will happen without glitches.  Which
#   isn't easy for 160 LEDs.
#
#   In my experiments, I couldn't find a balanced refresh rate for the Timer that
#   gave good results for both.  Either the 8SEG fades, or MIDI data is missed or
#   the LED matrix didn't update.
#
#   The other approach, which is the one taken here, is to utilise the multi-core
#   aspects of the Pico.  So this is what happens:
#     The "main" core runs the original thread which does the following:
#        a) Scan the UART and trigger the MIDI decoding.
#        b) Update the 8SEG display value, but not the display.
#        c) Update which LEDS in the LED matrix need illuminating.
#        d) Re-scan the LED matrix to update all 160 LEDs.
#
#     The second core runs a seperate thread that does the following:
#        a) Update the 8SEG LED display based on the value set in the original thread.
#        b) Run periodic "garbage collection" for Micropython.
#
#   I don't know why garbage collection is needed, but without it, everything
#   grinds to a halt!
#

# -------------------------------------------------------
#
# Initialise the serial and MIDI handling
#
uart = machine.UART(0,31250)

# Size of the LED array
w = 16
h = 10

playing = []
for p in range (128):
    playing.append(0)

# MIDI callback routines
def doMidiNoteOn(ch, cmd, note, vel):
#    print ("Note On\t", note, "\t", vel)
    setMidiLed(note)
    playing[note]+=1
    segMIDI(ch, cmd, note, vel)
    uart.write(ustruct.pack("bbb",cmd+ch,note,vel))

def doMidiNoteOff(ch, cmd, note, vel):
#    print ("Note Off\t", note, "\t", vel)
    playing[note]-=1
    if (playing[note] <= 0):
        clearMidiLed(note)
        playing[note] = 0
    segMIDI(ch, cmd, note, vel)
    uart.write(ustruct.pack("bbb",cmd+ch,note,vel))

def doMidiThru(ch, cmd, d1, d2):
    segMIDI(ch, cmd, d1, d2)
    if (d2 == -1):
        #print(ch,"\tThru\t", hex(cmd>>4), "\t", d1)
        uart.write(ustruct.pack("bb",cmd+ch,d1))
    else:
        #print(ch,"\tThru\t", hex(cmd>>4), "\t", d1, "\t", d2)
        uart.write(ustruct.pack("bbb",cmd+ch,d1,d2))

md = SimpleMIDIDecoder.SimpleMIDIDecoder()
md.cbNoteOn (doMidiNoteOn)
md.cbNoteOff (doMidiNoteOff)
md.cbThru (doMidiThru)

# -------------------------------------------------------
#
# Initialise the Waveshare 8-Segment LED
#

# Using default GP9,10,11
LED = LED_8SEG()
LEDValue = 0 # Don't use this directly, use the set/get routines
def segInit():
    setLEDValue(0)

# Access functions for the shared LEDValue
def getLEDValue():
    global LEDValue
    return LEDValue

def setLEDValue(value):
    global LEDValue
    LEDValue = value

# Set dimensions of the MIDI display
MIDI_W  = 12
MIDI_H  = 10
# And position it against the LED display
MIDI_WS = -2
MIDI_HS = 0

# Update routines for the 8SEG LED to show
# either four HEX or four DEC digits.
#
def segHex (v, dot):
    global LED
    time.sleep(0.0005)
    LED.write_cmd(UNITS,LED.SEG8[v%16])
    time.sleep(0.0005)
    LED.write_cmd(TENS,LED.SEG8[(v%256)//16])
    time.sleep(0.0005)
    LED.write_cmd(HUNDREDS,LED.SEG8[(v%4096)//256]|dot)
    time.sleep(0.0005)
    LED.write_cmd(KILOBIT,LED.SEG8[(v%65536)//4096])

def segDec (v, dot):
    global LED
    time.sleep(0.0005)
    LED.write_cmd(UNITS,LED.SEG8[v%10])
    time.sleep(0.0005)
    LED.write_cmd(TENS,LED.SEG8[(v%100)//10])
    time.sleep(0.0005)
    LED.write_cmd(HUNDREDS,LED.SEG8[(v%1000)//100]|dot)
    time.sleep(0.0005)
    LED.write_cmd(KILOBIT,LED.SEG8[(v%10000)//1000])

def segScan ():
    ledvalue = getLEDValue()
    segHex(ledvalue, Dot)
    #print ("2: LEDValue=", hex(ledvalue))

def segMIDI(ch, cmd, d1, d2):
    # Fudge so that passing in 0 will still print 0...
    if (ch == 0):
        ch = 1
    # Display can only show two bytes...
    ledvalue = (((cmd|(ch-1))<<8)+d1)
    setLEDValue(ledvalue)
    #print ("1: LEDValue=", hex(ledvalue))

# -------------------------------------------------------
#
# Initialise the Waveshare RGB LED
#

# Using the default GP6 for data
strip = NeoPixel(brightness=0.1)

# From CPython Lib/colorsys.py
def hsv_to_rgb(h, s, v):
    if s == 0.0:
        return v, v, v
    i = int(h * 6.0)
    f = (h * 6.0) - i
    p = v * (1.0 - s)
    q = v * (1.0 - s * f)
    t = v * (1.0 - s * (1.0 - f))
    i = i % 6
    if i == 0:
        return v, t, p
    if i == 1:
        return q, v, p
    if i == 2:
        return p, v, t
    if i == 3:
        return p, q, v
    if i == 4:
        return t, p, v
    if i == 5:
        return v, p, q

def clearDisplay():
    for x in range(w):
        for y in range(h):
            strip.pixels_set(x+y*w, [0, 0, 0])

def midi2pixel(note):
    y = (int)(note/MIDI_W)
    x = note-y*MIDI_W
    y = y + MIDI_HS
    x = x + MIDI_WS
    # need to reverse the x-coordinate
    x = MIDI_W - 1 - x
    return x, y

def ledOn (x, y, r, g, b):
    if (x < 0) or (x >= w) or (y < 0) or (y >= h):
        return
    strip.pixels_set(x+y*w, [r, g, b])

def ledOff (x, y):
    if (x < 0) or (x >= w) or (y < 0) or (y >= h):
        return
    strip.pixels_set(x+y*w, [0, 0, 0])

def setMidiLed (mnote):
    x, y = midi2pixel(mnote)
    r, g, b = [int(c * 255) for c in hsv_to_rgb(((x + y) / MIDI_W / 4), 1.0, 1.0)]
    #print (mnote, "=\t(", x, ",", y, ")\t[", r, g, b, "]\t(", x+y*w, ")")
    ledOn(x, y, r, g, b)

def clearMidiLed (mnote):
    x, y = midi2pixel(mnote)
    ledOff(x, y)


# -------------------------------------------------------
#
# The second thread: handles the 8SEG LED
#

counter = 0
def ledThread():
    global counter
    # Set up the LED display
    segInit()
    
    # The repeatedly scan it!
    while True:
        segScan()
        counter+=1
        if (counter > 100):
            # This apparently helps keep the thread running...
            gc.collect()

_thread.start_new_thread(ledThread, ())

# -------------------------------------------------------
#
# The main thread: Initialisation routines
#

# Initialise the display
clearDisplay()
segMIDI(0,0xaa,0xbb,0) # test pattern

# Show a test pattern
for mnote in [24, 36, 48, 60, 72, 84, 96, 97, 106, 107, 95, 83, 71, 59, 47, 35, 34, 25]:
    setMidiLed(mnote)

strip.pixels_show()
time.sleep(10.0)
clearDisplay()
segMIDI(0,0,0,0)

# -------------------------------------------------------
#
# The main thread itself: handles UART, MIDI, LED matrix
#

scancounter=0
while True:
    # Update the displays every few scans.
    # If we try to do this too often (remember it has to
    # serially output all 160 LED values to update) then
    # everything will just grind to a halt!
    scancounter+=1
    if (scancounter > 100):
        strip.pixels_show()
        scancounter = 0

    # Check for MIDI messages
    if (uart.any()):
        md.read(uart.read(1)[0])
