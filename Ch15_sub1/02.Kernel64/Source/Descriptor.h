#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include "Types.h"

/*
 * flags of global descriptors
 */

#define GDT_TYPE_CODE           0x0A // code descriptor with read permission
#define GDT_TYPE_DATA           0x02 // data with write permission
#define GDT_TYPE_TSS            0x09 // task status segment descriptor

#define GDT_FLAGS_LOWER_S       0x10 // segment descriptor 
#define GDT_FLAGS_LOWER_DPL0    0x00 // ring 0
#define GDT_FLAGS_LOWER_DPL1    0x20 // ring 1
#define GDT_FLAGS_LOWER_DPL2    0x40 // ring 2
#define GDT_FLAGS_LOWER_DPL3    0x60 // ring 3
#define GDT_FLAGS_LOWER_P       0x80 // prsent

// long mode : always 1
// protected mode: always 0
#define GDT_FLAGS_UPPER_L       0x20

// long mode: always 0
// protected mode:
//   code: 1 for 16 bit, 0 for 32 bit
//   data: 0 for sp, 1 for esp
#define GDT_FLAGS_UPPER_DB      0x40

// long mode: always 1
// protected mode: 1 for accessing 4GB
#define GDT_FLAGS_UPPER_G       0x80


/*
 * global descriptor flags for MINT64OS
 */

// lower part of kernel code segment descriptor: [bit 44: bit 48)
#define GDT_FLAGS_LOWER_KERNELCODE ( \
    GDT_FLAGS_LOWER_S | \
    GDT_FLAGS_LOWER_DPL0 | \
    GDT_FLAGS_LOWER_P \
)

// lower part of kernel data segment descriptor: [bit 44: bit 48)
#define GDT_FLAGS_LOWER_KERNELDATA ( \
    GDT_FLAGS_LOWER_S | \
    GDT_FLAGS_LOWER_DPL0 | \
    GDT_FLAGS_LOWER_P \
)

// lower part of segment descriptor: [bit 44: bit 48)
#define GDT_FLAGS_LOWER_TSS ( \
    GDT_FLAGS_LOWER_DPL0 | \
    GDT_FLAGS_LOWER_P \
)

// lower part of user code segment descriptor: [bit 44: bit 48)
#define GDT_FLAGS_LOWER_USERCODE ( \
    GDT_TYPE_CODE | \
    GDT_FLAGS_LOWER_S | \
    GDT_FLAGS_LOWER_DPL3 | \
    GDT_FLAGS_LOWER_P \
)

// lower part of user data segment descriptor: [bit 44: bit 48)
#define GDT_FLAGS_LOWER_USERDATA ( \
    GDT_TYPE_DATA | \
    GDT_FLAGS_LOWER_S | \
    GDT_FLAGS_LOWER_DPL3 | \
    GDT_FLAGS_LOWER_P \
)

// upper part of code segment descriptor: [bit 52: bit 56)
#define GDT_FLAGS_UPPER_CODE ( \
    GDT_FLAGS_UPPER_G | \
    GDT_FLAGS_UPPER_L \
)

// upper part of data segment descriptor: [bit 52: bit 56)
#define GDT_FLAGS_UPPER_DATA ( \
    GDT_FLAGS_UPPER_G | \
    GDT_FLAGS_UPPER_L \
)

// upper part of TSS system descriptor: [bit 52: bit 56)
#define GDT_FLAGS_UPPER_TSS ( \
    GDT_FLAGS_UPPER_G \
)


/*
 * MINT64OS global descriptors offset
 */

#define GDT_KERNELCODESEGMENT   0x08

#define GDT_KERNELDATASEGMENT   0x10

#define GDT_TSSSEGMENT          0x18


/*
 * MINT64OS GDT related memory layout
 */

// GDTR start address: 1MB + 264KB page table
#define GDTR_STARTADDRESS       0x142000

// number of 8 byte entries in MINT64OS:
//   null descriptor, kernel code, kernel data
#define GDT_MAXENTRY8COUNT      3

// number of 16 byte entry in MINT64OS:
//   TSS entry
#define GDT_MAXENTRY16COUNT     1

// size of Global Descriptor Table of MINT64OS
#define GDT_TABLESIZE ( \
    (sizeof(GDTENTRY8) * GDT_MAXENTRY8COUNT) + \
    (sizeof(GDTENTRY16) * GDT_MAXENTRY16COUNT) \
)

// size of TSS of MINT64OS
#define TSS_SEGMENTSIZE (sizeof(TSSSEGMENT))


/*
 * flags of Interrupt Descriptor Table's gate descriptor
 */

#define IDT_TYPE_INTERRUPT      0x0E // interrupt IDT gate descriptor
#define IDT_TYPE_TRAP           0x0F // trap IDT gate descriptor

#define IDT_FLAGS_DPL0          0x00 // ring 0
#define IDT_FLAGS_DPL1          0x20 // ring 1
#define IDT_FLAGS_DPL2          0x40 // ring 2
#define IDT_FLAGS_DPL3          0x60 // ring 3
#define IDT_FLAGS_P             0x80 // present

#define IDT_FLAGS_IST1          1 // first index of IST. MINT64OS has only one


/*
 * IDT gate descriptor flags for MINT64OS
 */

