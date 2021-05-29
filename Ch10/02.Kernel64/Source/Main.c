#include "Types.h"

void kPrintString(int iX, int iY, const char *pcString);

void Main(void) {
    kPrintString(0, 10, "Switch To IA-32e Mode Success~!!");
    kPrintString(0, 11, "IA-32e C Language Kernel Start............. [PASS]");
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
void kPrintString(int iX, int iY, const char *pcString) {
	CHARACTER *pstScreen = (CHARACTER *) 0xB8000;
    int i;

    pstScreen += iY * 80 + iX;

    for (i = 0; pcString[i] != 0; i++) {
    	pstScreen[i].bCharacter = pcString[i];
    }
}
