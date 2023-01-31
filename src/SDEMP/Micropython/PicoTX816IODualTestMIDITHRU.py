# TX816 IO Panel Test (with MIDI pass-THRU)
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
# Uses following two libaries:
#   mcp3008 from https://github.com/romilly/pico-code
#   max7219 from https://github.com/JennaSys/micropython-max7219
#
# IMPORTANT: The max7219 library is slightly modified for the Pico's SPI 0 bus in GPIO 16-19.
#   In the __init__ constructor, the SPI initialisation line is changed from 
#      self._spi = SPI(spi_bus, baudrate=baudrate, polarity=0, phase=0)
#   to
#      self._spi = SPI(0, baudrate=baudrate, polarity=0, phase=0, sck=Pin(18), mosi=Pin(19), miso=Pin(16))
#
#   Also there is an optional fix required in the flush() function to remove flicker on the second MAX7219.
#   Replace the following line:
#      self._write([pos + MAX7219_REG_DIGIT0, buffer[pos + (current_dev * self.scan_digits)]] + ([MAX7219_REG_NOOP, 0] * dev))
#   with:
#      self._write(([MAX7219_REG_NOOP, 0] * (self.devices-dev)) + [pos + MAX7219_REG_DIGIT0, buffer[pos + (current_dev * self.scan_digits)]] + ([MAX7219_REG_NOOP, 0] * dev))
#
# OPTIONAL: I have a version of the mcp3008 library that includes a smoothread() function
#           to average out pot readings over several scans.
#
import max7219
from mcp3008 import MCP3008
from machine import Pin, SPI
from time import sleep

# UART initialisation
uart0 = machine.UART(0,31250)
uart1 = machine.UART(1,31250)

def checkMidi():
    if (uart0.any()):
        # Just send anything received out on both UARTs
        data = uart0.read()
        uart0.write(data)
        uart1.write(data)

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
    
    for i in range(15, -1, -1):
        PINclock.value(0)
        PINdata.value((value>>i) & 1)
        PINclock.value(1)
        
    PINclock.value(0)
    PINlatch.value(1)
    PINclock.value(1)

for i in range (16):
    update595((1<<i))
    sleep(0.1)
for i in range (14,-1,-1):
    update595((1<<i))
    sleep(0.1)
update595(0)

# MCP3008 initialisation on SPI1
spi = machine.SPI(1, sck=Pin(14),mosi=Pin(15),miso=Pin(12), baudrate=100000)
cs = machine.Pin(13, machine.Pin.OUT)
pots = MCP3008(spi, cs)
sleep(1)

# MAX7219 initialisation on SPI0
display = max7219.SevenSegment(digits=16, scan_digits=8, cs=17, spi_bus=0, reverse=False)
display.brightness(4)
display.text("1122334455667788")
sleep(5)
display.clear()
sleep(1)

# Switches for TG1-4: 2, 3, 6, 7; TG5-8: 8, 9, 10, 11
switches = [
    machine.Pin(2, machine.Pin.IN, machine.Pin.PULL_UP),
    machine.Pin(3, machine.Pin.IN, machine.Pin.PULL_UP),
    machine.Pin(6, machine.Pin.IN, machine.Pin.PULL_UP),
    machine.Pin(7, machine.Pin.IN, machine.Pin.PULL_UP),
    machine.Pin(8, machine.Pin.IN, machine.Pin.PULL_UP),
    machine.Pin(9, machine.Pin.IN, machine.Pin.PULL_UP),
    machine.Pin(10, machine.Pin.IN, machine.Pin.PULL_UP),
    machine.Pin(11, machine.Pin.IN, machine.Pin.PULL_UP)
]

swreading = [1,1,1,1,1,1,1,1]  # HIGH = not pushed
lastswreading = [1,1,1,1,1,1,1,1]
swvalues = [2,2,2,2,2,2,2,2]   # Toggles between 2 and 1

potreading = [0,0,0,0,0,0,0,0]
lastpotreading = [100,0,0,0,0,0,0,0] # Dummy reading to force an update
checkpotreading = [0,0,0,0,0,0,0,0]

while True:
    checkMidi()

    # Read switches
    for sw in range (8):
        swreading[sw] = switches[sw].value()
        if lastswreading[sw] == 1 and swreading[sw] == 0:
            # Switch goes HIGH to LOW
            print (sw, ": ", lastswreading[sw], "->", swreading[sw])
            if swvalues[sw] == 1:
                swvalues[sw] = 2
            else:
                swvalues[sw] = 1

        lastswreading[sw] = swreading[sw]

    # The LEDs are actually wired up backwards, so LEDs 6,7 correspond to switch 1 on TG1, etc.
    # So as the LEDs have to be sent in the following order:
    #   1st 595: LEDs 0 to 7
    #   2nd 595: LEDs 0 to 7
    #
    # They have to be sent LSB first, and "least significant device" first.
    #
    leds = (swvalues[4] << 14) + (swvalues[5] << 12) + (swvalues[6] << 10) + (swvalues[7] << 8) + (swvalues[0] << 6) + (swvalues[1] << 4) + (swvalues[2] << 2) + swvalues[3]
    #print ("0x%02X" % leds)
    update595(leds)

    # Read pots
    changed = False
    for pot in range (8):
        # Choose one of the following lines.
        # Only use smoothread() if using the @diyelectromusic updated mcp3008 library...
        potreading[pot] = 63 - int(pots.read(pot) / 16)
        #potreading[pot] = 63 - int(pots.smoothread(pot) / 16)
        if potreading[pot] != lastpotreading[pot]:
            # Reading has changed
            changed = True
            if potreading[pot] == checkpotreading[pot]:
                # Reading is steady so act on it
                print (pot, "->", potreading[pot])

        # Sort of "debounce" the readings
        lastpotreading[pot] = checkpotreading[pot]
        checkpotreading[pot] = potreading[pot]
    
    # Reading has changed
    if (changed):
        display.text("%02d%02d%02d%02d%02d%02d%02d%02d" % (1+potreading[0], 1+potreading[1], 1+potreading[2], 1+potreading[3], 1+potreading[4], 1+potreading[5], 1+potreading[6], 1+potreading[7]))
    sleep(.1)
