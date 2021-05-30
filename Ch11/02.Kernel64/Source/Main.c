#include "Types.h"
#include "Keyboard.h"

void kPrintString(int iX, int iY, const char *pcString);

void Main(void) {
    kPrintString(0, 10, "Switch To IA-32e Mode Success~!!");
    kPrintString(0, 11, "IA-32e C Language Kernel Start............. [PASS]");
    kPrintString(0, 12, "Keyboard Activate...........................[    ]");
    
    // activate keyboard
    // if keyboard input buffer is full after 0xFFFF counters or does not
    // response with ACK code, activation fails
    if (kActivateKeyboard()) {
    	kPrintString(45, 12, "Pass");
    	// set Num Lock, Caps Lock, Scroll Lock off
    	kChangeKeyboardLED(FALSE, FALSE, FALSE);
    }
    else {
    	kPrintString(45, 12, "Fail");
    	while (1);
    }

    // Simple shell section
    char vcTemp[2] = {0, };
    BYTE bFlags;
    BYTE bTemp;
    int i = 0; 
    while (TRUE) {
        // if key is sent from keyboard
    	if (kIsOutputBufferFull()) {
    		bTemp = kGetKeyboardScanCode();
    		// convert scan code to ASCII code
            if (kConvertScanCodeToASCIICode(bTemp, &(vcTemp[0]), &bFlags)) {
                // print only when key is pressed. In other word,
                // do not print when key is released
            	if (bFlags & KEY_FLAGS_DOWN) {
					kPrintString(i++, 13, vcTemp);
				}
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
void kPrintString(int iX, int iY, const char *pcString) {
	CHARACTER *pstScreen = (CHARACTER *) 0xB8000;
    int i;

    pstScreen += iY * 80 + iX;

    for (i = 0; pcString[i] != 0; i++) {
    	pstScreen[i].bCharacter = pcString[i];
    }
}
