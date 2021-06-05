#include <stdarg.h>

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


// total ram size (MB)
static QWORD gs_qwTotalRAMMBSize = 0;

// check total ram size of computer
// info:
//   this function should called only once when computer boot
//   other application must not call this function
void kCheckTotalRAMSize(void) {
    QWORD *pdwCurrentAddress;
    QWORD dwPreviousValue;

    pdwCurrentAddress = (QWORD *) 0x4000000;

    // starts from 64MB (0x4000000) and check address that is multiply of 4MB
    while (TRUE) {
        dwPreviousValue = *pdwCurrentAddress;

        *pdwCurrentAddress = 0x12345678;

        // if the address does not exist, it does not have correct value
        if (*pdwCurrentAddress != 0x12345678) {
            break;
        }

        *pdwCurrentAddress = dwPreviousValue;
        pdwCurrentAddress += (0x400000 / 8);
    }

    gs_qwTotalRAMMBSize = (QWORD) pdwCurrentAddress / 0x100000;
}


// returns total ram size of this computer
// return:
//   total ram size of this computer
QWORD kGetTotalRAMSize(void) {
    return gs_qwTotalRAMMBSize;
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


/* string related functions */

// get length of a string
// params:
//   pcBuffer: pointer to string that has \0
// return:
//   length of a string except \0
// info:
//   if string does not have \0, the result is
//   unpredicted
int kStrLen(const char *pcBuffer) {
    int i;
    for (i = 0;; i++) {
        if (pcBuffer[i] == '\0') {
            break;
        }
    }
    return i;
}


// implementation of atoi()
// params:
//   pcBuffer: a string with only numbers
//   iRadix: notation of number in string
//     option: 10 or 16
// return:
//   long size integer
long kAToI(const char *pcBuffer, int iRadix) {
    long lReturn;

    switch (iRadix) {
        case 16:
            lReturn = kHexStringToQword(pcBuffer);
            break;
        case 10:
        default:
            lReturn = kDecimalStringToLong(pcBuffer);
            break;
    }
    return lReturn;
}


// convert a hex string to integer
// params:
//   pcBuffer: hex string
// return:
//   long size integer
QWORD kHexStringToQword(const char *pcBuffer) {
    QWORD qwValue = 0;
    for (int i = 0; pcBuffer[i] != '\0'; i++) {
        qwValue *= 16;
        if (('A' <= pcBuffer[i]) && (pcBuffer[i] <= 'Z')) {
            qwValue += (pcBuffer[i] - 'A') + 10;
        }
        else if (('a' <= pcBuffer[i]) && (pcBuffer[i] <= 'z')) {
            qwValue += (pcBuffer[i] - 'a') + 10;
        }
        else {
            qwValue += pcBuffer[i] -'0';
        }
    }
    return qwValue;
}


// convert a decimal string to integer
// params:
//   pcBuffer: decimal string
// return:
//   long size integer
long kDecimalStringToLong(const char *pcBuffer) {
    long lValue = 0;
    int i = 0;

    // negative integer
    if (pcBuffer[0] == '-') {
        i = 1;
    }

    for (; pcBuffer[i] != '\0'; i++) {
        lValue *= 10;
        lValue += pcBuffer[i] - '0';
    }

    if (pcBuffer[0] == '-') {
        lValue *= -1;
    }
    return lValue;
}


// implementation of itoa()
// params:
//   lValue: integer
//   pcBuffer: pointer where the converted string will be held
//   iRadix: hex or decimal notation
//     options: 10 means decimal string and 16 means hex string
// return:
//   length of string to return
int kIToA(long lValue, char *pcBuffer, int iRadix) {
    int iReturn;

    switch (iRadix) {
        case 16:
            iReturn = kHexToString(lValue, pcBuffer);
            break;

        case 10:
        default:
            iReturn = kDecimalToString(lValue, pcBuffer);
            break;
    }
    return iReturn;
}


// convert integer to hex string
// params:
//   qwValue: integer
//   pcBuffer: pointer where the converted hex string will be held
// return:
//   length of the converted hex string
int kHexToString(QWORD qwValue, char *pcBuffer) {
    QWORD qwCurrentValue;
    QWORD i;

    if (qwValue == 0) {
        pcBuffer[0] = '0';
        pcBuffer[1] = '\0';
        return 1;
    }

    for (i = 0; qwValue > 0; i++) {
        qwCurrentValue = qwValue % 16; 
        if (qwCurrentValue >= 10) {
            pcBuffer[i] = 'A' + (qwCurrentValue - 10);
        }
        else {
            pcBuffer[i] = '0' + qwCurrentValue;
        }
        qwValue = qwValue / 16;
    }
    pcBuffer[i] = '\0';
    kReverseString(pcBuffer);
    return i;
}


// convert integer to decimal string
// params:
//   qwValue: integer
//   pcBuffer: pointer where the converted decimal string will be held
// return:
//   length of the converted decimal string
int kDecimalToString(long lValue, char *pcBuffer) {
    long i;

    if (lValue == 0) {
        pcBuffer[0] = '0';
        pcBuffer[1] = '\0';
        return 1;
    }

    if (lValue < 0) {
        i = 1;
        pcBuffer[0] = '-';
        lValue *= -1;
    }
    else {
        i = 0;
    }

    for (; lValue > 0; i++) {
        pcBuffer[i] = '0' + lValue % 10;
        lValue = lValue / 10;
    }
    pcBuffer[i] = '\0';

    if (pcBuffer[0] == '-') {
        kReverseString(&(pcBuffer[1]));
    }
    else {
        kReverseString(pcBuffer);
    }
    return i;
}


// reverse string
// params:
//   pcBuffer: pointer to a string
void kReverseString(char *pcBuffer) {
    int iLength;
    char cTemp;

    iLength = kStrLen(pcBuffer);

    for (int i = 0; i < iLength / 2; i++) {
        cTemp = pcBuffer[i];
        pcBuffer[i] = pcBuffer[iLength - 1 - i];
        pcBuffer[iLength - 1 - i] = cTemp;
    }
}


// implementation of sprintf()
// params:
//   pcBuffer: pointer where the formatted string will be held
//   pcFormatString: string template
//   ...: variables that substitute the template
// return:
//   size of the formatted string
int kSprintf(char *pcBuffer, const char *pcFormatString, ...) {
    va_list ap;
    int iReturn;

    va_start(ap, pcFormatString);
    iReturn = kVSPrintf(pcBuffer, pcFormatString, ap);
    va_end(ap);

    return iReturn;
}


// implementation of vsprintf()
// params:
// params:
//   pcBuffer: pointer where the formatted string will be held
//   pcFormatString: string template
//   ap: start address of variable length arguments
// return:
//   size of the formatted string
int kVSPrintf(char *pcBuffer, const char *pcFormatString, va_list ap) {
    QWORD i;
    int iBufferIndex = 0;
    int iFormatLength, iCopyLength;
    char *pcCopyString;
    QWORD qwValue;
    int iValue;

    iFormatLength = kStrLen(pcFormatString);
    for (i = 0; i < iFormatLength; i++) {
        if (pcFormatString[i] == '%') {
            i++;
            switch (pcFormatString[i]) {
                // string
                case 's':
                    pcCopyString = (char *) (va_arg(ap, char *));
                    iCopyLength = kStrLen(pcCopyString);
                    kMemCpy(pcBuffer + iBufferIndex, pcCopyString, iCopyLength);
                    iBufferIndex += iCopyLength;
                    break;
                
                // character
                case 'c':
                    pcBuffer[iBufferIndex] = (char) (va_arg(ap, int));
                    iBufferIndex++;
                    break;

                // integer
                case 'd':
                case 'i':
                    iValue = (int) (va_arg(ap, int));
                    iBufferIndex += kIToA(iValue, pcBuffer + iBufferIndex, 10);
                    break;

                // 4 bytes in hex format
                case 'x':
                case 'X':
                    qwValue = (DWORD) (va_arg(ap, DWORD)) & 0xFFFFFFFF;
                    iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
                    break;

                // 8 bytes in hex format
                case 'q':
                case 'Q':
                case 'p':
                    qwValue = (QWORD) (va_arg(ap, QWORD));
                    iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
                    break;
                
                default:
                    pcBuffer[iBufferIndex] = pcFormatString[i];
                    iBufferIndex++;
                    break;
            }
        }
        else {
            pcBuffer[iBufferIndex] = pcFormatString[i];
            iBufferIndex++;
        }
    }
    pcBuffer[iBufferIndex] = '\0';
    return iBufferIndex;
}
