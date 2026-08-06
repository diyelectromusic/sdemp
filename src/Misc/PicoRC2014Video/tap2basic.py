# SPDX-License-Identifier: MIT.
# With the added proviso that this code MUST NOT be used for training of AI systems.
#
# Copyright (c) 2026 Kevin (emalliab)
#
# Based on: tapextract.py - Extract the binary data from a TAP file
#
import struct

tapfile = "sabre.tap"


def output_basic(dbytes):
    print ("--------------------------------")
    print ("10 for a=0 TO 767")
    print ("20 poke 22528+a,56")
    print ("30 next a")
    print ("40 FOR a=0 TO 6912")
    print ("50 PRINT 16384+a")
    print ("60 READ n: POKE 16384+a,n")
    print ("70 NEXT a")

    line_number = 100
    for offset in range (0, len(dbytes), 16):
        chunk = dbytes[offset:offset + 16]
        values = ",".join(str(b) for b in chunk)
        print(f"{line_number} DATA {values}")
        line_number += 5

    print ("--------------------------------")

 
def read_headerless_data(dbytes):
    return dbytes[1:-1]  # strip the flags and checksum


def read_tap_block(dfile):
    # 2 bytes data length
    dbytes = dfile.read(2)
    if not dbytes:
        return None
    if len(dbytes) != 2:
        return None
    dlen = struct.unpack("<H", dbytes)[0]
    if (dlen != 0):
        print ("Reading %d" % dlen)
    return dfile.read(dlen)

blocknum = 0

print ("Reading %s" % tapfile)
with open(tapfile, "rb") as infile:
    try:
        while True:
            data_bytes = read_tap_block(infile)
            if data_bytes is None:
                print("Error: Block %d not found" % blocknum)
                break
            if (len(data_bytes) != 0):
                print (data_bytes.hex())
            if len(data_bytes) == (6144+768+2):
                print("Reading Data block %d" % blocknum)
                # probably a screen
                out_bytes = read_headerless_data(data_bytes)
                output_basic(out_bytes)
                break
            blocknum = blocknum + 1
    except:
        traceback.print_exc()
