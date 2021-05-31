#ifndef __ASSEMBLYUTILITY_H__
#define __ASSEMBLYUTILITY_H__

#include "Types.h"

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

#endif /* __ASSEMBLYUTILITY_H__ */