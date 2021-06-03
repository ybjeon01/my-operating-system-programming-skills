#include "Types.h"
#include "AssemblyUtility.h"
#include "Keyboard.h"
#include "Queue.h"
#include "Utility.h"

// function that checks if output buffer of PS/2 Controller is full.
// return:
//   True if output buffer is full. Otherwise, False
BOOL kIsOutputBufferFull(void) {
    // read state register from PS/2 Controller
    // if bit 0 is set to 1, PS/2 Controller filled buffer
    // with data sent by keyboard
    if (kInPortByte(0x64) & 0x01) {
        return TRUE;
    }
    return FALSE;
}


// function that checks if input buffer of PS/2 Controller is full.
// return:
//   True if input buffer is full. Otherwise, False
// info:
//   PS/2 Controller shares a register, called data register for input buffer
//   and output buffer.
//   this function is used when command is needed to sent to keyboard
BOOL kIsInputBufferFull(void) {
    // read state register from PS/2 Controller
    // if bit 1 is set to 1, the data in data register is not flushed to
    // keyboard yet
    if (kInPortByte(0x64) & 0x02) {
        return TRUE;
    }
    return FALSE;
}


// wait until keyboard gives ACK signal. if the data in output buffer is not
// ACK signal, put the scan code into queue and keep waiting for ACK signal
// return:
//   True if ACK is received. otherwise False after timeout
// info:
//   timeout is 100 keys * 0xFFFF counters
BOOL kWaitForACKAndPutOtherScanCode(void) {
    int i, j;
    BYTE bData;
    BOOL bResult = FALSE;

    // Because keyboard sends acknowledge code for every command,
    // it is necessary to wait for the code. it is possible that
    // output buffer is filled with some key data before ACK code,
    // transmitted data is checked 100 times
	for (j = 0; j < 100; j++) {
        // Keyboard is not as fast as CPU. it is necessary to wait for input
        // buffer is filled. 0xFFFF is just arbitrary number that I think is
        // enough loop counter for waiting. If buffer is full after 0xFFFF,
        // buffer is ignored and activation command is sent
		for (i = 0; i < 0xFFFF; i++) {
			if (kIsOutputBufferFull()) {
                break;
            }
		}

		bData = kInPortByte(0x60);
		// if data is ACK
		if (bData == 0xFA) {
			bResult = TRUE;
			break;
		}
		// if data is not ACK, convert scan code to ASCII code and put the
		// ASCII code to keyboard queue
		else {
            kConvertScanCodeAndPutQueue(bData);
		}
	}
	return bResult;
}


// activate keyboard, so user can get data from keyboard
// info:
//   this function activates PS/2 Controller and
//   keyboard itself
// return:
//   True if keyboard successfully activated. Otherwise False
BOOL kActivateKeyboard(void) {
    int i, j;
    BOOL bPreviousInterrupt;
    BOOL bResult;

    // disable all interrupt while working with queue and keyboard
    bPreviousInterrupt = kSetInterruptFlag(FALSE);


    // activate keyboard feature of PS/2 Controller
    // writing to 0x64: controller register
    // reading from 0x64: command register
    kOutPortByte(0x64, 0xAE);

    // Keyboard is not as fast as CPU. it is necessary to wait for input buffer
    // is filled. 0xFFFF is just arbitrary number that I think is enough
    // loop counter for waiting. If buffer is full after 0xFFFF, buffer is
    // ignored and activation command is sent
    for (i = 0; i < 0xFFFF; i++) {
        if (!kIsInputBufferFull()) {
            break;
        }
    }

    // send keyboard activation command to keyboard
    kOutPortByte(0x60, 0xF4);

    bResult = kWaitForACKAndPutOtherScanCode();
    kSetInterruptFlag(bPreviousInterrupt);
    return bResult;
}


// read byte from output buffer
// caution:
//   if there is no data in output buffer, computer is freezed
BYTE kGetKeyboardScanCode(void) {
    while (!kIsOutputBufferFull());
    // read from output buffer (data register)
    return kInPortByte(0x60);
}