// IDT gate descriptor that kernel-level code utilizes
#define IDT_FLAGS_KERNEL ( \
    IDT_FLAGS_DPL0 | \
    IDT_FLAGS_P \
)

// IDT gate descriptor that user-level code utilizes
#define IDT_FLAGS_USER ( \
    IDT_FLAGS_DPL3 | \
    IDT_FLAGS_P \
)


/*
 * MINT64OS IDT related memory layout
 */

// MINT64OS number of IDT entries
#define IDT_MAXENTRYCOUNT   100

// IDTR start address:
//   1MB + 264KB page table + 160 bytes TSS 
#define IDTR_STARTADDRESS ( \
    GDTR_STARTADDRESS + \
    sizeof(GDTR) + \
    GDT_TABLESIZE + \
    TSS_SEGMENTSIZE \
)

// IDT start address:
//   1MB + 264KB page table + 160 bytes TSS + 16 bytes IDTR
#define IDT_STARTADDRESS ( \
    IDTR_STARTADDRESS + \
    sizeof(IDTR) \
)

// size of Interrupt Descriptor Table of MINT64OS
#define IDT_TABLESIZE ( \
    IDT_MAXENTRYCOUNT * \
    sizeof(IDTENTRY) \
)

// IST start address: 7MB
#define IST_STARTADDRESS			0x700000

// IST size: 1MB
#define IST_SIZE					0x100000

#pragma pack(push, 1)


/*
 * GDTR and IDTR structure
 * GDT and IDT descriptor structure
 */

// GDTR and IDTR structure
typedef struct kGDTRStruct {
    WORD wLimit;
    QWORD qwBaseAddress;
    // to align 16 byte address
    WORD wPadding;
    DWORD dwPadding;
} GDTR, IDTR;

// 8 byte GDT entry structure:
//   code, data segment descriptor
typedef struct kGDTEntry8Struct {
    WORD wLowerLimit;             // LIMIT (16:0]
    WORD wLowerBaseAddress;       // BASE (32:16)
    BYTE bUpperBaseAddress1;      // BASE (40:32]
    BYTE bTypeAndLowerFlag;       // P/DPL/S/Type (48:40]
    BYTE bUpperLimitAndUpperFlag; // G/DB/L/AVL/LIMIT (56:48]
    BYTE bUpperBaseAddress2;      // BASE (64:56]
} GDTENTRY8;

// 16 bytes GDT entry structure:
//   TSS descriptor
typedef struct kGDTEntry16Struct {
    WORD wLowerLimit;             // LIMIT (16:0]
    WORD wLowerBaseAddress;       // BASE (32:16)
    BYTE bMiddleBaseAddress1;     // BASE (40:32]
    BYTE bTypeAndLowerFlag;       // P/DPL/S/Type (48:40]
    BYTE bUpperLimitAndUpperFlag; // G/-/-/AVL/LIMIT (56:48]
    BYTE bMiddleBaseAddress2;     // BASE (64:56]
    DWORD dwUpperBaseAddress;     // BASE (96:64]
    DWORD dwReserved;             // RES/0/RES (128:96]
} GDTENTRY16;

// 104 bytes TSS structure for long mode
typedef struct kTSSDataStruct {
    DWORD dwReserved1;       // RES (32:0]
    QWORD qwRsp[3];          // RSP2/RSP1/RSP0 (224:32]
    QWORD qwReserved2;       // RES (228:224]
    QWORD qwIST[7];          // IST7/IST6/IST5/IST4/IST3/IST2/IST1 (736:228]
    QWORD qwReserved3;       // RES (800:736]
    WORD wReserved;          // RES (816:800]
    WORD wIOMapBaseAddress;  // I/O Map BASE (832:816]
} TSSSEGMENT;

// 16 bytes IDT gate descriptor structure
typedef struct kIDTEntryStruct {
    WORD wLowerBaseAddress;   // handler offset (16:0]
    WORD wSegmentSelector;    // CS descriptor offset (32:16)
    // 3 bits IST, 5 bits 0
    BYTE bIST;                // 0/0/0/0/0/IST (40:32]
    BYTE bTypeAndFlags;       // P/DPL/0/Type (48:40]
    WORD wMiddleBaseAddress;  // handler offset (64:48]
    DWORD dwUpperBaseAddress; // handler offset (96:64]
    DWORD dwReserved;         // RES (128:96]
} IDTENTRY;

#pragma pack (pop)


/*
 * functions for setting descriptors and initializing tables
 */

// public function that initializes GDT, its entries, and TSS
//   info:
//     In Mint64OS, page table, GDTR, GDT, TSS, IDTR, IDT are located
//     consecutively from 1MB
void kInitializeGDTTableAndTSS(void);

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
);

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
);

// private function that initializes Task State Segment for MINT64OS
//   info:
//     TSS segment has one IST address, 8MB and its size is 1MB.
//     This function should be called only once
//   params:
//     pstTSS: address of TSS
void kInitializeTSSSegment(TSSSEGMENT *pstTSS);

// public function that initializes Interrupt Descriptor Table
void kInitializeIDTTables(void);

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
    BYTE vType
);

// public and temporary handler for interrupt test
void kDummyHandler(void);

#endif