/*
      MIT License
      
      Copyright (c) 2025 emalliab.wordpress.com (Kevin)
      
      Permission is hereby granted, free of charge, to any person obtaining a copy of
      this software and associated documentation files (the "Software"), to deal in
      the Software without restriction, including without limitation the rights to
      use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
      the Software, and to permit persons to whom the Software is furnished to do so,
      subject to the following conditions:
      
      The above copyright notice and this permission notice shall be included in all
      copies or substantial portions of the Software.
      
      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
      FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
      COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHERIN
      AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
      WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <TimerOne.h>

void hdlrHelp(int idx, char* pParam);
void hdlrList(int idx, char* pParam);
void hdlrGoto(int idx, char* pParam);
void hdlrClear(int idx, char* pParam);
void hdlrRestore(int idx, char* pParam);
void hdlrAddress(int idx, char* pParam);
void hdlrOpcodes(int idx, char* pParam);
void hdlrAsm (int idx, char* pParam);
void disassembleROM(void);

#define CMD_BUFFER 12
char cmdInput[CMD_BUFFER+1];
char cmdSaved[CMD_BUFFER+1];
int  cmdIdx;

#define MAX_4LINES 16 // 1..MAX
#define MAX_5LINES 32 // 1..MAX
uint8_t line;

bool show5bit = false;

#define MAXROMSIZE 32

uint8_t RAM[MAXROMSIZE];
uint8_t ROM[MAXROMSIZE] = {
    0xA1, 0x01, 0xA2, 0x51,
    0xA4, 0x01, 0xA8, 0x51,
    0xA4, 0x01, 0xA2, 0x51,
    0xF0, 0x00, 0x00, 0x00,
    0xA1, 0xA2, 0xA4, 0xA8,
    0xA8, 0xA4, 0xA2, 0xA1,
    0xA1, 0xA2, 0xA4, 0xA8,
    0xA8, 0xA4, 0xA2, 0xA1,
};

typedef void (*hdlr_t)(int idx, char *param);
#define FSH(x) ((const __FlashStringHelper *)x)

struct cmd_t {
  char    cmd[CMD_BUFFER+1];
  hdlr_t  pFn;
  uint8_t idx;
};

#define NUM_CMDS (16+7)
const cmd_t PROGMEM cmdTable[NUM_CMDS] = {
  // Assembly commands - must be first
  {"ADDA", hdlrAsm, 0},
  {"MOVAB", hdlrAsm, 1},
  {"INA", hdlrAsm, 2},
  {"MOVA", hdlrAsm, 3},
  {"MOVBA", hdlrAsm, 4},
  {"ADDB", hdlrAsm, 5},
  {"INB", hdlrAsm, 6},
  {"MOVB", hdlrAsm, 7},
  {"OUTB", hdlrAsm, 8},
  {"OUT2B", hdlrAsm, 9},
  {"OUT", hdlrAsm, 10},
  {"OUT2", hdlrAsm, 11},
  {"JNCB", hdlrAsm, 12},
  {"JMPB", hdlrAsm, 13},
  {"JNC", hdlrAsm, 14},
  {"JMP", hdlrAsm, 15},

  // Other commands
  {"H", hdlrHelp, 0},
  {"L", hdlrList, 0},
  {"G", hdlrGoto, 0},
  {"C", hdlrClear, 0},
  {"R", hdlrRestore, 0},
  {"A", hdlrAddress, 0},
  {"O", hdlrOpcodes, 0},
};

#define HASIM(op) (op==0||op==3||op==5||op==7||op>9)

uint16_t str2num (char *pStr) {
  if (pStr == nullptr) return 0;

  uint16_t retval = 0;
  int idx = 0;
  if ((*pStr == '0') && (*(pStr+1) == 'x')) {
    pStr += 2;
    // process as HEX
    while ((*pStr != '\0') && (idx < CMD_BUFFER)) {
      retval = retval * 16;
      if ((*pStr >= '0') && (*pStr <= '9')) {
        retval += *pStr - '0';
      } else if ((*pStr >= 'A') && (*pStr <= 'F')) {
        retval += *pStr - 'A' + 10;
      } else if ((*pStr >= 'a') && (*pStr <= 'f')) {
        retval += *pStr - 'a' + 10;
      } else {
        // Stop at first non-hex char
        retval = 0;
        break;
      }
      pStr++;
      idx++;
    }
  } else if (*pStr == 'b') {
    pStr++;
    // process as BIN
    while ((*pStr != '\0') && (idx < CMD_BUFFER)) {
      retval = retval * 2;
      if (*pStr == '1') {
        retval = retval + 1;
      } else if (*pStr == '0') {
        // valid, but ignore
      } else {
        // Stop at first non-bin char
        retval = 0;
        break;
      }
      pStr++;
      idx++;
    }
  } else {
    // process as DEC
    while ((*pStr != '\0') && (idx < CMD_BUFFER)) {
      retval = retval * 10;
      if ((*pStr >= '0') && (*pStr <= '9')) {
        retval += *pStr - '0';
      } else {
        // Stop at first non-dec char
        retval = 0;
        break;
      }
      pStr++;
      idx++;
    }
  }

  if (idx == CMD_BUFFER) {
    return 0;
  } else {
    return retval;
  }
}

bool cmdRunner (void) {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      strcpy(cmdSaved, cmdInput);
      cmdIdx = 0;
      return true;
    }
    else if (cmdIdx < CMD_BUFFER-1) {
      cmdInput[cmdIdx++] = c;
      cmdInput[cmdIdx] = '\0';
    }
  }

  return false;
}

char nullp[2] = "0";
bool cmdProcess (void) {
  char *pIdx = cmdSaved;
  char *pCmd = pIdx;
  char *pParam = nullp;

  while (*pIdx != '\0') {
    pCmd = pIdx;
    while ((*pIdx != ' ') && (*pIdx != '\n')) {
      pIdx++;
    }

    if (*pIdx == ' ') {
      *pIdx = '\0';
      pIdx++;
      pParam = pIdx;
    }

    int cidx=0;
    for (cidx=0; cidx<NUM_CMDS; cidx++) {
      if (strcasecmp_P(pCmd, cmdTable[cidx].cmd) == 0) {
        // Found
        break;
      }
    }

    if (cidx == NUM_CMDS) {
      // Not found
      return false;
    } else {
      hdlr_t hdlr;
      memcpy_P(&hdlr, &cmdTable[cidx].pFn, sizeof(hdlr_t));
      uint8_t fnidx;
      memcpy_P(&fnidx, &cmdTable[cidx].idx, sizeof(uint8_t));
      if (hdlr != nullptr) {
        hdlr(fnidx, pParam);
        return true;
      }
    }
  }
  return false;
}

void printbin (uint8_t val, int size) {
  if ((size < 4) || (size > 8)) {
    return;
  }
  for (int i=size-1; i>=0; i--) {
    if (val & (1<<i)) {
      Serial.print("1");
    } else {
      Serial.print("0");
    }
  }
}

void printhex (uint8_t val, int size) {
  if ((val < 16) & (size == 2)) {
    Serial.print("0");
  }
  Serial.print(val,HEX);
}

void printins (uint8_t ins) {
  uint8_t cmd = ins >> 4;
  uint8_t im = ins & 0x0F;

  Serial.print(FSH(cmdTable[cmd].cmd));
  if (HASIM(cmd)) {
    Serial.print(" b");
    printbin(im,4);
  } else {
    Serial.print("      ");
  }
}

void printop (uint8_t op) {
  uint8_t cmd = op >> 4;
  uint8_t im = op & 0x0F;

  Serial.print("b");
  printbin(cmd,4);
  Serial.print(" ");
  printbin(im,4);
  Serial.print("\t0x");
  printhex(op,2);
}

void printline (uint8_t ln) {
  Serial.print("b");
  if (show5bit) {
    printbin(ln, 5);
  } else {
    printbin(ln, 4);
  }
  Serial.print(" [");
  printhex(ln,1);
  Serial.print("]");
}

void currentLine (void) {
  Serial.print("Current line: ");
  printline(line);
  Serial.print("\n");
}

void hdlrHelp(int idx, char *pParam) {
  Serial.print("Help\n----\n");
  Serial.println("H: Help");
  Serial.println("L: List");
  Serial.println("G: Goto");
  Serial.println("C: Clear");
  Serial.println("R: Restore");
  Serial.println("A: Addr Mode");
  Serial.println("O: Opcodes");
  Serial.println("OpCode");
  Serial.println("OpCode im");
  Serial.print("\n");
  currentLine();
}

void hdlrList(int idx, char *pParam) {
  Serial.print("RAM Disassembly\n\n");
  disassemble();
  currentLine();
}

void hdlrGoto(int idx, char *pParam) {
  uint16_t newline = str2num(pParam);
  int maxlines = (show5bit == true) ? MAX_5LINES : MAX_4LINES;

  if (newline < maxlines) {
    Serial.print("Goto line ");
    Serial.println(newline);
    line = newline;
  } else {
    Serial.print("Line out of range (0..");
    Serial.print(maxlines-1);
    Serial.println(")");
  }
  currentLine();
}

void hdlrClear(int idx, char* pParam) {
  Serial.print("Clearing RAM ...");
  for (int i=0; i<MAXROMSIZE; i++) {
    RAM[i] = 0;
  }
  Serial.println(" Done");
  line = 0;
}

void hdlrRestore(int idx, char* pParam) {
  Serial.print("Restoring RAM from ROM ...");
  for (int i=0; i<MAXROMSIZE; i++) {
    RAM[i] = ROM[i];
  }
  Serial.println(" Done");
  line = 0;
}

void hdlrAddress(int idx, char *pParam) {
  if (show5bit) {
    Serial.print("Address Mode = 4 bit\n\n");
    show5bit = false;
  } else {
    Serial.print("Address Mode = 5 bit\n\n");
    show5bit = true;
  }
}

void hdlrOpcodes(int idx, char* pParam) {
  Serial.println("Supported OpCodes:");
  for (int i=0; i<16; i++) {
    Serial.print("  b");
    printbin(i,4);
    if (HASIM(i)) {
      Serial.print(" data\t");
    } else {
      Serial.print(" 0000\t");
    }
    Serial.print(FSH(cmdTable[i].cmd));
    if (HASIM(i)) {
      Serial.print(" im");
    }
    Serial.print("\n");
  }
}

void hdlrAsm (int idx, char* pParam) {
  Serial.print("Assemble:\n");
  printline(line);
  Serial.print(" ");

  uint16_t im = str2num(pParam);
  if (im >= 16) {
    // ignore
    Serial.println("Invalid");
    return;
  }

  // opcode + intermediate value
  RAM[line] = (idx<<4) + im;

  printins (RAM[line]);
  Serial.print("\t");
  printop (RAM[line]);
  Serial.print("\n");

  line++;
  if (show5bit) {
    line = line % MAX_5LINES;
  } else {
    line = line % MAX_4LINES;
  }
  currentLine();
}

void scanLines (void) {
  uint8_t addr;
  if (show5bit) {
    // 5 bit address mode
    addr = PINC & 0x1F;
  } else {
    // 4 bit address mode
    addr = PINC & 0x0F;
  }
  PORTB = (PORTB & ~(0x0F)) | (RAM[addr] & 0x0F);
  PORTD = (PORTD & ~(0xF0)) | (RAM[addr] & 0xF0);
}

void disassemble() {
  for (int i=0; i<MAXROMSIZE/2; i++) {
    uint8_t cmd = RAM[i] >> 4;
    uint8_t im  = RAM[i] & 0x0F;
    printline(i);
    Serial.print(": ");

    printins(RAM[i]);
    Serial.print("\t");
    printop(RAM[i]);
  
    if (show5bit) {
      Serial.print("\t");
      int i2 = i + MAXROMSIZE/2;
      cmd = RAM[i2] >> 4;
      im  = RAM[i2] & 0x0F;
      printline(i2);
      Serial.print(": ");

      printins(RAM[i2]);
      Serial.print("\t");
      printop(RAM[i2]);
    }

    Serial.print("\n");
  }
}

void setup() {
  DDRB |= 0x0F;
  DDRD |= 0xF0;

  hdlrRestore(0, nullptr);

  Timer1.initialize(1000);  // 1mS
  Timer1.attachInterrupt(scanLines);

  Serial.begin(9600);
  hdlrHelp(0,nullptr);

  line = 0;
}

void loop() {
  if (cmdRunner()) {
    Serial.print("\n");
    if (!cmdProcess()) {
      Serial.println("*** Command Not Recognised ***\n");
      hdlrHelp(0,nullptr);
    }
  }
}
