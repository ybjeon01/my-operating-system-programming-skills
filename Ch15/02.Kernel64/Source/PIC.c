#include "PIC.h"
#include "AssemblyUtility.h"

// initialize master and slave PICs for MINT64OS
// info:
//   ICW1: edge trigger, dual PICs, ICW4
//   ICW2:
//     master: 0x20
//     slave:  0x28
//   ICW3: pin number 2
//   ICW4: no automatic EOI, 8006/88 mode
void kInitializePIC(void) {
    /* Initialize Master PIC */

    // ICW1 command to command port (general configuration):
    //   edge trigger(keyboard, mouse), dual PICs, ICW4(8086 mode)
    kOutPortByte(PIC_MASTER_PORT1, 0x11);

    // ICW2 command (interrupt vector start index):
    //   0x20 (IDT 32nd)
    // info:
    //   first 3 bits must be 0, so value must be multiply of 8
    kOutPortByte(PIC_MASTER_PORT2, PIC_IRQSTARTVECTOR);
    
    // ICW3 command (master's pin where slave PIC is connected):
    //   0x04 (second bit)
    kOutPortByte(PIC_MASTER_PORT2, 0x04);

    // ICW4 command (extended configuration):
    //   uPM (bit 0): 1 for 8086/88 processors
    //   AEOI (bit 1): 0 for manual EOI
    kOutPortByte(PIC_MASTER_PORT2, 0x01);

    
    /* Initialize Slave PIC */
    
    // ICW1 command (general configuration):
    //   same as master
    kOutPortByte(PIC_SLAVE_PORT1, 0x11);

    // ICW2 command (interrupt vector start index):
    //   0x28 (IDT 40nd)
    // info:
    //   same as master
    kOutPortByte(PIC_SLAVE_PORT2, PIC_IRQSTARTVECTOR + 8);

    // ICW3 command (master's pin where slave PIC is connected):
    //   0x02 (integer)
    // info:
    //   Unlike master, the number must be represented as integer in slave
    kOutPortByte(PIC_SLAVE_PORT2, 0x02);

    // ICW4 command (extended configuration):
    //   same as master
    kOutPortByte(PIC_SLAVE_PORT2, 0x01);
}


// set mask register so unwanted interrupts are not sent to CPU and
// wanted interrupts are sent to CPU
// params:
//   wIRQBitMask: IRQ mask for IRQ 0 to IRQ 15
// info:
//   setting bit 2 masks all interrupts from IRQ 8 to 15
void kMaskPICInterrupt(WORD wIRQBitMask) {
    // set IMR to master PIC
	// OCW1 command, mask IRQ 0 ~ IRQ 7
	kOutPortByte(PIC_MASTER_PORT2, (BYTE) wIRQBitMask);

	// set IMR to slave PIC
	// OCW1 command, mask IRQ 8 ~ IRQ 15
	kOutPortByte(PIC_SLAVE_PORT2, (BYTE) ( wIRQBitMask >> 8 ));
}


// send EOI (OCW2 command) to in-service interrupt
// params:
//   iIRQNumber:
// info:
//   this function does not send EOI with IRQ number to PIC. The number decides
//   whether it should send EOI to two PICs or just one PIC
void kSendEOIToPIC(int iIRQNumber) {
	// OCW2 command:
    //   EOI (bit 5) = 1
	kOutPortByte(PIC_MASTER_PORT1, 0x20);

    // if IRQ is bigger 7, OCW2 commands must be sent to both master and
    // slave PICs
	if (iIRQNumber > 7) {
		// OCW2 command: 
        //   EOI bit (bit 5) = 1
		kOutPortByte(PIC_SLAVE_PORT1, 0x20);
	}
}