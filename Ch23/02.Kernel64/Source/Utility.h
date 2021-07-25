#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <stdarg.h>

#include "Types.h"


/* memory related structures and functions */

// entry of memory mapping returned by E820 bios service
typedef struct SMAP_entry {
    QWORD Base;
    QWORD Length;
    DWORD Type;   // 1: free, 2~4: non free 
    DWORD ACPI;
} SMAP_entry_t;

#define SMAP_COUNT_ADDRESS 0x20000
#define SMAP_START_ADDRESS 0x20004


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


// check total ram size of computer
// info:
//   this function should called only once when computer boot
//   other application must not call this function
void kCheckTotalRAMSize(void);


// returns total ram size of this computer
// return:
//   total ram size of this computer
QWORD kGetTotalRAMSize(void);


/* Interrupt related functions */

// enable or disable interrupt and return old interrupt state
// params:
//   bEnableInterupt: True to enable / False to disable
// return:
//   True if interrupt was enabled. Otherwise, False
BOOL kSetInterruptFlag(BOOL bEnableInterrupt);


/* string related functions */

// get length of a string
// params:
//   pcBuffer: pointer to string that has \0
// return:
//   length of a string except \0
// info:
//   if string does not have \0, the result is
//   unpredicted
int kStrLen(const char *pcBuffer);


// implementation of atoi()
// params:
//   pcBuffer: a string with only numbers
//   iRadix: notation of number in string
//     option: 10 or 16
// return:
//   long size integer
long kAToI(const char *pcBuffer, int iRadix);


// convert a hex string to integer
// params:
//   pcBuffer: hex string
// return:
//   long size integer
QWORD kHexStringToQword(const char *pcBuffer);


// convert a decimal string to integer
// params:
//   pcBuffer: decimal string
// return:
//   long size integer
long kDecimalStringToLong(const char *pcBuffer);


// implementation of itoa()
// params:
//   lValue: integer
//   pcBuffer: pointer where the converted string will be held
//   iRadix: hex or decimal notation
//     options: 10 means decimal string and 16 means hex string
// return:
//   length of string to return
int kIToA(long lValue, char *pcBuffer, int iRadix);


// convert integer to hex string
// params:
//   qwValue: integer
//   pcBuffer: pointer where the converted hex string will be held
// return:
//   length of the converted hex string
int kHexToString(QWORD qwValue, char *pcBuffer);


// convert integer to decimal string
// params:
//   qwValue: integer
//   pcBuffer: pointer where the converted decimal string will be held
// return:
//   length of the converted decimal string
int kDecimalToString(long lValue, char *pcBuffer);


// reverse string
// params:
//   pcBuffer: pointer to a string
void kReverseString(char *pcBuffer);


// implementation of sprintf()
// params:
//   pcBuffer: pointer where the formatted string will be held
//   pcFormatString: string template
//   ...: variables that substitute the template
// return:
//   size of the formatted string
int kSPrintf(char *pcBuffer, const char *pcFormatString, ...);


// implementation of vsprintf()
// params:
// params:
//   pcBuffer: pointer where the formatted string will be held
//   pcFormatString: string template
//   ap: start address of variable length arguments
// return:
//   size of the formatted string
int kVSPrintf(char *pcBuffer, const char *pcFormatString, va_list ap);


/* scheduler related constants */

extern volatile QWORD g_qwTickCount;


// get the count of ticks that PIT interrupt occurs
// return:
//   number of ticks that PIT interrupt happens from the start of booting
QWORD kGetTickCount(void);


// stop executing instructions for qwMillisecond
// params:
//   qwMillisecond: time to sleep
void kSleep(QWORD qwMillisecond) ;

#endif /* __UTILITY_H__ */
