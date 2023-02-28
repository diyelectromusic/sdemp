# TX816 IO Panel
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
# This also requires the use of the SimpleMIDIDecoder.py library
# from @diyelecromusic...
#
# IMPORTANT: Everything in this code assumes:
#   Num TGs = Num switches = Num Pots = Num displays = 8... etc
#
import max7219
from mcp3008 import MCP3008
from machine import Pin, SPI, UART
from time import sleep, ticks_ms
from rp2 import PIO, StateMachine, asm_pio
import ustruct
import SimpleMIDIDecoder

# Specify the MIDI channel map.
# NB: Tone generators are only on channels 1-8
#     0 = "don't route".
#
# So TG numbers and OUT MIDI CHs are the same (1-8)
# but IN MIDI CHs still come in as 1-16.
#
# NB: This is the Lo-Fi Orchestra mapping...
MIDIRT = [
#  TG    CH
    0, #  Dummy to allow 1-based index
    1, #  1 Brass
    2, #  2 Flute
    2, #  3 Flute
    4, #  4 Reeds
    4, #  5 Reeds
    6, #  6 Guitar
    7, #  7 Bass
    5, #  9 Timpani
    8, #  8 Glock/Vibes
    0, # 10 Drums
    3, # 11 Strings
    3, # 12 Strings
    3, # 13 Strings
    3, # 14 Strings
    0, # 15 n/a
    0, # 16 n/a
    ]

MIDICOMCH = 1 # IN MIDI CH to use for "Common"
#MIDICOMCH = 16 # IN MIDI CH to use for "Common"

# -----------------------------------------------
#
#  UART initialisation
#
# -----------------------------------------------
uart0 = UART(0,31250)
uart1 = UART(1,31250)

def uart_midi_send(cmd, ch, b1, b2):
    midiActivity(ch)
    if (b2 == -1):
        uart1.write(ustruct.pack("bb",cmd+ch-1,b1))
    else:
        uart1.write(ustruct.pack("bbb",cmd+ch-1,b1,b2))


# -----------------------------------------------
#
#  MIDI routing
#
# -----------------------------------------------

MIDI_CC_VOLUME  =  7  # Channel Volume
MIDI_CC_BANKSEL = 32  # Bank Select LSB Only
MIDI_CC_DETUNE  = 94  # Effect 4 Depth maps onto master tuning for a TG

# Need to record which TGs are in common mode.
# Any in common are played via MIDICOMCH regardless of the MIDIRT settings above.
#
# NB: Index 0 = "global" indication.
MIDICOMMON = [False, False, False, False, False, False, False, False, False]

def midiSetCommon (tg):
    # Common mode
    MIDICOMMON[tg] = True
    MIDICOMMON[0] = True

def midiSetInd (tg):
    # Individual mode
    MIDICOMMON[tg] = False
    
    # If no more TGs are in common mode
    # then set idx [0] to False as a "short cut"
    MIDICOMMON[0] = (sum(MIDICOMMON[1::]) > 0)

def midiSendToCommon (cmd, ch, b1, b2):
    # If "common" routing is required on any TG
    # the idx 0 will be True
    commonRouted = False
    if MIDICOMMON[0] and ch == MIDICOMCH:
        for tg in range(1,9):
            # NB: Need to allow for special case of using
            #     a common MIDI channel that is the same as
            #     one of the TGs themselves...
            #     In this case, they are basically always
            #     in "common" mode...
            if MIDICOMMON[tg] or tg==MIDICOMCH:
                uart_midi_send(cmd, tg, b1, b2)
                commonRouted = True
    return commonRouted

def midiSendToInd (cmd, ch, b1, b2):
    # Only individually send if "common" routing
    # for this TG is not enabled.
    if not MIDICOMMON[MIDIRT[ch]]:
        uart_midi_send(cmd, MIDIRT[ch], b1, b2)


# -----------------------------------------------
#
#  MIDI Message Handling
#
# -----------------------------------------------

def doMidiNoteOn(ch,cmd,note,vel,uart):
    # Only interested in incoming MIDI on uart 0
    if uart == 0:
        if not midiSendToCommon(cmd, ch, note, vel):
            # need to locally route
            midiSendToInd(cmd, ch, note, vel)

