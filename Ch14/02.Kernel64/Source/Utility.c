#include "Utility.h"
#include "AssemblyUtility.h"

/* memory related functions */

// function that initializes a region of memory space
// params:
//   pvDestination: memory address where you want to initialize
//   bData: data to fill in the region
//   iSize: size of region
void kMemSet(void *pvDestination, BYTE bData, int iSize) {
    for (int i = 0; i < iSize; i++) {
        ((char *) pvDestination)[i] = bData;
    }
}

// function that copies data in source address to destination
// params:
//   pvDestination: destination address
//   pvSource: source address
//   iSize: size of data to copy
int kMemCpy(void *pvDestination, const void *pvSource, int iSize) {
    int i;

    for (i=0; i < iSize; i++) {
        ((char *) pvDestination)[i] = ((char *) pvSource)[i];
    }
    return iSize;
}

// function that compares data in pvSource with data in pvDestination
// params:
//   pvDestination: destination address
//   pvSource: source address
//   iSize: size of data to compare
int kMemCmp (const void *pvDestination, const void *pvSource, int iSize) {
    int i;
    char cTemp;

    for (i = 0; i < iSize; i++) {
        cTemp = ((char *) pvDestination)[i] - ((char *) pvSource)[i];
        if (cTemp != 0) {
            return (int) cTemp;
        }
    }
    return 0;
}


/* Interrupt related functions */

// enable or disable interrupt and return old interrupt state
// params:
//   bEnableInterupt: True to enable / False to disable
// return:
//   True if interrupt was enabled. Otherwise, False
BOOL kSetInterruptFlag(BOOL bEnableInterrupt) {
	QWORD qwRFLAGS;
	qwRFLAGS = kReadRFLAGS();

	if (bEnableInterrupt == TRUE) {
		kEnableInterrupt();
	}
	else {
		kDisableInterrupt();
	}

	// check IF bit (interrupt flag, bit 9)
	// 'qwRFLAGS & 0x0200' is bit operation so you should not use
	// "return qwRFLAGS & 0x0200;"
    if (qwRFLAGS & 0x0200) {
    	return TRUE;
    }
    return FALSE;
}