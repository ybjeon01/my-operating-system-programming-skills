#ifndef __ASSEMBLYUTILITY_H__
#define __ASSEMBLYUTILITY_H__

#include "Types.h"
#include "Task.h"

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


// function that reads two bytes from port
// param:
//   port address (word): memory address where data is stored
// return:
//   two bytes value from the port I/O address
WORD kInPortWord(WORD wPort);


// function that writes two bytes to port
// param:
//   port address (word): I/O port address to write data
//   data (word): data to write
void kOutPortWord(WORD wPort, WORD wData);


// function that reads four bytes from port
// param:
//   port address (word): memory address where data is stored
// return:
//   four bytes value from the port I/O address
DWORD kInPortDword(WORD wPort);


// function that writes four bytes to port
// param:
//   port address (word): I/O port address to write data
//   data (dword): data to write
void kOutPortDword(WORD wPort, DWORD dwData);


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


/* context switch related functions */

// save current task context to task data structure
// and load next task context to run
// params:
//   pstCurrentContext: pointer where current context will be stored
//   pstNextContext: pointer to context to load
// info:
//   this looks like a function. However, it works slight different way.
//   this code must be called, but it never returns. and it preserves
//   registers instead of pushing to stack, so it can store the registers
//   of the task
void kSwitchContext(CONTEXT *pstCurrentContext, CONTEXT *pstNextContext);


/* Processor Halt related functions */

// stop processor executing instructions until signals happen
void kHlt(void);


/* MUTEX related functions */

// test a value in destination memory with compare value. and if they are
// the same value, move the value in bSource to destination memory
// params:
//   pbDestination: pointer to a value which you may want to change
//   bCompare: a value to compare with a value in pbDestination
//   bSource: a value to put if the compared values are the same
BOOL kTestAndSet(volatile BYTE *pbDestination, BYTE bCompare, BYTE bSource);


/* FPU related functions */

// initialize registers in FPU device
// info:
//   this initialization is for tasks that use FPU device, so it is necessary
//   that each task calls this function.
void kInitializeFPU(void);


// save FPU context to TCB
// params:
//   pvFPUContext: FPU context in a TCB
// info:
//    address of pvFPUContext must be aligned by 16 bytes
void kSaveFPUContext(void *pvFPUContext);


// load FPU context of a TCB to FPU device
// params:
//   pvFPUContext: FPU context in a TCB
// info:
//    address of pvFPUContext must be aligned by 16 bytes
void kLoadFPUContext(void *pvFPUContext);


// set TS bit in CR0
// info:
//   if TS bit is set, using FPU device causes exception.
//   MINT64OS is responsible to switch FPU context and clear
//   the bit
void kSetTS(void);


// clear TS bit in CR0
// info:
//   if TS bit is set, using FPU device causes exception.
//   MINT64OS is responsible to switch FPU context and clear
//   the bit
void kClearTS(void);


#endif /* __ASSEMBLYUTILITY_H__ */