def doMidiNoteOff(ch,cmd,note,vel,uart):
    # Only interested in incoming MIDI on uart 0
    if uart == 0:
        if not midiSendToCommon(cmd, ch, note, vel):
            midiSendToInd(cmd, ch, note, vel)

def doMidiThru(ch,cmd,d1,d2,uart):
    # Only interested in incoming MIDI on uart 0
    if uart == 0:
        # NB: The Ind/Common only applies to notes
        if (d2 == -1):
            #print(ch, MIDIRT[ch],"\tThru\t", hex(cmd>>4), "\t", d1)
            uart_midi_send(cmd, MIDIRT[ch], d1, 0)
        else:
            #print(ch, MIDIRT[ch],"\tThru\t", hex(cmd>>4), "\t", d1, "\t", d2)
            uart_midi_send(cmd, MIDIRT[ch], d1, d2)

def doMidiSysEx(data, uart):
    # Process returning SysEx messages on uart 1
    if (uart == 1):
        # Process Yamaha Specific Voice Dump SysEx to extract PC number
        # Format:
        #    F0 43 1d 00 01 1B ..data.. ck F7
        #
        #     43 = Yamaha ID
        #   011B = size (155 = b000 0001 b001 1011 = b00 0000 1001 1011 = 0x9B)
        #      d = device number (TG number)
        #     ck = checksum
        #
        # TODO: Ok, so the voice doesn't include the PC number...
        #       Q: Could I store the checksums for the 155 bytes of
        #          voice data for all the voices I have loaded and
        #          work backwards from that...?
        #
        # Something to come back to I think...
        #print(data)
        pass

# NB: This uses TG channels directly, so no additional routing required
def injectMidiPC(ch,pc):
    midiActivity(ch)
    #print(ch, ch, "\tProgramChange:\t", pc)
    uart_midi_send(0xC0, ch, pc, -1)

# NB: This uses TG channels directly, so no additional routing required
def injectMidiCC(ch,cc,dd):
    midiActivity(ch)
    #print(ch, ch, "\tControlChange:\t", cc, dd)
    uart_midi_send(0xB0, ch, cc, dd)

md = []
for i in range(2):
    # Set up one MIDI decoder per hardware UARTs
    md_t = SimpleMIDIDecoder.SimpleMIDIDecoder(i)
    md_t.cbNoteOn (doMidiNoteOn)
    md_t.cbNoteOff (doMidiNoteOff)
    md_t.cbThru (doMidiThru)
    md_t.cbSysEx (doMidiSysEx)
    md.append(md_t)
    
# -----------------------------------------------
#
#  595 Shift Register for LEDS
#
# -----------------------------------------------
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

# LED test pattern
for i in range (16):
    update595((1<<i))
    sleep(0.1)
for i in range (14,-1,-1):
    update595((1<<i))
    sleep(0.1)
update595(0)

# -----------------------------------------------
#
#  MCP3008 initialisation on SPI1 for ALG
#  MAX7219 initialisation on SPI0 for 7-Segment Display
#
# -----------------------------------------------

#  MCP3008 initialisation on SPI1
spi = machine.SPI(1, sck=Pin(14),mosi=Pin(15),miso=Pin(12), baudrate=100000)
cs = machine.Pin(13, machine.Pin.OUT)
pots = MCP3008(spi, cs)
sleep(1)

# MAX7219 initialisation on SPI0
display = max7219.SevenSegment(digits=16, scan_digits=8, cs=17, spi_bus=0, reverse=False)
display.brightness(4)
display.text("1122334455667788")
sleep(1)
display.clear()
sleep(1)

# -----------------------------------------------
#
#  Switch handling
#
# -----------------------------------------------

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

