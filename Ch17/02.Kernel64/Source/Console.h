#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "Types.h"

/* Video controller related constants */

// console background color
#define CONSOLE_BACKGROUND_BLACK            0x00
#define CONSOLE_BACKGROUND_BLUE             0x10
#define CONSOLE_BACKGROUND_GREEN            0x20
#define CONSOLE_BACKGROUND_CYAN             0x30
#define CONSOLE_BACKGROUND_RED              0x40
#define CONSOLE_BACKGROUND_MAGENTA          0x50
#define CONSOLE_BACKGROUND_BROWN            0x60
#define CONSOLE_BACKGROUND_WHITE            0x70
#define CONSOLE_BACKGROUND_BLINK            0x80

// console text color
#define CONSOLE_FOREGROUND_DARKBLACK        0x00
#define CONSOLE_FOREGROUND_DARKBLUE         0x01
#define CONSOLE_FOREGROUND_DARKGREEN        0x02
#define CONSOLE_FOREGROUND_DARKCYAN         0x03
#define CONSOLE_FOREGROUND_DARKRED          0x04
#define CONSOLE_FOREGROUND_DARKMAGENTA      0x05
#define CONSOLE_FOREGROUND_DARKBROWN        0x06
#define CONSOLE_FOREGROUND_DARKWHITE        0x07
#define CONSOLE_FOREGROUND_BRIGHTBLACK      0x08
#define CONSOLE_FOREGROUND_BRIGHTBLUE       0x09
#define CONSOLE_FOREGROUND_BRIGHTGREEN      0x0A
#define CONSOLE_FOREGROUND_BRIGHTCYAN       0x0B
#define CONSOLE_FOREGROUND_BRIGHTRED        0x0C
#define CONSOLE_FOREGROUND_BRIGHTMAGENTA    0x0D
#define CONSOLE_FOREGROUND_BRIGHTYELLOW     0x0E
#define CONSOLE_FOREGROUND_BRIGHTWHITE      0x0F


// default background color + text color
#define CONSOLE_DEFAULTTEXTCOLOR    ( \
    CONSOLE_BACKGROUND_BLACK | \
    CONSOLE_FOREGROUND_BRIGHTGREEN \
)


#define CONSOLE_WIDTH               80
#define CONSOLE_HEIGHT              25
#define CONSOLE_VIDEOMEMORYADDRESS  0xB8000

// video controller I/O port addresses
#define VGA_PORT_INDEX              0x3D4
#define VGA_PORT_DATA               0x3D5
#define VGA_INDEX_UPPERCURSOR       0x0E
#define VGA_INDEX_LOWERCURSOR       0x0F



#pragma pack(push, 1)


typedef struct kConsoleManagerStruct {
    // cursor location
    int iCurrentPrintOffset;
} CONSOLEMANAGER;

#pragma pack(pop)


/* console related functions */

// function that initializes console
// params:
//   iX: x of initial cursor location
//   iY: y of initial cursor location
void kInitializeConsole(int iX, int iY);


// function that sets cursor location
// params:
//   iX: x of cursor location
//   iY: y of cursor location
void kSetCursor(int iX, int iY);


// function that gets cursor location
// params:
//   piX: pointer to hold iX
//   piY: pointer to hold iY
void kGetCursor(int *piX, int *piY);


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
void kPrintf(const char* pcFormatString, ...);


// print a formated string to console 
// params:
//   pcBuffer: a formated string with \n or \t
// return:
//   linear offset of console address after last text
// info:
//   if \n is included, offset is first offset of next line
//   if \t is included, offset is multiply of 4
//   if sentence does not fit in screen, one line goes up
int kConsolePrintString(const char* pcBuffer);


// clear all screen
void kClearScreen(void);


// implementation of getch()
// return:
//   byte size ascii code
BYTE kGetCh(void);


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
void kPrintStringXY(int iX, int iY, const char* pcString);

#endif /*__CONSOLE_H__*/