// switch keyboard LED on or off
BOOL kChangeKeyboardLED(
    BOOL bCapsLockOn,
    BOOL bNumLockOn,
    BOOL bScrollLockOn
) {
    int i, j;
    BOOL bPreviousInterrupt;
    BOOL bResult;
    BOOL bData;

    bPreviousInterrupt = kSetInterruptFlag(FALSE);

    // wait for input buffer to be empty
    for (i = 0; i < 0xFFFF; i++) {
        if (!kIsInputBufferFull()) {
            break;
        }
    }
    // keyboard command that signals LED change: 0xED
    kOutPortByte(0x60, 0xED);

    // wait until command was flushed into keyboard
    for (i = 0; i < 0xFFFF; i++) {
        if (!kIsInputBufferFull()) {
            break;
        }
    }

    // wait for ACK code from keyboard
    bResult = kWaitForACKAndPutOtherScanCode();
    if (bResult == FALSE) {
        kSetInterruptFlag(bPreviousInterrupt);
        return FALSE;
    }

    // send wanted LED state to keyboard
    kOutPortByte(0x60, bCapsLockOn << 2 | bNumLockOn << 1 | bScrollLockOn);

    // wait until data was flushed into keyboard
    for (i = 0; i < 0xFFFF; i++) {
        if (!kIsInputBufferFull()) {
            break;
        }
    }

    // wait for ACK code from keyboard
    bResult = kWaitForACKAndPutOtherScanCode();
    kSetInterruptFlag(bPreviousInterrupt);
    return bResult;
}


// enable A20 Gate through PS/2 Controller to access bigger memory address
// info:
//   one of three ways to activate A20 Gate
//   1. BIOS service
//   2. I/O port
//   3. PS/2 Controller
void kEnableA20Gate(void) {
    BYTE kOutputPortData;
    int i;
    BOOL bPreviousInterrupt;

    bPreviousInterrupt = kSetInterruptFlag(FALSE);

    // In PS/2 Controller, there is a output port that you cannot access by
    // I/O port address. two keyboard controller commands(0xD0/0xD1) sends
    // data in the port to output buffer and vice versa
    kOutPortByte(0x64, 0xD0);

    // wait for output buffer to be filled with keyboard controller output
    // port data
    for (i = 0; i < 0xFFFF; i++) {
        if (kIsOutputBufferFull()) {
            break;
        }
    }

    // read output port data
    kOutputPortData = kInPortByte(0x60);
    // set A20 gate bit
    kOutputPortData |= 0x01;

    // wait for input buffer to be empty
    for (i = 0; i < 0xFFFF; i++) {
        if (!kIsInputBufferFull()) {
            break;
        }
    }

    // send data in input buffer to output port
    kOutPortByte(0x64, 0xD1);
    kOutPortByte(0x60, kOutputPortData);

    kSetInterruptFlag(bPreviousInterrupt);
}

// reset processor to reboot computer
void kReboot(void) {
    int i;
    BOOL bPreviousInterrupt;

    bPreviousInterrupt = kSetInterruptFlag(FALSE);

    // wait for input buffer to be empty
    for (i = 0; i < 0xFFFF; i++) {
        if (!kIsInputBufferFull()) {
            break;
        }
    }

    // send data in output port to output buffer
    kOutPortByte(0x64, 0xD1);
    // set bit 0 for rebooting
    kOutPortByte(0x60, 0x00);

    // loop until keyboard controller command is processed
    while (1);
}


///////////////////////////////////////////////////////////////////////////////
// table that convert scan code to ASCII code and related functions
///////////////////////////////////////////////////////////////////////////////

// keyboard manager that stores states of keyboard
static KEYBOARDMANAGER gs_stKeyboardManager = {0, };

static QUEUE gs_stKeyQueue;
static KEYDATA gs_vstKeyQueueBuffer[KEY_MAXQUEUECOUNT];

