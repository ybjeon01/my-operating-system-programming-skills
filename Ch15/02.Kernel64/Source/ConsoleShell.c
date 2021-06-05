#include "Types.h"
#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"

SHELLCOMMANDENTRY gs_vstCommandTable[] = {
    {"help", "Show Help", kHelp},
    {"cls", "Clear Screen", kCls},
    {"totalram", "Show Total RAM Size", kShowTotalRAMSize},
    {"strtod", "String to Decimal/Hex Convert", kStringToDecimalHexTest},
    {"shutdown", "Shutdown And Reboot OS", kShutdown},
};

// main loop of shell
void kStartConsoleShell(void) {
    // command buffer
    char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
    int iCommandBufferIndex = 0;

    // key from keyboard queue
    BYTE bKey;
    
    // console cursor loc
    int iCursorX, iCursorY;

    // print prompt
    kPrintf(CONSOLESHELL_PROMPTMESSAGE);

    while (TRUE) {
        // wait until key is received
        // no timeout
        bKey = kGetCh();

        // backspace: delete prev text
        if (bKey == KEY_BACKSPACE) {
            if (iCommandBufferIndex > 0) {
                kGetCursor(&iCursorX, &iCursorY);
                kPrintStringXY(iCursorX - 1, iCursorY, " ");
                kSetCursor(iCursorX - 1, iCursorY);
                iCommandBufferIndex--;
            }
        }
        // enter: execute command
        else if (bKey == KEY_ENTER) {
            kPrintf("\n");
            if (iCommandBufferIndex > 0) {
                vcCommandBuffer[iCommandBufferIndex] ='\0';
                kExecuteCommand(vcCommandBuffer);
            }
            // after executing command
            kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
            // initialize command buffer and index again
            kMemSet(vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
            iCommandBufferIndex = 0;
        }
        // ignore shift, caps lock, num lock, and scroll lock
        else if (
            (bKey == KEY_LSHIFT) || \
            (bKey == KEY_RSHIFT) || \
            (bKey == KEY_CAPSLOCK) || \
            (bKey == KEY_NUMLOCK) || \
            (bKey == KEY_SCROLLLOCK)
        ) {
            ;
        }
        else {
            if (bKey == KEY_TAB) {
                bKey = ' ';
            }
            if (iCommandBufferIndex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT) {
                vcCommandBuffer[iCommandBufferIndex++] = bKey;
                kPrintf("%c", bKey);
            }
        }
    }
}

// function that search command and execute
// params:
//   pcCommandBuffer: buffer that contains command string 
void kExecuteCommand(const char *pcCommandBuffer) {
    int i, iSpaceIndex;
    int iCommandBufferLength, iCommandLength;
    int iCount;

    // extract command part from string buffer by a space
    iCommandBufferLength = kStrLen(pcCommandBuffer);
    for (iSpaceIndex = 0; iSpaceIndex < iCommandBufferLength; iSpaceIndex++) {
        if (pcCommandBuffer[iSpaceIndex] == ' ') {
            break;
        }
    }

    // search command from list
    iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);
    for (i = 0; i < iCount; i++) {
        iCommandLength = kStrLen(gs_vstCommandTable[i].pcCommand);

        BOOL sameLength = iCommandLength == iSpaceIndex;
        BOOL sameCommand = kMemCmp(
            gs_vstCommandTable[i].pcCommand,
            pcCommandBuffer,
            iSpaceIndex
        );

        if (sameLength && (sameCommand == 0)) {
            gs_vstCommandTable[i].pfFunction(pcCommandBuffer + iSpaceIndex + 1);
            break;
        }
    }

    if (iCount <= i) {
        kPrintf("'%s' is not found.\n", pcCommandBuffer);
    }
}


