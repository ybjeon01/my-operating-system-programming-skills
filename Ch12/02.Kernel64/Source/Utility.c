#include "Utility.h"


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