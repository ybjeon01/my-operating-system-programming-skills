#ifndef __PCI_H__
#define __PCI_H__

#include "Types.h"


#define PCI_CONFIG_IO_ADDR                 0x0CF8
#define PCI_DATA_IO_ADDR                   0x0CFC


#define PCI_BUS_MAX                        0xFF
#define PCI_DEV_MAX                        0x20
#define PCI_FUNC_MAX                       0x08     


#define PCI_OFFSET_0x00                    0x00
#define PCI_OFFSET_0x04                    0x04
#define PCI_OFFSET_0x08                    0x08
#define PCI_OFFSET_0x0C                    0x0C
#define PCI_OFFSET_0x10                    0x10
#define PCI_OFFSET_0x14                    0x14
#define PCI_OFFSET_0x18                    0x18
#define PCI_OFFSET_0x1C                    0x1C
#define PCI_OFFSET_0x20                    0x20
#define PCI_OFFSET_0x24                    0x24


#pragma pack(push, 1)

typedef struct kPCIHeader {
    WORD wVendorID;
    WORD wDeviceID;
    WORD wCommand;
    WORD wStatus;
    BYTE bRevisionID;
    BYTE bProgIF;
    BYTE bSubclass;
    BYTE bClass;
    BYTE bCacheLineSize;
    BYTE bLatencyTimer;
    BYTE bHeaderType;
    BYTE bBist;
} PCIHeader;


#pragma pack(pop)


// read 4 bytes from PCI device configuration.
// params:
//   bBus: pci bus address.
//   bDev: pci device address.
//   bFunc: pci function address.
//   bOffset: offset of the chosen PCI configuration to read.
// return:
//   4 bytes from PCI device configuration addr + bOffset
// notes:
//   bBus must be in the range from 0 to 255.
//   bDev must be in the range from 0 to 31.
//   bFunc must be in the range from 0 to 7.
//   bOffset must be multiple of 4 bytes.
//   For bOffset, please use PCI_OFFSET_XXXX macro.
//   This is to avoid mistake as much as possible.
//   Otherwise, unexpected behavior will occur.
DWORD kReadPCIDword(
    const BYTE bBus,
    const BYTE bDev,
    const BYTE bFunc,
    const BYTE bOffset
);


// write 4 bytes to PCI device configuration.
// params:
//   bBus: pci bus address.
//   bDev: pci device address.
//   bFunc: pci function address.
//   bOffset: offset of the chosen PCI configuration to read.
//   dwValue: data to write
// notes:
//   bBus must be in the range from 0 to 255.
//   bDev must be in the range from 0 to 31.
//   bFunc must be in the range from 0 to 7.
//   bOffset must be multiple of 4 bytes.
//   For bOffset, please use PCI_OFFSET_XXXX macro.
//   This is to avoid mistake as much as possible.
//   Otherwise, unexpected behavior will occur.
void kWritePCIDword(
    const BYTE bBus,
    const BYTE bDev,
    const BYTE bFunc,
    const BYTE bOffset,
    const DWORD dwValue
);


// util function that writes 2 bytes to PCI device configuration.
// params:
//   bBus: pci bus address.
//   bDev: pci device address.
//   bFunc: pci function address.
//   bOffset: offset of the chosen PCI configuration to read.
//   wValue: data to write
// notes:
//   bBus must be in the range from 0 to 255.
//   bDev must be in the range from 0 to 31.
//   bFunc must be in the range from 0 to 7.
//   bOffset must be multiple of 2 bytes.
//   For bOffset, please use PCI_OFFSET_XXXX macro.
//   This is to avoid mistake as much as possible.
//   Otherwise, unexpected behavior will occur.
void kWritePCIWord(
    const BYTE bBus,
    const BYTE bDev,
    const BYTE bFunc,
    const BYTE bOffset,
    const WORD wValue
);



// util function that writes 1 byte to PCI device configuration.
// params:
//   bBus: pci bus address.
//   bDev: pci device address.
//   bFunc: pci function address.
//   bOffset: offset of the chosen PCI configuration to read.
//   wValue: data to write
// notes:
//   bBus must be in the range from 0 to 255.
//   bDev must be in the range from 0 to 31.
//   bFunc must be in the range from 0 to 7.
//   bOffset must be multiple of 1 byte.
//   For bOffset, please use PCI_OFFSET_XXXX macro.
//   This is to avoid mistake as much as possible.
//   Otherwise, unexpected behavior will occur.
void kWritePCIByte(
    const BYTE bBus,
    const BYTE bDev,
    const BYTE bFunc,
    const BYTE bOffset,
    const BYTE bValue
);


// check if there is a PCI device at the given address.
// params:
//   bBus: pci bus address.
//   bDev: pci device address.
// return:
//   true if there is device.
//   false if there is not device.
// notes:
//   bBus must be in the range from 0 to 255.
//   bDev must be in the range from 0 to 31.
//   bFunc must be in the range from 0 to 7.
//   Otherwise, unexpected behavior will occur.
BOOL kIsPCIDevice(const BYTE bBus, const BYTE bDev, const BYTE bFunc);


// util function to get address from base address with mask on the lowest bits.
// this function can distinguish IO port mapped address, 32 bit memory mapped
// address, and 64 bit memory mapped address.
// Once given the base address from PCI device, unnecessary bits are masked and
// returned.
// params:
//   bBus: pci bus address.
//   bDev: pci device address.
//   bFunc: pci function address.
//   bOffset: offset of the chosen PCI configuration to read.
// return:
//   base address.
// notes:
//   bBus must be in the range from 0 to 255.
//   bDev must be in the range from 0 to 31.
//   bFunc must be in the range from 0 to 7.
//   bOffset must be one of base address offsets.
//   Otherwise, unexpected behavior will occur.
QWORD kGetPCIBaseAddress(
    const BYTE bBus,
    const BYTE bDev,
    const BYTE bFunc,
    const BYTE bOffset
);


// get pci memory range of a base address.
// params:
//   bBus: pci bus address.
//   bDev: pci device address.
//   bFunc: pci function address.
//   bOffset: offset of the chosen PCI configuration to read.
// return:
//   true if there is device.
//   false if there is not device.
// notes:
//   bBus must be in the range from 0 to 255.
//   bDev must be in the range from 0 to 31.
//   bFunc must be in the range from 0 to 7.
//   bOffset must points to one of base addresses.
//   Otherwise, unexpected behavior will occur.
QWORD kGetPCIMemorySize(
    const BYTE bBus,
    const BYTE bDev,
    const BYTE bFunc,
    const BYTE bOffset
);

#endif /* __PCI_H__ */