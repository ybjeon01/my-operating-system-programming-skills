#include "Types.h"
#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "PIT.h"
#include "RTC.h"
#include "Task.h"


SHELLCOMMANDENTRY gs_vstCommandTable[] = {
    {
        "help",
        "Show Help",
        kHelp
    },
    {
        "cls",
        "Clear Screen",
        kCls
    },
    {
        "totalram",
        "Show Total RAM Size",
        kShowTotalRAMSize
    },
    {
        "strtod",
        "String to Decimal/Hex Convert",
        kStringToDecimalHexTest
    },
    {
        "shutdown",
        "Shutdown And Reboot OS",
        kShutdown
    },
    {
        "settimer",
        "Set PIT Controller Counter0 ex)settimer 10(ms) 1(periodic)",
        kSetTimer
    },
    {
        "wait",
        "Wait ms Using PIT, ex)wait 100(ms)",
        kWaitUsingPIT
    },
    {
        "rdtsc",
        "Read Time Stamp Counter",
        kReadTimeStampCounter
    },
    {
        "cpuspeed",
        "Measure Processor Speed",
        kMeasureProcessorSpeed
    },
    {
        "date",
        "Show Date And Time",
        kShowDateAndTime
    },
    {
        "createtask",
        "Create a test task",
        kCreateTestTask
    },


    /* custom shell commands */
    
    {
        "access",
        "write and access address",
        access
    },
    {
        "memorymap",
        "get current memory map",
        printMemoryMap
    },
    {
        "banner",
        "print MINT64OS banner",
        banner
    },
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


// set timer using PIT counter0 register
// params:
//   pcCommandBuffer: parameters passed to command by shell
//     time: counter reset value in ms unit
//     periodic: boolean value to decide repeat
//       1: true
//       0: false
void kSetTimer(const char *pcParameterBuffer) {
    char vcParameter[100];
    PARAMETERLIST stList;
    long lValue;
    BOOL bPeriodic;

    kInitializeParameter(&stList, pcParameterBuffer);
    
    if (kGetNextParameter(&stList, vcParameter) == 0) {
        kPrintf("ex)settimer 10(ms) 1(periodic)\n");
        return;
    }
    lValue = kAToI(vcParameter, 10);

    if (kGetNextParameter(&stList, vcParameter) == 0) {
        kPrintf("ex)settimer 10(ms) 1(periodic)\n");
        return;
    }
    bPeriodic = (BOOL) kAToI(vcParameter, 10);

    kInitializePIT(MSTOCOUNT(lValue), bPeriodic);
    kPrintf("Time = %d ms, Periodic = %d Change Complete\n", lValue, bPeriodic);
}


// busy-wait for a period by using PIT controller
// params:
//   pcCommandBuffer: parameters passed to command by shell
//     time: counter reset value in ms unit
//     periodic: boolean value to decide repeat
//       1: true
//       0: false
void kWaitUsingPIT(const char *pcParameterBuffer) {
    char vcParameter[100];
    int iLength;
    PARAMETERLIST stList;
    long lMillisecond;
    int i;

    kInitializeParameter(&stList, pcParameterBuffer);
    
    if (kGetNextParameter(&stList, vcParameter) == 0) {
        kPrintf("ex)wait 100(ms)\n");
        return;
    }
    lMillisecond = kAToI(vcParameter, 10);
    kPrintf("%d ms Sleep Start...\n", lMillisecond);

    // disable interrupt, so other tasks does not use PIT
    kDisableInterrupt();

    // iteration is because PIT does not support big number for timer
    for (i = 0; i < lMillisecond / 30; i++) {
        kWaitUsingDirectPIT(MSTOCOUNT(30));
    }
    kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 30));

    kEnableInterrupt();
    kPrintf("%d Sleep Complete\n", lMillisecond);
    
    // restore prev PIT
    kInitializePIT(MSTOCOUNT(1), TRUE);
}


// read time stamp counter
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   printMemoryMap does have any parameters
void kReadTimeStampCounter(const char *pcParameterBuffer) {
    QWORD qwTSC = kReadTSC();
    kPrintf("Time Stamp Counter = %q\n", qwTSC);
}


