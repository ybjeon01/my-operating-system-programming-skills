#include "PCI.h"
#include "Types.h"
#include "AssemblyUtility.h"


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
    const BYTE bOffset) {

    DWORD dwAddr;

    dwAddr = (1 << 31) | (bBus << 16) | (bDev << 11) | (bFunc << 8) | bOffset;  
    
    kOutPortDword(PCI_CONFIG_IO_ADDR, dwAddr);
    return kInPortDword(PCI_DATA_IO_ADDR);
}


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
    const DWORD dwValue) {

    DWORD dwAddr;

    dwAddr = (1 << 31) | (bBus << 16) | (bDev << 11) | (bFunc << 8) | bOffset;  
    
    kOutPortDword(PCI_CONFIG_IO_ADDR, dwAddr);
    kOutPortDword(PCI_DATA_IO_ADDR, dwValue);
}


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
) {
    DWORD dwAddr;
    DWORD data;

    // pci addr must be multiple of 4 bytes
    data = kReadPCIDword(bBus, bDev, bFunc, bOffset & 0xFC);
    // write bValue to data
    data |= (wValue << ((bOffset & 2) * 8));
    kWritePCIDword(bBus, bDev, bFunc, bOffset & 0xFC, data);
}


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
) {
    DWORD dwAddr;
    DWORD data;

    // pci addr must be multiple of 4 bytes
    data = kReadPCIDword(bBus, bDev, bFunc, bOffset & 0xFC);
    // write bValue to data
    data |= (bValue << ((bOffset & 3) * 8));
    kWritePCIDword(bBus, bDev, bFunc, bOffset & 0xFC, data);
}


// util function to check if there is a PCI device at the given address.
// params:
//   bBus: pci bus address.
//   bDev: pci device address.
//   bFunc: pci function address.
// return:
//   true if there is device.
//   false if there is not device.
// notes:
//   bBus must be in the range from 0 to 255.
//   bDev must be in the range from 0 to 31.
//   bFunc must be in the range from 0 to 7.
//   Otherwise, unexpected behavior will occur.
BOOL kIsPCIDevice(const BYTE bBus, const BYTE bDev, const BYTE bFunc) {
    return kReadPCIDword(bBus, bDev, bFunc, PCI_OFFSET_0x00) != 0xFFFF;
}


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
) {
    DWORD dwBase0;
    QWORD dwBase1;

    dwBase0 = kReadPCIDword(bBus, bDev, bFunc, bOffset);

    // 64 bit and memory mapped IO
    if ((dwBase0 & 0x07) == 0x04)
        dwBase1 = kReadPCIDword(bBus, bDev, bFunc, bOffset + 4);

    
    // Port IO address
    if (dwBase0 & 0x01)
        return dwBase0 & 0xFFFFFFFC;
    else
        return (dwBase1 << 32) | (QWORD) dwBase0 & 0xFFFFFFFFFFFFFFF0;
}


// get pci memory size of a base address.
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
) {
    DWORD dwCmdStu;
    DWORD dwBase0;
    DWORD dwBase1;
    QWORD qwSize;

    qwSize = 0;

    /* disable PCI device before reading size */

    // save command register to restore later
    // disable bus master
    dwCmdStu = kReadPCIDword(bBus, bDev, bFunc, PCI_OFFSET_0x04);
    kWritePCIDword(bBus, bDev, bFunc, PCI_OFFSET_0x04, dwCmdStu & ~0x07);


    /* read original base address */

    // save base address to restore later
    dwBase0 = kReadPCIDword(bBus, bDev, bFunc, bOffset);

    // 64 bit and memory mapped IO
    if ((dwBase0 & 0x07) == 0x04)
        dwBase1 = kReadPCIDword(bBus, bDev, bFunc, bOffset + 4);

    /* Ask size of IO mapped memory */

    // writing 0xFFFFFFFF to base addr register shows 2's complement of
    // memory size
    kWritePCIDword(bBus, bDev, bFunc, bOffset, 0xFFFFFFFF);
    
    // 64 bit and memory mapped IO
    if ((dwBase0 & 0x07) == 0x04)
        kWritePCIDword(bBus, bDev, bFunc, bOffset + 4, 0xFFFFFFFF);


    /* read size of IO mampped memroy */

    qwSize = kReadPCIDword(bBus, bDev, bFunc, bOffset);

    // 64 bit and memory mapped IO
    if ((dwBase0 & 0x07) == 0x04)
        qwSize |= ((QWORD) kReadPCIDword(bBus, bDev, bFunc, bOffset + 4)) << 32;
    

    /* restore original values to registers */

    // restore base addrress
    kWritePCIDword(bBus, bDev, bFunc, bOffset, dwBase0);

    // 64 bit and memory mapped IO
    if ((dwBase0 & 0x07) == 0x04)
        kWritePCIDword(bBus, bDev, bFunc, bOffset + 4, dwBase1);

    // restore the command register
    kWritePCIDword(bBus, bDev, bFunc, PCI_OFFSET_0x04, dwCmdStu);
    

    // port IO
    if (dwBase0 & 0x01) {
        // upper 16 bits are reserved if read as zero
        qwSize |= 0xFFFFFFFFFFFF0000;
        
        // mask information bits and 2's complement
        // port IO use the 0 and 1 bit as info bits
        return ~(qwSize & ~0x03) + 1;
    }
    // Memory mapped IO
    else {
        return ~(qwSize & 0xFFFFFFFFFFFFFFF0) + 1;
    }
}