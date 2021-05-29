#include "Types.h"
#include "Page.h"

void kPrintString(int iX, int iY, const char *pcString);
BOOL kInitializeKernel64Area(void);
BOOL kIsMemoryEnough(void);


void Main(void) {
	kPrintString(0, 3, "C Language Kernel Started~!!!");

    // Check if computer has enough RAM.
    // MINT64OS requires 64MB RAM
    kPrintString(0, 4, "Minimum Memory Size Check...................[    ]");
    if (!kIsMemoryEnough()) {
        kPrintString(45, 4, "Fail");
        kPrintString(0, 5, "Not Enough Memory~!! MINT64OS requires over"
            " 64Mbyte Memory~!!");

        // stop processing
        while(1);
    }
    kPrintString(45, 4, "Pass");


    // Initialize IA-32e mode kernel area with value of 0x00
    // area: 1MB ~ 6MB
    kPrintString(0, 5, "IA-32e Kernel Area Initialize...............[    ]");
    if (!kInitializeKernel64Area()) {
        kPrintString(45, 5, "Fail");
        kPrintString(0, 6, "Kernel Area Initialization Fail~!!");

        // stop processing
        while(1);
    }
    kPrintString(45, 5, "Pass");


    // initialize page table tree for IA-32e mode kernel
    // area: 1MB ~ (1MB + 264KB)
    kPrintString(0, 6, "IA-32e Page Tables initialize...............[    ]");
    kInitializePageTables();
    kPrintString(45, 6, "Pass");

    // stop processing
	while (1);
}

// write string to specific addr which is used for text mode, so you can
// see string in screen.
// iX: row where string will be
//     possible range: [0~24]
// iY: colume where string will be
//     possible range: [0~79]
// string: string to write in screen
// 
// if string overflows iY, it wll be written to next line
// There is no protection for memory overflow. This means if your string
// overflows iX, the string can be written to Kernel area in worst case
void kPrintString(int iX, int iY, const char *pcString) {
	CHARACTER *pstScreen = (CHARACTER *) 0xB8000;
    int i;

    pstScreen += iY * 80 + iX;

    for (i = 0; pcString[i] != 0; i++) {
    	pstScreen[i].bCharacter = pcString[i];
    }
}

// initialize IA-32e mode kernel area with 0x00
// the area for IA-32e mode Kernel is [1MB, 6MB)
BOOL kInitializeKernel64Area(void) {
    DWORD* pdwCurrentAddress = (DWORD*) 0x100000;

    while ( (DWORD) pdwCurrentAddress < 0x600000) {
        *pdwCurrentAddress = 0x00;

        // there is possibility that computer pretends that is allows
        // to write to specific memory area. To make sure that the addr is
        // initialized, reading the addr is necessary
        if (*pdwCurrentAddress != 0) {
            return FALSE;
        }
        pdwCurrentAddress++;
    }
    return TRUE;
}

// Check if computer has enough memory address that MINT64OS can use.
// MINT64OS requires 64MB
BOOL kIsMemoryEnough(void) {
    // 0x100000 == 1MB
    DWORD* pdwCurrentAddress = (DWORD*) 0x100000;

    // loop up to 0x4000000(64MB)
    while ( (DWORD) pdwCurrentAddress < 0x4000000) {
        *pdwCurrentAddress = 0x12345678;

        // there is possibility that computer pretends that is allows
        // to write to specific memory area. To make sure that the addr exists,
        // reading the addr is necessary
        if (*pdwCurrentAddress != 0x12345678) {
            return FALSE;
        }

        // Do not check every single byte. Instead, check first byte of
        // every megabyte
        pdwCurrentAddress += (0x100000 / 4);
    }
    return TRUE;
}