// table for converting scan code to ASCII code
static KEYMAPPINGENTRY gs_vstKeyMappingTable[ KEY_MAPPINGTABLEMAXCOUNT ] =
{
    /*  0   */  {   KEY_NONE        ,   KEY_NONE        },
    /*  1   */  {   KEY_ESC         ,   KEY_ESC         },
    /*  2   */  {   '1'             ,   '!'             },
    /*  3   */  {   '2'             ,   '@'             },
    /*  4   */  {   '3'             ,   '#'             },
    /*  5   */  {   '4'             ,   '$'             },
    /*  6   */  {   '5'             ,   '%'             },
    /*  7   */  {   '6'             ,   '^'             },
    /*  8   */  {   '7'             ,   '&'             },
    /*  9   */  {   '8'             ,   '*'             },
    /*  10  */  {   '9'             ,   '('             },
    /*  11  */  {   '0'             ,   ')'             },
    /*  12  */  {   '-'             ,   '_'             },
    /*  13  */  {   '='             ,   '+'             },
    /*  14  */  {   KEY_BACKSPACE   ,   KEY_BACKSPACE   },
    /*  15  */  {   KEY_TAB         ,   KEY_TAB         },
    /*  16  */  {   'q'             ,   'Q'             },
    /*  17  */  {   'w'             ,   'W'             },
    /*  18  */  {   'e'             ,   'E'             },
    /*  19  */  {   'r'             ,   'R'             },
    /*  20  */  {   't'             ,   'T'             },
    /*  21  */  {   'y'             ,   'Y'             },
    /*  22  */  {   'u'             ,   'U'             },
    /*  23  */  {   'i'             ,   'I'             },
    /*  24  */  {   'o'             ,   'O'             },
    /*  25  */  {   'p'             ,   'P'             },
    /*  26  */  {   '['             ,   '{'             },
    /*  27  */  {   ']'             ,   '}'             },
    /*  28  */  {   '\n'            ,   '\n'            },
    /*  29  */  {   KEY_CTRL        ,   KEY_CTRL        },
    /*  30  */  {   'a'             ,   'A'             },
    /*  31  */  {   's'             ,   'S'             },
    /*  32  */  {   'd'             ,   'D'             },
    /*  33  */  {   'f'             ,   'F'             },
    /*  34  */  {   'g'             ,   'G'             },
    /*  35  */  {   'h'             ,   'H'             },
    /*  36  */  {   'j'             ,   'J'             },
    /*  37  */  {   'k'             ,   'K'             },
    /*  38  */  {   'l'             ,   'L'             },
    /*  39  */  {   ';'             ,   ':'             },
    /*  40  */  {   '\''            ,   '\"'            },
    /*  41  */  {   '`'             ,   '~'             },
    /*  42  */  {   KEY_LSHIFT      ,   KEY_LSHIFT      },
    /*  43  */  {   '\\'            ,   '|'             },
    /*  44  */  {   'z'             ,   'Z'             },
    /*  45  */  {   'x'             ,   'X'             },
    /*  46  */  {   'c'             ,   'C'             },
    /*  47  */  {   'v'             ,   'V'             },
    /*  48  */  {   'b'             ,   'B'             },
    /*  49  */  {   'n'             ,   'N'             },
    /*  50  */  {   'm'             ,   'M'             },
    /*  51  */  {   ','             ,   '<'             },
    /*  52  */  {   '.'             ,   '>'             },
    /*  53  */  {   '/'             ,   '?'             },
    /*  54  */  {   KEY_RSHIFT      ,   KEY_RSHIFT      },
    /*  55  */  {   '*'             ,   '*'             },
    /*  56  */  {   KEY_LALT        ,   KEY_LALT        },
    /*  57  */  {   ' '             ,   ' '             },
    /*  58  */  {   KEY_CAPSLOCK    ,   KEY_CAPSLOCK    },
    /*  59  */  {   KEY_F1          ,   KEY_F1          },
    /*  60  */  {   KEY_F2          ,   KEY_F2          },
    /*  61  */  {   KEY_F3          ,   KEY_F3          },
    /*  62  */  {   KEY_F4          ,   KEY_F4          },
    /*  63  */  {   KEY_F5          ,   KEY_F5          },
    /*  64  */  {   KEY_F6          ,   KEY_F6          },
    /*  65  */  {   KEY_F7          ,   KEY_F7          },
    /*  66  */  {   KEY_F8          ,   KEY_F8          },
    /*  67  */  {   KEY_F9          ,   KEY_F9          },
    /*  68  */  {   KEY_F10         ,   KEY_F10         },
    /*  69  */  {   KEY_NUMLOCK     ,   KEY_NUMLOCK     },
    /*  70  */  {   KEY_SCROLLLOCK  ,   KEY_SCROLLLOCK  },

    /*  71  */  {   KEY_HOME        ,   '7'             },
    /*  72  */  {   KEY_UP          ,   '8'             },
    /*  73  */  {   KEY_PAGEUP      ,   '9'             },
    /*  74  */  {   '-'             ,   '-'             },
    /*  75  */  {   KEY_LEFT        ,   '4'             },
    /*  76  */  {   KEY_CENTER      ,   '5'             },
    /*  77  */  {   KEY_RIGHT       ,   '6'             },
    /*  78  */  {   '+'             ,   '+'             },
    /*  79  */  {   KEY_END         ,   '1'             },
    /*  80  */  {   KEY_DOWN        ,   '2'             },
    /*  81  */  {   KEY_PAGEDOWN    ,   '3'             },
    /*  82  */  {   KEY_INS         ,   '0'             },
    /*  83  */  {   KEY_DEL         ,   '.'             },
    /*  84  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  85  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  86  */  {   KEY_NONE        ,   KEY_NONE        },
    /*  87  */  {   KEY_F11         ,   KEY_F11         },
    /*  88  */  {   KEY_F12         ,   KEY_F12         }
};

