#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "Types.h"

// check if output buffer of PS/2 Controller is full.
BOOL kIsOutputBufferFull(void);

// check if input buffer of PS/2 Controller is full.
// info:
//   PS/2 Controller shares a register, called data register for input buffer
//   and output buffer.
//   this function is used when command is needed to sent to keyboard
BOOL kIsInputBufferFull(void);

// activate keyboard, so user can get data from keyboard
// info:
//   this function activates PS/2 Controller and
//   keyboard itself
BOOL kActivateKeyboard(void);

// read byte from output buffer
// caution:
//   if there is no data in output buffer, computer is freezed
BYTE kGetKeyboardScanCode(void);

// switch keyboard LED on or off
BOOL kChangeKeyboardLED(BOOL bCapsLockOn, BOOL bNumLokOn, BOOL bScrollLockOn);

// enable A20 Gate through PS/2 Controller to access bigger memory address
// info:
//   one of three ways to activate A20 Gate
//   1. BIOS service
//   2. I/O port
//   3. PS/2 Controller
void kEnableA20Gate(void);

// reset processor to reboot computer
void kReboot(void);

#endif /* __KEYBOARD_H__ */