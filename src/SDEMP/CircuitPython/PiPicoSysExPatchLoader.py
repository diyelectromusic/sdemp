# Pi Pico SysEx Patch Loader
# CircuitPython UART MIDI Version
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
import time
import os
import errno
import re
import digitalio
import displayio
import terminalio
from adafruit_display_text import label
from adafruit_st7789 import ST7789

# NB: We are just sending "raw" sysex MIDI data so I'm avoiding the MIDI library...
uart = busio.UART(board.GP0, board.GP1, baudrate=31250, timeout=0.001)

# Display initialisation code taken from following CP bundle 7 example:
#    st7789_240x135_simpletext_Pimoroni_Pico_Display_Pack.py
#
# Required the following libraries from the bundle:
#    adafruit_st7789.mpy
#
displayio.release_displays()
tft_cs = board.GP17
tft_dc = board.GP16
spi_mosi = board.GP19
spi_clk = board.GP18
spi = busio.SPI(spi_clk, spi_mosi)
display_bus = displayio.FourWire(spi, command=tft_dc, chip_select=tft_cs)
display = ST7789(
    display_bus, rotation=270, width=240, height=135, rowstart=40, colstart=53
)
# End of Display initialisation code

SW_A = 0
SW_B = 1
SW_X = 2
SW_Y = 3
# Pimoroni Switches: A, B, X, Y
pimSw = [board.GP12, board.GP13, board.GP14, board.GP15]
sws = []
lastsws = []
for sw in range(len(pimSw)):
    sw = digitalio.DigitalInOut(pimSw[sw])
    sw.direction = digitalio.Direction.INPUT
    sw.pull = digitalio.Pull.UP
    sws.append(sw)
    lastsws.append(True)

try:
    os.chdir("/syx")
except OSError as exec:
    if exec.args[0] == errno.ENOENT:
        print ("ERROR: No /syx directory found for patches")

files = os.listdir("/syx")

fnregex = re.compile("\.syx$")

fnames = []
for f in files:
    fname = fnregex.split(f)
    fnames.append(fname[0])

def sendSysExFromFile (fileidx):
    if fileidx >= len(fnames):
        return
    filename = fnames[fileidx] + ".syx"
    try:
        os.stat(filename)
    except OSError as exec:
        if exec.args[0] == errno.ENOENT:
            print ("ERROR: Cannot open filename: ", filename)
        else:
            print ("ERROR: Unknown error: ", exec.args[0])
        return

    print ("Opening: ", filename)
    with open (filename, "rb") as fh:
        sysex = fh.read()
        
    #print(sysex)
    written = uart.write(sysex)
    print ("Written: ", written)

#print ("Number of names = ", len(fnames))
#for n in range(len(fnames)):
#    print (n, "->", fnames[n])

menus = displayio.Group()

FONT = terminalio.FONT
TEXTCOLOUR = 0xFFFFFF
BACKCOLOUR = 0x000000
HIGHCOLOUR = 0xC080C0
NAMES_PER_PAGE = 6
namelabs = []
for f in range(NAMES_PER_PAGE):
    namelab = label.Label(FONT, text=" "*20, color=TEXTCOLOUR, scale=2)
    namelab.x = 20
    namelab.y = 10+f*20
    namelabs.append(namelab)
    menus.append(namelab)

NUM_NAMES = len(fnames)
NUM_PAGES = int (1 + len(fnames) / NAMES_PER_PAGE)

print ("Names=", NUM_NAMES, " Pages=", NUM_PAGES)

def showPage (page):
    for d in range(NAMES_PER_PAGE):
        nameidx = NAMES_PER_PAGE*page + d
        if nameidx < len(fnames):
            namelabs[d].text = fnames[nameidx]
        else:
            namelabs[d].text = " "*20
        namelabs[d].color = TEXTCOLOUR

def highlightName (page, name):
    for d in range(NAMES_PER_PAGE):
        nameidx = NAMES_PER_PAGE*page + d
        if nameidx == name:
            namelabs[d].color = HIGHCOLOUR
        else:
            namelabs[d].color = TEXTCOLOUR

page = 0
name = 0

def updateMenuDisplay ():
    global name, page
    showPage(page)
    highlightName(page, name)
    display.show(menus)

def nameUp ():
    global name, page
    name = name + 1
    if name >= NUM_NAMES:
        name = 0
    page = int (name / NAMES_PER_PAGE)
    
def nameDown ():
    global name, page
    name = name - 1
    if name < 0:
        name = NUM_NAMES - 1
    page = int (name / NAMES_PER_PAGE)

def pageUp ():
    global name, page
    name = name + NAMES_PER_PAGE 
    if name >= NUM_NAMES:
        name = 0
    page = int (name / NAMES_PER_PAGE)

def pageDown ():
    global name, page
    name = name - NAMES_PER_PAGE
    if name < 0:
        name = NUM_NAMES - 1
    page = int (name / NAMES_PER_PAGE)

def sendFile ():
    print ("Sending: ", fnames[name])
    sendSysExFromFile(name)

def buttonPress (btn):
    if (btn == SW_A):
        nameDown ()
    elif (btn == SW_B):
        nameUp ()
    elif (btn == SW_X):
        sendFile ()
    elif (btn == SW_Y):
        pageUp ()
    else:
        # Ignore
        pass


updateMenuDisplay()

while True:
    for s in range(len(sws)):
        value = sws[s].value
        if (not value and lastsws[s]):
            # HIGH to LOW transition
            buttonPress(s)
            updateMenuDisplay()
        lastsws[s] = value

# Pause for a while to let the display do its thing
time.sleep(2)

# Without this, the displayio libraries always treat the display as a "console"
displayio.release_displays()

