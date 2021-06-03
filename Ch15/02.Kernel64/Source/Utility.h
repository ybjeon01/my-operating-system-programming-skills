#ifndef __UTILITY_H__
#define __UTILITY_H__

#include "Types.h"


/* memory related functions */

// function that initializes a region of memory space
// params:
//   pvDestination: memory address where you want to initialize
//   bData: data to fill in the region
//   iSize: size of region
void kMemSet(void *pvDestination, BYTE bData, int iSize);

// function that copies data in source address to destination
// params:
//   pvDestination: destination address
//   pvSource: source address
//   iSize: size of data to copy
int kMemCpy(void *pvDestination, const void *pvSource, int iSize);

// function that compares data in pvSource with data in pvDestination
// params:
//   pvDestination: destination address
//   pvSource: source address
//   iSize: size of data to compare
int kMemCmp(const void *pvDestination, const void *pvSource, int iSize);


/* Interrupt related functions */

// enable or disable interrupt and return old interrupt state
// params:
//   bEnableInterupt: True to enable / False to disable
// return:
//   True if interrupt was enabled. Otherwise, False
BOOL kSetInterruptFlag(BOOL bEnableInterrupt);

#endif /* __UTILITY_H__ */