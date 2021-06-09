#ifndef __PIC_H__
#define __PIC_H__

#include "Types.h"


/* PIC I/O port */

#define PIC_MASTER_PORT1			0x20 // master command port
#define PIC_MASTER_PORT2			0x21 // master data port

#define PIC_SLAVE_PORT1				0xA0 // slave command port
#define PIC_SLAVE_PORT2				0xA1 // slave data port


/* MINT64OS IDT descriptor start index for handling IRQ */

#define PIC_IRQSTARTVECTOR			0x20 // 32


/* PIC related functions */

// initialize master and slave PICs for MINT64OS
// info:
//   ICW1: edge trigger, dual PICs, ICW4
//   ICW2:
//     master: 0x20
//     slave:  0x28
//   ICW3: pin number 2
//   ICW4: no automatic EOI, 8006/88 mode
void kInitializePIC(void);

// set mask register so unwanted interrupts are not sent to CPU and
// wanted interrupts are sent to CPU
// params:
//   wIRQBitMask: IRQ mask for IRQ 0 to IRQ 15
// info:
//   setting bit 2 masks all interrupts from IRQ 8 to 15
void kMaskPICInterrupt(WORD wIRQBitMask);

// send EOI (OCW2 command) to in-service interrupt
// params:
//   iIRQNumber:
// info:
//   this function does not send EOI with IRQ number to PIC. The number decides
//   whether it should send EOI to two PICs or just one PIC
void kSendEOIToPIC(int iIRQNumber);

#endif /* __PIC_H__ */