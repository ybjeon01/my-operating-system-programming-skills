#ifndef __INTERRUPTHANDLER_H__
#define __INTERRUPTHANDLER_H__

#include "Types.h"

// common exception handler for exceptions that do not have handler
// info:
//    This handler is for reserved exception/interrupt: 0 ~ 31
// params:
//   iVectorNumber: IDT gate descriptor index number
//   qwErrorCode: error code for some exceptions.
void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode);

// common Interrupt handler for interrupts that do not have handler
// info:
//   This handler is for interrupts whose vector number is over 31
// params:
//   iVectorNumber: IDT gate descriptor index number
void kCommonInterruptHandler(int iVectorNumber);

// PIC keyboard Interrupt Handler. This function prints how many this
// interrupt is received from booting to the top right corner
// params:
//   iVectorNumber: IDT gate descriptor index number
void kKeyboardHandler(int iVectorNumber);

#endif /* __INTERRUPTHANDLER_H__ */