// measure processor maximum clock for 10 seconds
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   printMemoryMap does have any parameters
void kMeasureProcessorSpeed(const char *pcParmeterBuffer) {
    int i;
    QWORD qwLastTSC, qwTotalTSC = 0;

    kPrintf("Now Measuring.");

    // disable interrupt, so other tasks does not use PIT
    kDisableInterrupt();

    // measure processor speed for 10 seconds
    for (i = 0; i < 200; i++) {
        qwLastTSC = kReadTSC();
        kWaitUsingDirectPIT(MSTOCOUNT(50));
        qwTotalTSC += kReadTSC() - qwLastTSC;

        kPrintf(".");
    }

    // restore PIT
    kInitializePIT(MSTOCOUNT(1), TRUE);
    kEnableInterrupt();

    kPrintf("\nCPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000 / 1000);
}


// print date and time stored in RTC controller
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   printMemoryMap does have any parameters
void kShowDateAndTime(const char *pcParameterBuffer) {
    BYTE bSecond, bMinute, bHour;
    BYTE bDayOfWeek, bDayOfMonth, bMonth;
    WORD wYear;

    kReadRTCTime(&bHour, &bMinute, &bSecond);
    kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

    kPrintf(
        "Date: %d/%d/%d %s, ",
        wYear,
        bMonth,
        bDayOfMonth,
        kConvertDayOfWeekToString(bDayOfWeek)
    );
    kPrintf(
        "Time: %d:%d:%d\n",
        bHour,
        bMinute,
        bSecond
    );
}


/* task related arrays and shell commands */

// first task is this console shell
// second task is a simple task that just prints a message
static TCB gs_vstTask[2] = {0, };

// a stack for the simple task
static QWORD gs_vstStack[1024] = {0, };


// a simple test task that switches between test tasks
// this is just a function called by another console shell command
void kTestTask(void) {
    int i = 0;
    while (TRUE) {
        kPrintf(
            "[%d] This message is from kTestTask. Press any key to switch to"
            " kConsoleShell~!!\n", 
        i++
        );

        kGetCh();
        kSwitchContext(&(gs_vstTask[1].stContext), &(gs_vstTask[0].stContext));
    }
}


// create a simple task and switch to the task until user presses a key, 'q'
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   printMemoryMap does have any parameters
void kCreateTestTask(const char *pcParameterBuffer) {
    KEYDATA stData;
    int i = 0;

    // create a TCB for a test task
    kSetUpTask(
        &(gs_vstTask[1]),
        1,
        0,
        (QWORD) kTestTask,
        (void *) gs_vstStack,
        sizeof(gs_vstStack)
    );

    // if user hits 'q' letter, quit the shell
    while (TRUE) {
        kPrintf(
            "[%d] This message from kConsoleShell. Press q to finish the"
            " command or other keys to switch to test task\n",
            i++
        );
        if (kGetCh() == 'q') {
            break;
        }

        // it does not need to create TCB for console shell
        // because first call of kSwitchContext saves all the context
        // to TCB for console shell
        kSwitchContext(&(gs_vstTask[0].stContext), &(gs_vstTask[1].stContext));
    }
}


/* custom shell commands */

// check if memory at specific address is readable and writable
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   parmater: address in hex (unit: byte) or address in decimal (unit: MB)
//   this function actually tries to write data at address given by paramter
//   If the area is hardware reserved, reboot might happen
// example:
//   access 0x12345
//   access 1024
void access(const char *pcParameterBuffer) {
    char vcParameter[100];
    PARAMETERLIST stList;
    int iLength;

    BYTE *bAddr;
    BYTE bPrevValue;

    // initialize paramter list
    kInitializeParameter(&stList, pcParameterBuffer);
    iLength = kGetNextParameter(&stList, vcParameter);

    if (iLength == 0) {
        kPrintf("Usage: access memory_address\n");
        return;
    }

    if (kMemCmp(vcParameter, "0x", 2) == 0) {
        bAddr = (BYTE *) (kAToI(&(vcParameter[2]), 16));
    }
    else {
        bAddr = (BYTE *) (kAToI(vcParameter, 10) << 20);
    }
    
    *bAddr = (BYTE) 0x1234;
    if (*bAddr == (BYTE) 0x1234) {
        kPrintf("exist\n");
        *bAddr = bPrevValue;
    }
    else {
        kPrintf("does not exist\n");
    }
}


// print computer memory map that shows hardware reserved area and
// free area
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   printMemoryMap does have any parameters
void printMemoryMap(const char *pcParameterBuffer) {
    SMAP_entry_t *smap = (SMAP_entry_t *) SMAP_START_ADDRESS;
    DWORD count = *(DWORD *) SMAP_COUNT_ADDRESS;

    char buffer[20];
    int iMaxBaseLength = 0;
    int iMaxRangeLength = 0;
    int iLength = 0;

    int iCursorX, iCursorY;

    for (int i = 0; i < count; i++) {
        iLength = kIToA(smap->Base, buffer, 16);
        if (iLength > iMaxBaseLength) {
            iMaxBaseLength = iLength;
        }

        iLength = kIToA(smap->Length, buffer, 16);
        if (iLength > iMaxRangeLength) {
            iMaxRangeLength = iLength;
        }
        smap += 1;
    }

    smap = (SMAP_entry_t *) SMAP_START_ADDRESS;
    for (int i = 0; i < count; i++) {
        kGetCursor(&iCursorX, &iCursorY);
        iLength = kIToA(smap->Base, buffer, 16);
        kSetCursor(iMaxBaseLength - iLength, iCursorY);
        kPrintf("%p", smap->Base);

        kGetCursor(&iCursorX, &iCursorY);
        iLength = kIToA(smap->Length, buffer, 16);
        kSetCursor(iCursorX + 3 + iMaxRangeLength - iLength, iCursorY);
        kPrintf("%p", smap->Length);

        kPrintf("   %d\n", smap->Type);
        smap += 1;

        if (i % 24 == 0 && i != 0) {
            kGetCursor(&iCursorX, &iCursorY);
            kPrintf("press any key to continue...");
            kGetCh();
            kSetCursor(iCursorX, iCursorY);
            kPrintf("                            ");
            kSetCursor(iCursorX, iCursorY);
        }
    }
}


// print MINT64OS banner
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   banner does have any parameters
void banner(const char *pcParameterBuffer) {
    const char *banner = \
        "888b     d888 8888888 888b    888 88888888888 .d8888b.      d8888\n" \
        "8888b   d8888   888   8888b   888     888    d88P  Y88b    d8P888\n" \
        "88888b.d88888   888   88888b  888     888    888          d8P 888\n" \
        "888Y88888P888   888   888Y88b 888     888    888d888b.   d8P  888\n" \
        "888 Y888P 888   888   888 Y88b888     888    888P  Y88b d88   888\n" \
        "888  Y8P  888   888   888  Y88888     888    888    888 8888888888\n"\
        "888       888   888   888   Y8888     888    Y88b  d88P       888\n" \
        "888       888 8888888 888    Y888     888     Y8888PP         888\n"; 

    kPrintf(banner);
}