midiactivity = [3,3,3,3,3,3,3,3] # Toggles bewteen 3 and 0
swreading = [1,1,1,1,1,1,1,1]  # HIGH = not pushed
lastswreading = [1,1,1,1,1,1,1,1]
longpresssw = [0,0,0,0,0,0,0,0]
swvalues = [2,2,2,2,2,2,2,2]   # Toggles between 2 and 1
swtimes = [0,0,0,0,0,0,0,0]
sw=0
SWSHORTPRESS = 300 # Less than this in mS is a short press
SWLONGPRESS = 800  # Greater than this in mS is a long press
def scanSwitches():
    global sw, swvalues, lastswreading, swreading
    # Read switches
    sw = sw+1
    if sw == 8:
        # Split the LED display into building the value to output
        scanLeds(1)
        return
    elif sw == 9:
        # ... then output the LED sequence
        scanLeds(2)
        return
    elif sw > 9:
        sw = 0

    swreading[sw] = switches[sw].value()
    if lastswreading[sw] == 1 and swreading[sw] == 0:
        # Switch goes HIGH to LOW - i.e. pressed
        # Start switch "clock"
        swtimes[sw] = ticks_ms()
        longpresssw[sw] = 0
    elif lastswreading[sw] == 0 and swreading[sw] == 0:
        # Switch LOW and staying LOW - i.e. stays pressed
        swtime = ticks_ms() - swtimes[sw]
        if swtime > SWLONGPRESS and longpresssw[sw] == 0:
            #print (sw, "-> Long Press")
            longpresssw[sw] = 1
            #changeMode(sw)
            changeIndCommon(sw)
    elif lastswreading[sw] == 0 and swreading[sw] == 1:
        # Switch goes LOW to HIGH - i.e. released
        swtime = ticks_ms() - swtimes[sw]
        #print (sw, ": ", lastswreading[sw], "->", swreading[sw], "time: ", swtime)
        if swtime < SWSHORTPRESS:
            changeMode(sw)
            #changeIndCommon(sw)
        else:
            # Long Press already handled in the LOW staying LOW case
            pass

    lastswreading[sw] = swreading[sw]

def changeIndCommon(sw):
    global swvalues
    if swvalues[sw] == 1:
        midiSetInd(sw+1) # Set to "Individual"
        swvalues[sw] = 2
    else:
        midiSetCommon(sw+1) # Set to "Common"
        swvalues[sw] = 1

# -----------------------------------------------
#
#  LED Display Handling
#
# -----------------------------------------------

miditicks = [0,0,0,0,0,0,0,0]
MIDITICK = 30
flashing = [0,0,0,0,0,0,0,0]
LEDTICK = 400
ledtick = LEDTICK + ticks_ms() 
def flashLeds():
    global ledtick, flashing
    newtime = ticks_ms()
    if newtime > ledtick:
        # Toggles between 3 and 0
        if flashing[0] == 0:
            flashing = [3,3,3,3,3,3,3,3]
        else:
            flashing = [0,0,0,0,0,0,0,0]
        ledtick = LEDTICK + ticks_ms()

def midiActivity(ch):
    global midiactivity
    if ch > 0 and ch <=8:
        midiactivity[ch-1] = 0 # On
        miditicks[ch-1] = MIDITICK + ticks_ms()

def midiActivityTO():
    # See if we need to "time out" the MIDI activity indicator
    mstime = ticks_ms()
    for i in range(8):
        if miditicks[i] < mstime:
            midiactivity[i] = 3 # Off

leds = 0
def scanLeds(scantype):
    global swvalues, midiactivity, leds
    midiActivityTO()
    flashLeds()
    if scantype == 2:
        update595(leds)
    else:
        lval = [0,0,0,0,0,0,0,0]
        for led in range(len(lval)):
            if uimode[led] == 0:
                lval[led] = swvalues[led] & midiactivity[led]
            else:
                lval[led] = swvalues[led] & midiactivity[led] & flashing[led]

        # The LEDs are actually wired up backwards, so LEDs 6,7 correspond to switch 1 on TG1, etc.
        # So as the LEDs have to be sent in the following order:
        #   1st 595: LEDs 0 to 7
        #   2nd 595: LEDs 0 to 7
        #
        # They have to be sent LSB first, and "least significant device" first.
        #
        leds = (lval[4] << 14) + (lval[5] << 12) + (lval[6] << 10) + (lval[7] << 8) +\
               (lval[0] << 6)  + (lval[1] << 4)  + (lval[2] << 2)  + lval[3]
        #print ("0x%02X" % leds)

