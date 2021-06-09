#ifndef __ASSEMBLYUTILITY_H__
#define __ASSEMBLYUTILITY_H__

#include "Types.h"


/* I/O port related functions */

// function that reads one byte from port
// param:
//   port address (word): memory address where data is stored
// return:
//   a byte value from the port I/O address
BYTE kInPortByte(WORD wPort);

// function that writes one byte to port
// param:
//   port address (word): I/O port address to write data
//   data (byte): data to write
void kOutPortByte(WORD wPort, BYTE bData);


/* GDT, IDT, and TSS related functions */

// function that loads GDTR address to register
// param: 
//   qwGDTRAddress: address of GDTR
void kLoadGDTR(QWORD qwGDTRAddress);

// function that loads TSS descriptor offset to TR register
// param:
//   wTSSSegmentOffset: TSS descriptor offset in GDT
void kLoadTR(WORD wTSSSegmentOffset);

// function that loads IDTR address to register
// param:
//   qwIDTRAddress: address of IDTR
void kLoadIDTR(QWORD qwIDTRAddress);


/* interrupt related functions */

// function that activates interrupt
void kEnableInterrupt(void);

// function that deactivates interrupt
void kDisableInterrupt(void);

// function that returns RFLAGS
// return:
//   Rflags of 64 bit size
QWORD kReadRFLAGS(void);


/* time stamp counter related functions */

// function that returns time stamp counter
// return:
//   time stamp counter
QWORD kReadTSC(void);


#endif /* __ASSEMBLYUTILITY_H__ */
