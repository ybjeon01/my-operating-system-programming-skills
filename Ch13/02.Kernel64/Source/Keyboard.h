#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "Types.h"

// number of scan code to ignore when pause key is pressed
#define KEY_SKIPCOUNTFORPAUSE 2

// flag for key state
#define KEY_FLAGS_UP 0x00  // when key is released
#define KEY_FLAGS_DOWN 0x01 // when key is pressed
#define KEY_FLAGS_EXTENDEDKEY 0x02 // when extension key is pressed


// number of table mapping scan code to ASCII code
#define KEY_MAPPINGTABLEMAXCOUNT 89


// ASCII CODE in hex
#define KEY_NONE 0x00
#define KEY_ENTER '\n'
#define KEY_TAB '\t'
#define KEY_ESC 0x1B
#define KEY_BACKSPACE 0x08

#define KEY_CTRL        0x81
#define KEY_LSHIFT      0x82
#define KEY_RSHIFT      0x83
#define KEY_PRINTSCREEN 0x84
#define KEY_LALT        0x85
#define KEY_CAPSLOCK    0x86
#define KEY_F1          0x87
#define KEY_F2          0x88
#define KEY_F3          0x89
#define KEY_F4          0x8A
#define KEY_F5          0x8B
#define KEY_F6          0x8C
#define KEY_F7          0x8D
#define KEY_F8          0x8E
#define KEY_F9          0x8F
#define KEY_F10         0x90
#define KEY_NUMLOCK     0x91
#define KEY_SCROLLLOCK  0x92
#define KEY_HOME        0x93
#define KEY_UP          0x94
#define KEY_PAGEUP      0x95
#define KEY_LEFT        0x96
#define KEY_CENTER      0x97
#define KEY_RIGHT       0x98
#define KEY_END         0x99
#define KEY_DOWN        0x9A
#define KEY_PAGEDOWN    0x9B
#define KEY_INS         0x9C
#define KEY_DEL         0x9D
#define KEY_F11         0x9E
#define KEY_F12         0x9F
#define KEY_PAUSE       0xA0

#pragma pack(push ,1)

typedef struct kKeyMappingEntryStruct {
    // normal ASCII code that is not combined with shift or Caps Lock
    BYTE bNormalCode;
    // key combined with shift or Caps Lock
    BYTE bCombinedCode;
} KEYMAPPINGENTRY;

#pragma pack(pop)

typedef struct kKeyboardManagerStruct {
    // current state of keyboard
    BOOL bShiftDown;
    BOOL bCapsLockOn;
    BOOL bNumLockOn;
    BOOL bScrollLockOn;

    // Unlike normal scan code, extended scan code is two bytes and first byte
    // is 0xE0
    // Because only one byte can be processed at once, it is necessary to
    // set flag if first byte is extended code
    BOOL bExtendedCodeIn;
    // Pause key is unique extended scan code whose first byte starts with
    // 0xE1 and whose size is three bytes.
    // If this variable is set, kConvertScanCodeToASCIICode function ignores 
    // as many as the number specified in this variable
    int iSkipCountForPause;
} KEYBOARDMANAGER;


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

// private function that checks if scan code from keyboard is alphabet
// params:
//   bScanCode: byte code from keyboard
// return:
//   True if scan code is alphabet, otherwise False
BOOL kIsAlphabetscanCode(BYTE bScanCode);

// private function that checks if scan code is number or symbol
// this function excludes number pad
// params:
//   bScanCode: byte code from keyboard
// return:
//   True if scan code is not alphabet, otherwise False
BOOL kIsNumberOrSymbolScanCode(BYTE bScanCode);

// private function that checks if scan code is number pad
// params:
//   bScanCode: byte code from keyboard
// return:
//   True if scan code is from number pad, otherwise False
BOOL kIsNumberPadScanCode(BYTE bScanCode);

// private function that checks if combined key value should be used
// ex) Shift + a = uppercase a = A
// params:
//   bScanCode: byte code from keyboard
// return:
//   True if combined key value should be used
BOOL kShouldUseCombinedCode(BYTE bScanCode);

// private function that updates a global KeyboardManager structure that 
// contains current keyboard state.
// params:
//   bScanCode: byte code from keyboard
void updateCombinationKeyStatusAndLED(BYTE bScanCode);

// public function that analyze scan code based on current keyboard state
// params:
//   bScanCode: scan code from keyboard
//   pbASCIICode: address where ascii code corresponding to the scan code is
//                saved
//   phFlags: address where the scan code's characteristics are saved
// return:
//   True if analyzing scan code is finished, otherwise False
// info:
//   if return value is False, it is because the scan code to analyze is 
//   two or three bytes. To finish analyzing, read more scan code
//   from keyboard and call this function again with the new scan code
BOOL kConvertScanCodeToASCIICode(
    BYTE bScanCode,
    BYTE *pbASCIICode,
    BOOL *pbFlags);


#endif /* __KEYBOARD_H__ */