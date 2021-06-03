#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "PIC.h"

void kPrintString(int iX, int iY, const char *pcString);

void Main(void) {
    kPrintString(0, 10, "Switch To IA-32e Mode Success~!!");
    kPrintString(0, 11, "IA-32e C Language Kernel Start............. [PASS]");


    /* 
     * load GDT again and load IDT for handling interrupts 
     * This GDT is at 1MB and contains segment descriptors and TSS descriptor
     * TSS descriptor is for offering stack to interrupt handler
     */ 

    kPrintString(0, 12, "GDT Switch For IA-32e Mode..................[    ]");
    kInitializeGDTTableAndTSS();
    kLoadGDTR(GDTR_STARTADDRESS);
    kPrintString(45, 12, "Pass");

    kPrintString(0, 13, "TSS Segment Load............................[    ]");
    kLoadTR(GDT_TSSSEGMENT);
    kPrintString(45, 13, "PASS");

    kPrintString(0, 14, "IDT Initialization..........................[    ]");
    kInitializeIDTTables();
    kLoadIDTR(IDTR_STARTADDRESS);
    kPrintString(45, 14, "PASS");


    /* Activate keyboard */

    kPrintString(0, 15, "Keyboard Activate...........................[    ]");

    // if keyboard input buffer is full after 0xFFFF counters or does not
    // response with ACK code, activation fails
    if (kActivateKeyboard()) {
        kPrintString(45, 15, "Pass");
        // set Num Lock, Caps Lock, Scroll Lock off
        kChangeKeyboardLED(FALSE, FALSE, FALSE);
    }
    else {
        kPrintString(45, 15, "Fail");
        while (1);
    }


    /* Initialize PIC controller */

    kPrintString(0, 16, "PIC Controller And Interrupt Initialize.....[    ]");
    kInitializePIC();
    // unmask all interrupts
    kMaskPICInterrupt(0);
    // interrupt was deactivated in 01.Kernel32/EntryPoint.s 
    kEnableInterrupt();
    kPrintString(45, 16, "Pass");    


    /* very simple shell */

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
                    kPrintString(i++, 17, vcTemp);

                    // cause zero division exception to test that
                    // interrupt-related code is working 
                    if (vcTemp[0] == '0') {
                        bTemp = bTemp / 0;
                    }
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