# -----------------------------------------------
#
#  UI Mode Handling
#
# -----------------------------------------------

# UI Mode for each TG
#  0 = Normal
#  1 = Volume
#  2 = Bank Selection
#  3 = Detune
NUMMODES = 4
uimode = [0,0,0,0,0,0,0,0]
dots = [
    False, False, False, False,
    False, False, False, False,
    False, False, False, False,
    False, False, False, False
    ]

def changeMode(sw):
    global uimode, displayupdated, dots, pot
    uimode[sw] = uimode[sw] + 1
    if uimode[sw] == 1:
        # Volume Mode
        dots[sw*2] = False
        dots[1+sw*2] = False
    elif uimode[sw] == 2:
        # Bank Select Mode
        dots[sw*2] = False
        dots[1+sw*2] = True
    elif uimode[sw] == 3:
        # Detune Mode
        dots[sw*2] = True
        dots[1+sw*2] = False
    else:
        # Normal Mode
        uimode[sw] = 0
        dots[sw*2] = False
        dots[1+sw*2] = False
    #print (sw, "Mode=", uimode[sw])
    # Force an update of the display
    displayupdated = True
    pot = 7

# Note: potval comes in as "MIDI scale" so 0..127

midiVoice = [0,0,0,0,0,0,0,0]
def setMidiVoice (tg, potval):
    global midiVoice
    # Scale to 0..31
    midiVoice[tg] = int(potval / 4)

def getMidiVoice (tg):
    # MIDI voices are 0..31
    return midiVoice[tg]

def getMidiVoiceDisplay (tg):
    # Voice is 0..31 but display shows 1..32
    return 1+midiVoice[tg]

midiVolume = [127,127,127,127,127,127,127,127]
def setMidiVolume (tg, potval):
    global midiVolume
    # Use directly as 0..127
    midiVolume[tg] = potval

def getMidiVolume (tg):
    # MIDI volume is 0..127
    return midiVolume[tg]

def getMidiVolumeDisplay (tg):
    # Volume is 0..127, but display only shows 1..64
    return 1+int(midiVolume[tg]/2)

midiBank = [0,0,0,0,0,0,0,0]
def setMidiBank (tg, potval):
    global midiBank
    # Scale to 0..7
    midiBank[tg] = int (potval / 16)

def getMidiBank (tg):
    # MIDI bank numbers are 0..7
    return midiBank[tg]

def getMidiBankDisplay (tg):
    # Bank is 0..7 but display adds a "b" and makes it 1..8
    return 0xb1 + midiBank[tg]

# 0=None; 1<64<127 maps onto -99 to 99
# But only a subset is supported here for slight
# detuning effects...
# Supports +/- 15
midiDetune = [0,0,0,0,0,0,0,0]
def setMidiDetune (tg, potval):
    global midiDetune
    # Scale to 0..31 then offset
    midiDetune[tg] = int (potval / 4) - 15
    if midiDetune[tg] > 15:
        midiDetune[tg] = 15

def getMidiDetune (tg):
    # MIDI detune is 0 for "no effect"
    # or 1<64<127 for negative<disabled<positive
    if midiDetune[tg] == 0:
        return 0
    else:
        return 64+midiDetune[tg]

def getMidiDetuneDisplay (tg):
    # Display is split:
    #   Digit 0 - 0 to F negative detune
    #   Digit 1 - 0 to F positive detune
    #
    if midiDetune[tg] <= -1 and midiDetune[tg] >= -15:
        return (0-midiDetune[tg]) * 16
    elif midiDetune[tg] >= 1 and midiDetune[tg] <= 15:
        return midiDetune[tg]
    else:
        return 0

# -----------------------------------------------
#
#  Pot Handling
#
# -----------------------------------------------

potreading = [0,0,0,0,0,0,0,0]
lastpotreading = [9999,9999,9999,9999,9999,9999,9999,9999] # Force pot update
checkpotreading = [0,0,0,0,0,0,0,0]
checkpotreading2 = [0,0,0,0,0,0,0,0]
checkpotreading3 = [0,0,0,0,0,0,0,0]
displaytext = ""
pot = -1
displayupdated = False

