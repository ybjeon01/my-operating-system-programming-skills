#include "Descriptor.h"
#include "Utility.h"
#include "ISR.h"

// public function that initializes GDT, its entries, and TSS
//   info:
//     In Mint64OS, page table, GDTR, GDT, TSS, IDTR, IDT are located
//     consecutively from 1MB
void kInitializeGDTTableAndTSS(void) {
    GDTR *pstGDTR;
    GDTENTRY8 *pstEntry;
    TSSSEGMENT *pstTSS;

    // 1MB + 264KB (page table)
    pstGDTR = (GDTR *) GDTR_STARTADDRESS;
    // 1MB + 264KB (page table) + 16 bytes (GDTR + padding)
    pstEntry = (GDTENTRY8 *) (GDTR_STARTADDRESS + sizeof(GDTR)); 
    // last accessible addr of table
    pstGDTR->wLimit = GDT_TABLESIZE - 1;
    pstGDTR->qwBaseAddress = (QWORD) pstEntry;

    // 1MB + 264KB (page table) + 16 bytes (GDTR + padding) + 40 bytes (GDT)
    pstTSS = (TSSSEGMENT *) ((QWORD) pstEntry + GDT_TABLESIZE);


    /* initialize GDT */

    // GDT NULL descriptor
    kSetGDTEntry8(&(pstEntry[0]), 0, 0, 0, 0, 0);

    // GDT 64 bits kernel code segment
    kSetGDTEntry8(
        &(pstEntry[1]),             // index
        0,                          // base
        0xFFFFF,                    // limit
        GDT_FLAGS_UPPER_CODE,       // G/L
        GDT_FLAGS_LOWER_KERNELCODE, // S/DPL0/P
        GDT_TYPE_CODE               // code/permission: read
    );

    // GDT 64 bit kernel data segment
    kSetGDTEntry8(
        &(pstEntry[2]),             // index
        0,                          // base
        0xFFFFF,                    // limit
        GDT_FLAGS_UPPER_DATA,       // G/L
        GDT_FLAGS_LOWER_KERNELDATA, // S/DPL0/P
        GDT_TYPE_DATA               // data/permission: write
    );

    // GDT TSS descriptor
    kSetGDTEntry16(
        (GDTENTRY16 *) &(pstEntry[3]), // index
        (QWORD) pstTSS,                // base
        sizeof(TSSSEGMENT) - 1,        // limit
        GDT_FLAGS_UPPER_TSS,           // G
        GDT_FLAGS_LOWER_TSS,           // DPL0/P
        GDT_TYPE_TSS                   // system segment
    );


    /* initialize TSS structure */

    kInitializeTSSSegment(pstTSS);
}


// private function that initializes GDT 8 bytes entry.
// Targets are code/data segment descriptor
// params:
//   pstEntry: pointer to entry
//   dwBaseAddress: base address of descriptor
//   dwLimit: limit of descriptor
//   bUpperFlags: G/DB/L/AVL; use GDT_FLAGS_UPPER constants
//   bLowerFlags: P/DPL/S; use GDT_FLAGS_LOWER constants
//   bType; type; use GDT_TYPE constants
void kSetGDTEntry8(
    GDTENTRY8 *pstEntry,
    DWORD dwBaseAddress,
    DWORD dwLimit,
    BYTE bUpperFlags,
    BYTE bLowerFlags,
    BYTE bType
) {
    pstEntry->wLowerLimit = dwLimit & 0xFFFF;
    pstEntry->wLowerBaseAddress = dwBaseAddress & 0xFFFF;
    pstEntry->bUpperBaseAddress1 = (dwBaseAddress >> 16) & 0xFF;
    pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
    pstEntry->bUpperLimitAndUpperFlag = ((dwLimit >> 16) & 0x0F) | bUpperFlags;
    pstEntry->bUpperBaseAddress2 = (dwBaseAddress >> 24) & 0xFF;
}


