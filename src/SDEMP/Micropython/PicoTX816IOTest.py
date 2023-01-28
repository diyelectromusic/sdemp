# TX816 IO Panel Test
# for Micro Python on the Raspberry Pi Pico
#
# @diyelectromusic
# https://diyelectromusic.wordpress.com/
#
#      MIT License
#      
#      Copyright (c) 2023 diyelectromusic (Kevin)
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
# This code uses following two libaries:
#   mcp3008 from https://github.com/romilly/pico-code
#   max7219 from https://github.com/JennaSys/micropython-max7219
#
# IMPORTANT: the max7219 library is slightly modified.
#   In the __init__ constructor, the SPI initialisation line is changed from 
#      self._spi = SPI(spi_bus, baudrate=baudrate, polarity=0, phase=0)
#   to
#      self._spi = SPI(0, baudrate=baudrate, polarity=0, phase=0, sck=Pin(18), mosi=Pin(19), miso=Pin(16))
#
import max7219
from mcp3008 import MCP3008
from machine import Pin, SPI
from time import sleep

# 595 initialisation
HC595_SHCP = 21
HC595_STCP = 20
HC595_DS   = 22
PINdata = machine.Pin(HC595_DS, machine.Pin.OUT)
PINlatch = machine.Pin(HC595_STCP, machine.Pin.OUT)
PINclock = machine.Pin(HC595_SHCP, machine.Pin.OUT)

def update595 (value):
    PINclock.value(0)
    PINlatch.value(0)
    PINclock.value(1)
    
    for i in range(7, -1, -1):
        PINclock.value(0)
        PINdata.value((value>>i) & 1)
        PINclock.value(1)
        
    PINclock.value(0)
    PINlatch.value(1)
    PINclock.value(1)

for i in range (8):
    update595((1<<i))
    sleep(0.1)
for i in range (6,-1,-1):
    update595((1<<i))
    sleep(0.1)
update595(0)

# MCP3008 initialisation on SPI1
spi = machine.SPI(1, sck=Pin(14),mosi=Pin(15),miso=Pin(12), baudrate=100000)
sleep(1)
cs = machine.Pin(13, machine.Pin.OUT)
pots = MCP3008(spi, cs)
sleep(1)

# MAX7219 initialisation on SPI0
display = max7219.SevenSegment(digits=8, scan_digits=8, cs=17, spi_bus=0, reverse=False)
display.brightness(4)
display.text("88888888")
sleep(3)
display.text("        ")
display.clear()
sleep(1)

# Switches for TG1-4: 2, 3, 6, 7
switches = [
    machine.Pin(2, machine.Pin.IN, machine.Pin.PULL_UP),
    machine.Pin(3, machine.Pin.IN, machine.Pin.PULL_UP),
    machine.Pin(6, machine.Pin.IN, machine.Pin.PULL_UP),
    machine.Pin(7, machine.Pin.IN, machine.Pin.PULL_UP)
]

swreading = [1,1,1,1]  # HIGH = not pushed
lastswreading = [1,1,1,1]
swvalues = [2,2,2,2]   # Toggles between 2 and 1

potreading = [0,0,0,0,0,0,0]
lastpotreading = [0,0,0,0,0,0,0]

while True:
    # Read switches
    for sw in range (4):
        swreading[sw] = switches[sw].value()
        if lastswreading[sw] == 1 and swreading[sw] == 0:
            # Switch goes HIGH to LOW
            print (sw, ": ", lastswreading[sw], "->", swreading[sw])
            if swvalues[sw] == 1:
                swvalues[sw] = 2
            else:
                swvalues[sw] = 1

        lastswreading[sw] = swreading[sw]

    leds = (swvalues[0] << 6) + (swvalues[1] << 4) + (swvalues[2] << 2) + swvalues[3]
    #print ("0x%02X" % leds)
    update595(leds)

    # Read pots
    for pot in range (4):
        # Scale to 0..63 from 0..1023
        potreading[pot] = 63 - int(pots.read(pot) / 16)
        if lastpotreading[pot] != potreading[pot]:
            print (pot, "->", potreading[pot])

        lastpotreading[pot] = potreading[pot]
    
    # Reading has changed
    display.text("%02d%02d%02d%02d" % (1+potreading[0], 1+potreading[1], 1+potreading[2], 1+potreading[3]))
    sleep(0.1)