// function that initializes parameter list
// params:
//   pstList: parameter list
//   pcParmater: string buffer that contains parameters
// info:
//   this function is used inside command function to extract parameters
//   from string buffer
void kInitializeParameter(PARAMETERLIST *pstList, const char *pcParameter) {
    pstList->pcBuffer = pcParameter;
    pstList->iLength = kStrLen(pcParameter);
    pstList->iCurrentPosition = 0;
}


// function that gets next parameter from parameter list
// params:
//   pstList: parameter list
//   pcParmater: string buffer that will hold next parameter string
// return:
//   length of next parameter string
//   if no more paramters, zero is return
// info:
//   this function is used inside command function to extract parameters
//   from string buffer
int kGetNextParameter(PARAMETERLIST *pstList, char *pcParameter) {
    int i;
    int iLength;

    if (pstList->iLength <= pstList->iCurrentPosition) {
        return 0;
    }

    for (i = pstList->iCurrentPosition; i < pstList->iLength; i++) {
        if (pstList->pcBuffer[i] == ' ') {
            break;
        }
    }

    kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i);
    iLength = i - pstList->iCurrentPosition;
    pcParameter[i] = '\0';

    pstList->iCurrentPosition += iLength + 1;
    return iLength;
}


// shell command that prints available commands
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kHelp does have any parameters
void kHelp(const char *pcCommandBuffer) {
    int i;
    int iCount;  // number of commands
    int iCursorX, iCursorY;
    int iLength, iMaxCommandLength = 0; // longest command length

    kPrintf("=========================================================\n");
    kPrintf("                    MINT64 Shell Help                    \n");
    kPrintf("=========================================================\n");

    iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);

    // count the length of the longest command
    for (i = 0; i < iCount; i++) {
        iLength = kStrLen(gs_vstCommandTable[i].pcCommand);
        if (iLength > iMaxCommandLength) {
            iMaxCommandLength = iLength;
        }
    }

    // print help string for each command
    for (i = 0; i < iCount; i++) {
        kPrintf("%s", gs_vstCommandTable[i].pcCommand);
        
        kGetCursor(&iCursorX, &iCursorY);
        kSetCursor(iMaxCommandLength, iCursorY);
        kPrintf("  - %s\n", gs_vstCommandTable[i].pcHelp);
    }
}


// clear screen
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kCls does have any parameters
void kCls(const char *pcParameterBuffer) {
    // the first line is used for debugging.
    // move cursor to second line
    kClearScreen();
    kSetCursor(0, 1);
}


// show total ram size of the computer
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kCls does have any parameters
void kShowTotalRAMSize(const char *pcParameterBuffer) {
    kPrintf("Total RAM Size = %d MB\n", kGetTotalRAMSize());
}


// check if given parameter is hex or decimal
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   parameters: strings only with number
void kStringToDecimalHexTest(const char *pcParameterBuffer) {
    char vcParameter[100];
    int iLength;
    PARAMETERLIST stList;
    int iCount = 0;
    long lValue;
    
    // initialize paramter list
    kInitializeParameter(&stList, pcParameterBuffer);
    
    while(TRUE) {
        // get next parameter. If there is no parameter, exit
        iLength = kGetNextParameter(&stList, vcParameter);
        if(iLength == 0) {
            break;
        }

        kPrintf(
            "Param %d = '%s', Length = %d, ",
            iCount + 1,
            vcParameter,
            iLength
        );

        // check if string is hex or decimal by checking first two chracters: 0x
        if(kMemCmp(vcParameter, "0x", 2) == 0) {
            lValue = kAToI(vcParameter + 2, 16);
            kPrintf("HEX Value = %q\n", lValue);
        }
        else {
            lValue = kAToI(vcParameter, 10);
            kPrintf("Decimal Value = %d\n", lValue);
        }
        iCount++;
    }
}


// reboot computer
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kShutdown does have any parameters
void kShutdown(const char *pcParameterBuffer) {
    kPrintf("System Shutdown Start...\n");
    
    kPrintf("Press Any Key To Reboot PC...");
    kGetCh();
    kReboot();
}