// private function that initialize GDT 16 bytes entry.
// Targets are TSS descriptor
// params:
//   pstEntry: pointer to entry
//   dwBaseAddress: base address of descriptor
//   dwLimit: limit of descriptor
//   bUpperFlags: G/AVL; use GDT_FLAGS_UPPER constants
//   bLowerFlags: P/DPL/S; use GDT_FLAGS_LOWER constants
//   bType; type; use GDT_TYPE_TSS
void kSetGDTEntry16(
    GDTENTRY16 *pstEntry,
    QWORD qwBaseAddress,
    DWORD dwLimit,
    BYTE bUpperFlags,
    BYTE bLowerFlags,
    BYTE bType
) {
    pstEntry->wLowerLimit = dwLimit & 0xFFFF;
    pstEntry->wLowerBaseAddress = qwBaseAddress & 0xFFFF;
    pstEntry->bMiddleBaseAddress1 = (qwBaseAddress >> 16) & 0xFF;
    pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
    pstEntry->bUpperLimitAndUpperFlag = ((dwLimit >> 16) & 0xFF) | bUpperFlags;
    pstEntry->bMiddleBaseAddress2 = (qwBaseAddress >> 24) & 0xFF;
    pstEntry->dwUpperBaseAddress = qwBaseAddress >> 32;
    pstEntry->dwReserved = 0;
}


// private function that initializes Task State Segment for MINT64OS
//   info:
//     TSS segment has one IST address, 8MB and its size is 1MB.
//     This function should be called only once
//   params:
//     pstTSS: address of TSS
void kInitializeTSSSegment(TSSSEGMENT *pstTSS) {
    kMemSet(pstTSS, 0, sizeof(TSSSEGMENT));
    pstTSS->qwIST[0] = IST_STARTADDRESS + IST_SIZE;
    // by setting IO bigger than TSS limit, IO Mapping is not used
    pstTSS->wIOMapBaseAddress = 0xFFFF;
}


/*
 * Interrupt Descriptor Table
 */

