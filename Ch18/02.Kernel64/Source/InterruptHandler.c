#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Task.h"
#include "Utility.h"

// common exception handler for exceptions that do not have handler
// info:
//    This handler is for reserved exception/interrupt: 0 ~ 31
// params:
//   iVectorNumber: IDT gate descriptor index number
//   qwErrorCode: error code for some exceptions.
void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode) {
    char vcBuffer[3] = {0, };

    // interrupt number as ASCII two-digit number
    vcBuffer[0] = '0' + iVectorNumber / 10;
    vcBuffer[1] = '0' + iVectorNumber % 10;

    kPrintStringXY(0, 0, "==================================================");
    kPrintStringXY(0, 1, "               Exception Occur~!!!!               ");
    kPrintStringXY(0, 2, "                    Vector:                       ");
    kPrintStringXY(27, 2, vcBuffer);
    kPrintStringXY(0, 3, "==================================================");

    while (1);
}

// common Interrupt handler for interrupts that do not have handler
// info:
//   This handler is for interrupts whose vector number is over 31
// params:
//   iVectorNumber: IDT gate descriptor index number
void kCommonInterruptHandler(int iVectorNumber) {
	char vcBuffer[] = "[INT:  , ]";
	// count how many interrupt occurs
	static int g_iCommonInterruptCount = 0;

	// interrupt number as ASCII two-digit number
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// interrupt count
	vcBuffer[8] = '0' + g_iCommonInterruptCount;
	g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
	kPrintStringXY(70, 0, vcBuffer);

	// send EOI to PIC controller
    // iVectorNumber is not IRQ number, so it is necessary to
    // subtract iVectorNumber by first IDT descriptor index
    // coressponding to first IRQ
	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

// PIC keyboard Interrupt Handler. This function prints how many this
// interrupt is received from booting to the top right corner
// params:
//   iVectorNumber: IDT gate descriptor index number
void kKeyboardHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ]";
    // count how many interrupt occurs
    static int g_iCommonInterruptCount = 0;
    BYTE bTemp;

    // interrupt number as ASCII two-digit number
    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;

    // interrupt count
    vcBuffer[8] = '0' + g_iCommonInterruptCount;
    g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
    kPrintStringXY(59, 0, vcBuffer);

    if (kIsOutputBufferFull()) {
        bTemp = kGetKeyboardScanCode();
        kConvertScanCodeAndPutQueue(bTemp);
    }

    // send EOI to PIC controller
    // reason for iVectorNumber - PIC_IRQSTARTVECTOR is that
    // iVectorNumber is number defined in ISR.
    // iVectorNumber is not IRQ number
    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}


// PIT counter0 Interrupt Handler. This function calls task scheduler
// params:
//   iVectorNumber: IDT gate descriptor index number
void kTimerHandler(int iVectorNumber) {
    char vcBuffer[] = "[INT:  , ]";
	// count how many interrupt occurs
	static int g_iCommonInterruptCount = 0;

	// interrupt number as ASCII two-digit number
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// interrupt count
	vcBuffer[8] = '0' + g_iCommonInterruptCount;
	g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
	kPrintStringXY(70, 0, vcBuffer);

	// send EOI to PIC controller
    // iVectorNumber is not IRQ number, so it is necessary to
    // subtract iVectorNumber by first IDT descriptor index
    // coressponding to first IRQ
	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);

    g_qwTickCount++;

    kDecreaseProcessorTime();
    if (kIsProcessorTimeExpired()) {
        kScheduleInInterrupt();
    }
}