def potDisplay(pot):
    # Some modes return a hex display, some decimal
    if uimode[pot] == 1:
        return "%02d" % getMidiVolumeDisplay(pot)
    elif uimode[pot] == 2:
        return "%02x" % getMidiBankDisplay(pot)
    elif uimode[pot] == 3:
        return "%02x" % getMidiDetuneDisplay(pot)
    else:
        return "%02d" % getMidiVoiceDisplay(pot)

def scanPots():
    global pot, displayupdated, potreading, lastpotreading, checkpotreading, checkpotreading2, checkreading3, displaytext
    # Read pots
    pot = pot+1
    if pot == 8:
        # Update display if required
        if (displayupdated):
            displaytext = potDisplay(0)+potDisplay(1)+potDisplay(2)+potDisplay(3)+\
                          potDisplay(4)+potDisplay(5)+potDisplay(6)+potDisplay(7)
        return
    elif pot == 9:
        if (displayupdated):
            display.text(displaytext, dots)
        return
    elif pot > 9:
        #print (potreading, potdivisor)
        # Reset
        pot = 0
        displayupdated = False

    # NB: pots are reversed in the circuit...
    # Also: pots scaled to 0..127 by default
    potreading[pot] = 127 - int(pots.smoothread(pot)/8)

    if potreading[pot] == checkpotreading[pot] and\
       potreading[pot] == checkpotreading2[pot] and\
       potreading[pot] == checkpotreading3[pot]:
        # Reading is steady...
        if potreading[pot] != lastpotreading[pot]:
            # Reading has changed and is steady so act on it
            displayupdated = True
            lastpotreading[pot] = potreading[pot]

            # Action depends on uimode[]
            #print (pot, "->", potreading[pot])

            if uimode[pot] == 1:
                # Volume mode:
                # Pot (0-7) -> MIDI TG/CH (1-8)
                # Value (0-1023) -> MIDI data (0-127)
                setMidiVolume(pot, potreading[pot])
                injectMidiCC(pot+1, MIDI_CC_VOLUME, getMidiVolume(pot))
            elif uimode[pot] == 2:
                # Bank Select:
                # Pot (0-7) -> MIDI TG/CH (1-8)
                # Value (0-1023) -> MIDI data (0-7)
                setMidiBank(pot, potreading[pot])
                injectMidiCC(pot+1, MIDI_CC_BANKSEL, getMidiBank(pot))
            elif uimode[pot] == 3:
                # Bank Select:
                # Pot (0-7) -> MIDI TG/CH (1-8)
                # Value (0-1023) -> Detune (-99 to 99) -> MIDI data (0-127)
                setMidiDetune(pot, potreading[pot])
                injectMidiCC(pot+1, MIDI_CC_DETUNE, getMidiDetune(pot))
            else:
                # Normal mode: MIDI Voice/Program Change
                # Pot (0-7) -> MIDI TG/CH (1-8)
                # Value (0-1023) -> MIDI data (0-31)
                setMidiVoice(pot, potreading[pot])
                injectMidiPC(pot+1, getMidiVoice(pot))

    # Sort of "debounce" the readings
    checkpotreading3[pot] = checkpotreading2[pot]
    checkpotreading2[pot] = checkpotreading[pot]
    checkpotreading[pot] = potreading[pot]

# -----------------------------------------------
#
#  Main Loop!
#
# -----------------------------------------------

#  NB: LED and 7Seg Displays are sub-scheduled into the
#      switch and pot scanning to split up the overheads
#      over several scans and seperate out the sensing
#      from the displaying.

toggle = False
while True:
    if (uart0.any()):
        md[0].read(uart0.read(1)[0])
    if (uart1.any()):
        md[1].read(uart1.read(1)[0])

    # Alternate scanning the switches (and outputing LEDS) and pots (and updating the 7-segments)
    if toggle:
        scanSwitches()
        toggle = False
    else:
        scanPots()
        toggle = True
