# Simple ROM hexdump based on the class found here:
# https://gist.github.com/NeatMonster/c06c61ba4114a2b31418a364341c26c0
#
class hexdump:
    def __init__(self, buf, off=0):
        self.buf = buf
        self.off = off

    def __iter__(self):
        for i in range(0, len(self.buf), 16):
            bs = bytearray(self.buf[i : i + 16])
            line = "{:05x}:   {:23}   {:23}\t{:16}".format(
                self.off + i,
                " ".join(("{:02x}".format(x) for x in bs[:8])),
                " ".join(("{:02x}".format(x) for x in bs[8:])),
                "".join((chr(x) if 32 <= x < 127 else "." for x in bs)),
            )
            yield line

    def __str__(self):
        return "\n".join(self)

    def __repr__(self):
        return "\n".join(self)

addr=0
with open ('rom.bin' ,'rb') as f:
    for chunk in iter(lambda: f.read(16), b''):
        print(hexdump(chunk, addr))
        addr=addr+16