// public function that initializes Interrupt Descriptor Table
void kInitializeIDTTables(void) {
    IDTR *pstIDTR;
    IDTENTRY *pstEntry;
    int i;

    // 1MB + 264KB (page table) + 160 bytes (GDTR, GDT, TSS)
    pstIDTR = (IDTR *) IDTR_STARTADDRESS;
    // 1MB + 264KB (page table) + 160 bytes (GDTR, GDT, TSS) +
    // 16 bytes (IDTR  + padding)
    pstEntry = (IDTENTRY *) (IDTR_STARTADDRESS + sizeof(IDTR));

    
    /* initialize IDTR */

    pstIDTR->qwBaseAddress = (QWORD) pstEntry;
    pstIDTR->wLimit = IDT_TABLESIZE - 1;


    /* 
     * initialize IDT entries
     * 
     * MINT64OS utilizes 100 interrupts/exceptions, and they uses
     * only one IST descriptor whose base address is 8MB and its size is 1MB
     * 
     * Every gate descriptor has the same flags. The only difference is
     * handler name
     * 
     * 0x08: kernel code segment offset
     * IDT_FLAGS_IST1: first IST descriptor
     * IDT_FLAGS_KERNEL: P/DPL0; ring 2~3 cannot software-interrupt
     * IDT_TYPE_INTERRUPT: type of descriptor
     */ 

    /* 0 to 19 reserved exception/interrupt handling functions */

    kSetIDTEntry(
        &(pstEntry[0]),
        kISRDivideError,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[1]),
        kISRDebug,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[2]),
        kISRNMI,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[3]),
        kISRBreakPoint,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[4]),
        kISROverflow,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL, 
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[5]),
        kISRBoundRangeExceeded, 
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );
    
    kSetIDTEntry(
        &(pstEntry[6]), 
        kISRInvalidOpcode, 
        0x08, 
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL, 
        IDT_TYPE_INTERRUPT
    );
    
    kSetIDTEntry(
        &(pstEntry[7]),
        kISRDeviceNotAvailable, 
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL, 
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[8]),
        kISRDoubleFault,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );
    
    kSetIDTEntry(
        &(pstEntry[9]),
        kISRCoprocessorSegmentOverrun,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[10]), 
        kISRInvalidTSS, 0x08, 
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL, 
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[11]),
        kISRSegmentNotPresent,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[12]), 
        kISRStackSegmentFault, 
        0x08, 
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL, 
        IDT_TYPE_INTERRUPT
    );
    
    kSetIDTEntry(
        &(pstEntry[13]), 
        kISRGeneralProtection, 
        0x08, 
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL, 
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[14]),
        kISRPageFault,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[15]),
        kISR15,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[16]),
        kISRFPUError,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[17]),
        kISRAlignmentCheck,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[18]),
        kISRMachineCheck,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[19]),
        kISRSIMDError,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );


    /* 
    * dummy handler 
    * handles rest of reserved exceptions from 20 to 31
    */

    for (i = 20; i < 32; i++) {
        kSetIDTEntry(
            &(pstEntry[i]),
            kISRETCException,
            0x08,
            IDT_FLAGS_IST1,
        	IDT_FLAGS_KERNEL,
            IDT_TYPE_INTERRUPT
        );
    }


    /*
    * PIC interrupt handling functions
    * In MINT64OS, there are referenced from 32 to 47 gate descriptors
    */

    kSetIDTEntry(
        &(pstEntry[32]),
        kISRTimer,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[33]),
        kISRKeyboard,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[34]),
        kISRSlavePIC,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[35]),
        kISRSerial2,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[36]),
        kISRSerial1,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[37]),
        kISRParallel2,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[38]),
        kISRFloppy,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[39]),
        kISRParallel1,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[40]),
        kISRRTC,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[41]),
        kISRReserved,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[42]),
        kISRNotUsed1,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[43]),
        kISRNotUsed2,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[44]),
        kISRMouse,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[45]),
        kISRCoprocessor,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[46]),
        kISRHDD1,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );

    kSetIDTEntry(
        &(pstEntry[47]),
        kISRHDD2,
        0x08,
        IDT_FLAGS_IST1,
        IDT_FLAGS_KERNEL,
        IDT_TYPE_INTERRUPT
    );


    /* 
    * dummy handler 
    * handles rest gate descriptors from 48
    */

    for (i = 48; i < IDT_MAXENTRYCOUNT; i++) {
        kSetIDTEntry(
            &(pstEntry[i]),
            kISRETCInterrupt,
            0x08,
            IDT_FLAGS_IST1,
        	IDT_FLAGS_KERNEL,
            IDT_TYPE_INTERRUPT
        );
    }
}


// private function that initializes IDT gate descriptor
//   params:
//     pstEntry: address of IDT entry
//     pvHandler: a interrupt or exception handler
//     wSelector: a GDT entry offset for context switching
//     bIST: index of IST  
//     bFlags: P/DPL; use IDT_FLAGS constants
//     bType: type of descriptor; use IDT_TYPE constants
void kSetIDTEntry(
    IDTENTRY *pstEntry,
    void *pvHandler,
    WORD wSelector,
    BYTE bIST,
    BYTE bFlags,
    BYTE bType
) {
    pstEntry->wLowerBaseAddress = (QWORD) pvHandler & 0xFFFF;
    pstEntry->wSegmentSelector = wSelector;
    pstEntry->bIST = bIST & 0x3;
    pstEntry->bTypeAndFlags = bType | bFlags;
    pstEntry->wMiddleBaseAddress = ( (QWORD) pvHandler >> 16 ) & 0xFFFF;
    pstEntry->dwUpperBaseAddress = (QWORD) pvHandler >> 32;
    pstEntry->dwReserved = 0;
}
