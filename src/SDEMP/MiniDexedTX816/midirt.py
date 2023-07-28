# Specify the MIDI channel map: maps input MIDI channels to TGs.
# NB: Tone generators are numbered 1-8
#     0 = "don't route".
#
# TG numbers are mapped onto MIDI OUT channels seperately.
# Note: MIDI IN channels are 1-16.
#
MIDIRT = [
#  TG     MIDI CH
    0, #  Dummy to allow 1-based index
    0, #  1 - used for "common" mode
    1, #  2 
    2, #  3 
    3, #  4 
    4, #  5 
    5, #  6 
    6, #  7 
    7, #  8 
    8, #  9 
    0, # 10 
    0, # 11 
    0, # 12 
    0, # 13 
    0, # 14 
    0, # 15 
    0, # 16 
    ]

MIDICOMCH = 1 # IN MIDI CH to use for "Common"
