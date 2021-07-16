#include <stdarg.h>

#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "AssemblyUtility.h"

CONSOLEMANAGER gs_stConsoleManager = {0, };


// function that initializes console
// params:
//   iX: x of initial cursor location
//   iY: y of initial cursor location
void kInitializeConsole(int iX, int iY) {
    kMemSet(&gs_stConsoleManager, 0, sizeof(gs_stConsoleManager));
    kSetCursor(iX, iY);
}


// function that sets cursor location
// params:
//   iX: x of cursor location
//   iY: y of cursor location
void kSetCursor(int iX, int iY) {
    // linear address of cursor loc
    int iLinearValue = iY * CONSOLE_WIDTH + iX;

    // choose where to put the cursor upper byte 
    kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_UPPERCURSOR);
    kOutPortByte(VGA_PORT_DATA, iLinearValue >> 8);

    // choose where to put the cursor lower byte
    kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR);
    kOutPortByte(VGA_PORT_DATA, iLinearValue & 0xFF);

    gs_stConsoleManager.iCurrentPrintOffset = iLinearValue;
}


// function that gets cursor location
// params:
//   piX: pointer to hold iX
//   piY: pointer to hold iY
void kGetCursor(int *piX, int *piY) {
    *piX = gs_stConsoleManager.iCurrentPrintOffset % CONSOLE_WIDTH;
    *piY = gs_stConsoleManager.iCurrentPrintOffset / CONSOLE_WIDTH;    
}


// implementation of printf
// params:
//   pcFormatString: a string with format
//   ...: variables that will fill the template
// info:
//   %s: string type
//   %c: char type
//   %d, %i: integer type with sign
//   %x, %X: 4bytes integer without sign (print in hex)
//   %q, %Q: 8bytes integer without sign (print in hex)
//   %p: same as %q, %Q
//   %f: float number that rounds to two decimal places
//
//  overall length of string should be less than 1024 bytes
//  otherwise, unpredictable result that harms the system can
//  happens
void kPrintf(const char *pcFormatString, ...) {
    va_list ap;
    char vcBuffer[1024];
    int iNextPrintOffset;

    // put address after pcFormatString to ap
    va_start(ap, pcFormatString);
    kVSPrintf(vcBuffer, pcFormatString, ap);
    va_end(ap);

    // print to console
    iNextPrintOffset = kConsolePrintString(vcBuffer);

    // update cursor location so cursor is after text
    kSetCursor(
        iNextPrintOffset % CONSOLE_WIDTH,
        iNextPrintOffset / CONSOLE_WIDTH
    );
}


// print a formated string to console 
// params:
//   pcBuffer: a formated string with \n or \t
// return:
//   linear offset of console address after last text
// info:
//   if \n is included, offset is first offset of next line
//   if \t is included, offset is multiply of 4
//   if sentence does not fit in screen, one line goes up
int kConsolePrintString(const char *pcBuffer) {
    CHARACTER *pstScreen = (CHARACTER *) CONSOLE_VIDEOMEMORYADDRESS;
    int iPrintOffset = gs_stConsoleManager.iCurrentPrintOffset;
    int iLength = kStrLen(pcBuffer);

    for (int i = 0; i < iLength; i++) {
        if (pcBuffer[i] == '\n') {
            iPrintOffset += CONSOLE_WIDTH - (iPrintOffset % CONSOLE_WIDTH);
        }
        else if (pcBuffer[i] == '\t') {
            iPrintOffset += 8 - (iPrintOffset % 8);
        }
        else {
            pstScreen[iPrintOffset].bCharacter = pcBuffer[i];
            pstScreen[iPrintOffset].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
            iPrintOffset += 1;
        }

        int rowLength = CONSOLE_WIDTH * sizeof(CHARACTER);
        if (iPrintOffset >= CONSOLE_HEIGHT * CONSOLE_WIDTH) {
            kMemCpy(
                (void *) CONSOLE_VIDEOMEMORYADDRESS,
                (void *) CONSOLE_VIDEOMEMORYADDRESS + rowLength,
                (CONSOLE_HEIGHT - 1) * rowLength
            );
            
            int lastLine = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
            for (int j = lastLine; j < CONSOLE_HEIGHT * CONSOLE_WIDTH; j++) {
                pstScreen[j].bCharacter = ' ';
                pstScreen[j].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
            }

            iPrintOffset = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
        }
    }
    return iPrintOffset;
}


// clear all screen
void kClearScreen(void) {
    CHARACTER *pstScreen = (CHARACTER *) CONSOLE_VIDEOMEMORYADDRESS;

    for (int i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
        pstScreen[i].bCharacter = ' ';
        pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
    }
    kSetCursor(0, 0);
}


// implementation of getch()
// return:
//   byte size ascii code
BYTE kGetCh(void) {
    KEYDATA stData;
    while (TRUE) {
        if (kGetKeyFromKeyQueue(&stData)) {
            if (stData.bFlags & KEY_FLAGS_DOWN) {
                return stData.bASCIICode;
            }
        } 
    }
}


// write string to specific addr which is used for text mode, so you can
// see string in screen.
// iX: row where string will be
//     possible range: [0~24]
// iY: column where string will be
//     possible range: [0~79]
// string: string to write in screen
// 
// if string overflows iY, it wll be written to next line
// There is no protection for memory overflow. This means if your string
// overflows iX, the string can be written to Kernel area in worst case
void kPrintStringXY(int iX, int iY, const char *pcString) {
    CHARACTER *pstScreen = (CHARACTER *) CONSOLE_VIDEOMEMORYADDRESS;
    int i;

    pstScreen += iY * 80 + iX;

    for (i = 0; pcString[i] != 0; i++) {
        pstScreen[i].bCharacter = pcString[i];
        pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
    }
}