// function that check if scan code from keyboard is alphabet
// params:
//   bScanCode: byte code from keyboard
// return:
//   True if scan code is alphabet, otherwise False
BOOL kIsAlphabetScanCode(BYTE bScanCode) {
    if (
        ('a' <= gs_vstKeyMappingTable[bScanCode].bNormalCode) &&
        (gs_vstKeyMappingTable[bScanCode].bNormalCode <= 'z')
    ) {
        return TRUE;
    }
    return FALSE;
}

// function that checks if scan code is number or symbol
// this function excludes number pad
// params:
//   bScanCode: byte code from keyboard
// return:
//   True if scan code is not alphabet, otherwise False
BOOL kIsNumberOrSymbolScanCode(BYTE bScanCode) {
    if (2 <= bScanCode && bScanCode <= 53 &&
         kIsAlphabetScanCode(bScanCode) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

// function that checks if scan code is number pad
// params:
//   bScanCode: byte code from keyboard
// return:
//   True if scan code is from number pad, otherwise False
BOOL kIsNumberPadScanCode(BYTE bScanCode) {
    if (71 <= bScanCode && bScanCode <= 83) {
        return TRUE;
    }
    return FALSE;
}

// function that checks if combined key value should be used
// ex) Shift + a = uppercase a = A
// params:
//   bScanCode: byte code from keyboard
// return:
//   True if combined key value should be used
BOOL kShouldUseCombinedCode(BYTE bScanCode) {
    BYTE bDownScanCode;
    BOOL bUseCombinedKey = FALSE;

    // 0x7F = 0b01111111
    // released key's scan code is pressed key's scan code with
    // bit 8 set
    bDownScanCode = bScanCode & 0x7F;

    // alphabet section
    if (kIsAlphabetScanCode(bDownScanCode)) {
        // if either one of Shift key or Caps Lock is pressed, then
        // return true
        if (
            gs_stKeyboardManager.bShiftDown ^
            gs_stKeyboardManager.bCapsLockOn
        ) {
            return TRUE;
        }
        return FALSE;
    }

    // number and symbols section
    if (kIsNumberOrSymbolScanCode(bDownScanCode)) {
        if (gs_stKeyboardManager.bShiftDown) {
            return TRUE;
        }
        return FALSE;
    }

    // In case that Num Lock is activated, number pad except
    // extended number pad scan code is affected by shift key
    if (kIsNumberPadScanCode(bDownScanCode) &&
        gs_stKeyboardManager.bExtendedCodeIn == FALSE) {
        if (gs_stKeyboardManager.bNumLockOn) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    // should not hit here. Just in case I made a mistake in code
    return FALSE;
}

// function that updates a global KeyboardManager structure that 
// contains current keyboard state.
// params:
//   bScanCode: byte code from keyboard
void updateCombinationKeyStatusAndLED(BYTE bScanCode) {
    BOOL bDown;
    BYTE bDownScanCode;
    BOOL bLEDStatusChanged = FALSE;

    // check if key is pressed or released by checking the last bit of
    // scan code (0x10000000)
    if (bScanCode & 0x80) {
        bDown = FALSE;
        bDownScanCode = bScanCode & 0x7F;
    }
    else {
        bDown = TRUE;
        bDownScanCode = bScanCode;
    }

    // Pressed left and right shift scan code
    if (bDownScanCode == 42 || bDownScanCode == 54) {
        gs_stKeyboardManager.bShiftDown = bDown;
    }
    // Pressed caps lock scan code
    else if (bDownScanCode == 58 && bDown == TRUE) {
        gs_stKeyboardManager.bCapsLockOn ^= TRUE;
        bLEDStatusChanged = TRUE;
    }
    // pressed num lock scan code
    else if (bDownScanCode == 69 && bDown == TRUE) {
        gs_stKeyboardManager.bNumLockOn ^= TRUE;
        bLEDStatusChanged = TRUE;
    }
    // pressed scroll lock scan code
    else if (bDownScanCode == 70 && bDown == TRUE) {
        gs_stKeyboardManager.bScrollLockOn ^= TRUE;
        bLEDStatusChanged = TRUE;
    }

    // send command to keyboard if LED status is changed
    if (bLEDStatusChanged) {
        kChangeKeyboardLED(gs_stKeyboardManager.bCapsLockOn,
                            gs_stKeyboardManager.bNumLockOn,
                          gs_stKeyboardManager.bScrollLockOn);
    }
}

// function that analyze scan code based on current keyboard state
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
    BOOL *pbFlags) {
    
    BOOL bUseCombinedKey;

    // if pause key was pressed, ignore scan code as many as
    // number of pressed paused key
    if (gs_stKeyboardManager.iSkipCountForPause > 0) {
        gs_stKeyboardManager.iSkipCountForPause--;
        return FALSE;
    }

    // Pause scan code
    // Pause is a unique extension key that starts with 0xE1
    // Because its size is three bytes and only key with 0xE1,
    // this function ignores next two incoming bytes
    if (bScanCode == 0xE1) {
        *pbASCIICode = KEY_PAUSE;
        *pbFlags = KEY_FLAGS_DOWN;
        gs_stKeyboardManager.iSkipCountForPause = KEY_SKIPCOUNTFORPAUSE;
        return TRUE;
    }
    // Extended scan code
    // Extension codes except pause start with 0xE0 and
    // their size is two bytes
    else if (bScanCode == 0xE0) {
        gs_stKeyboardManager.bExtendedCodeIn = TRUE;
        return FALSE;
    }

    // check if it should return combined key
    bUseCombinedKey = kShouldUseCombinedCode(bScanCode);

    if (bUseCombinedKey) {
        *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bCombinedCode;
    }
    else {
        *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bNormalCode;
    }

    // check if extension key
    if (gs_stKeyboardManager.bExtendedCodeIn) {
        *pbFlags = KEY_FLAGS_EXTENDEDKEY;
        gs_stKeyboardManager.bExtendedCodeIn = FALSE;
    }
    else {
        *pbFlags = 0;
    }

    // remember that "==", equal operator, precedes "&" bitwise operator
    // if you do not enclose "bScanCode & 0x80" with parentheses,
    // the pbFlags does not include KEY_FLAGS_DOWN
    if ( (bScanCode & 0x80) == 0) {
        *pbFlags |= KEY_FLAGS_DOWN;
    }

    // update combined key press or release state
    updateCombinationKeyStatusAndLED(bScanCode);

    return TRUE;
}


// function that initializes keyboard buffer and activates
// keyboard controller and keyboard
// return:
//   True if success. Otherwise False
BOOL kInitializeKeyboard(void) {
	// initialize queue
	kInitializeQueue(
        &gs_stKeyQueue,
        gs_vstKeyQueueBuffer,
        KEY_MAXQUEUECOUNT,
		sizeof(KEYDATA)
    );
    // activate keyboard controller and keyboard 
	return kActivateKeyboard();
}

// function that converts scan code to internal KeyData data structure
// and pushs the KeyData struct to queue
// params:
//   bScanCode: scan code from keyboard
// return:
//   True if succeed. Otherwise, False
BOOL kConvertScanCodeAndPutQueue(BYTE bScanCode) {
	KEYDATA stData;
	BOOL bResult = FALSE;
	BOOL bPreviousInterrupt;

	stData.bScanCode = bScanCode;

    BOOL converted = kConvertScanCodeToASCIICode(
        bScanCode,
        &(stData.bASCIICode),
        &(stData.bFlags)
    );  

	if (converted) {
		// disable interrupt
		bPreviousInterrupt = kSetInterruptFlag(FALSE);
		bResult = kPutQueue(&gs_stKeyQueue, &stData);
		// restore previous interrupt
		kSetInterruptFlag(bPreviousInterrupt);
	}
	return bResult;
}

// function that gets key data from keyboard buffer
// params:
//   pstData: pointer to variable will hold the data from the buffer
// return:
//   True if succeed. Otherwise, False
BOOL kGetKeyFromKeyQueue(KEYDATA *pstData) {
	BOOL bResult;
	BOOL bPreviousInterrupt;

	if (kIsQueueEmpty(&gs_stKeyQueue) == TRUE) {
		return FALSE;
	}

	// disable Interrupt
	bPreviousInterrupt = kSetInterruptFlag(FALSE);
	// get data from queue
	bResult = kGetQueue(&gs_stKeyQueue, pstData);
	// restore interrupt state
	kSetInterruptFlag(bPreviousInterrupt);
	return bResult;
}
