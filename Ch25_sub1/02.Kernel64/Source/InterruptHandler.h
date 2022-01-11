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


// PIT counter0 Interrupt Handler. This function calls task scheduler
// params:
//   iVectorNumber: IDT gate descriptor index number
void kTimerHandler(int iVectorNumber);


// FPU device-not-available exception handler
// params:
//   iVectorNumber: IDT gate descriptor index number
// info:
//   if TS bit in CR0 was set by scheduler, this function restores
//   FPU context
void kDeviceNotAvailableHandler(int iVectorNumber);


// pata HDD read/write ready interrupt handler
// params:
//   iVectorNumber: IDT gate descriptor index number
void kHDDHandler(int iVectorNumber);

#endif /* __INTERRUPTHANDLER_H__ */