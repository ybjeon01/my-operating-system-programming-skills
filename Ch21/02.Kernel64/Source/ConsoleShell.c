#include "Types.h"
#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "PIT.h"
#include "RTC.h"
#include "Task.h"
#include "List.h"
#include "Synchronization.h"


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
        "Create tasks, ex) createtask 1(type) 10(count)",
        kCreateTestTask
    },
    {
        "changepriority",
        "Change Task Priority, ex)changepriority 1(ID) 2(Priority)",
        kChangeTaskPriority
    },
    {
        "tasklist",
        "Show Task List",
        kShowTaskList
    },
    {
        "killtask",
        "End Task, ex)killtask 1(ID) or 0xFFFFFFFF(All Task)",
        kKillTask
    },
    {
        "cpuload",
        "Show Processor Load",
        kCPULoad
    },
    {
        "testmutex",
        "Test Mutex Function",
        kTestMutex
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
    {
        "listsq",
        "list scheduler queue ex) listsq ready(name) 1(priority)",
        kShowSchedulerList
    }
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
static void kExecuteCommand(const char *pcCommandBuffer) {
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
static void kInitializeParameter(PARAMETERLIST *pstList, const char *pcParameter) {
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
static void kHelp(const char *pcCommandBuffer) {
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
static void kCls(const char *pcParameterBuffer) {
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
static void kShowTotalRAMSize(const char *pcParameterBuffer) {
    kPrintf("Total RAM Size = %d MB\n", kGetTotalRAMSize());
}


// check if given parameter is hex or decimal
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   parameters: strings only with number
static void kStringToDecimalHexTest(const char *pcParameterBuffer) {
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
static void kShutdown(const char *pcParameterBuffer) {
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
static void kSetTimer(const char *pcParameterBuffer) {
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
static void kWaitUsingPIT(const char *pcParameterBuffer) {
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
static void kReadTimeStampCounter(const char *pcParameterBuffer) {
    QWORD qwTSC = kReadTSC();
    kPrintf("Time Stamp Counter = %q\n", qwTSC);
}


// measure processor maximum clock for 10 seconds
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   printMemoryMap does have any parameters
static void kMeasureProcessorSpeed(const char *pcParmeterBuffer) {
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
static void kShowDateAndTime(const char *pcParameterBuffer) {
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


/* task related shell commands */

static void kTestTask1(void) {
    BYTE bData;
    int iX;
    int iY;
    int iMargin;

    int i = 0;

    CHARACTER* pstScreen = (CHARACTER *) CONSOLE_VIDEOMEMORYADDRESS;
    TCB *pstRunningTask = kGetRunningTask();

    // margin from the screen border
    iMargin = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) % 10;

    iX = iMargin - 1;
    iY = iMargin - 1;

    // print 2000 characters until the end of the task
    for (int j = 0; j < 2000; j++) {
        switch (i) {
            // when iX, iY on the top line, move to right
            case 0:

                iX++;
                if (iX >= (CONSOLE_WIDTH - iMargin)) {
                    i = 1;
                }
                break;

            // when iX, iY on the right line, move to bottom
            case 1:
            
                iY++;
                if (iY >= (CONSOLE_HEIGHT - iMargin)) {
                    i = 2;
                }
                break;

            // when iX, iY on the bottom line, move to left
            case 2:

                iX--;
                if (iX < iMargin) {
                    i = 3;
                }
                break;

            // when iX, iY on the left line, move to top
            case 3:

                iY--;
                if (iY < iMargin) {
                    i = 0;
                }
                break;
        }

        pstScreen[iY * CONSOLE_WIDTH + iX].bCharacter = bData;
        pstScreen[iY * CONSOLE_WIDTH + iX].bAttribute = bData & 0x0F;
        bData++;
        kSchedule();
    }
    
    kExitTask();
}


// a function called by kCreateTestTask.
// it print a small pinwheel on the screen
// based on its task id 
static void kTestTask2(void) {
    int iOffset;

    CHARACTER* pstScreen = (CHARACTER *) CONSOLE_VIDEOMEMORYADDRESS;
    TCB *pstRunningTask = kGetRunningTask();
    char vcData[4] = {'-', '\\', '|', '/'};
    
    int i = 0;

    // pinwheel appears from the bottom. If pinwheels filled a line,
    // remaining pinwheels appears on a prev line
    iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (
        iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT)
    );

    while (TRUE) {
        pstScreen[iOffset].bCharacter = vcData[i % 4];
        pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
        i++;

        // kSchedule();
    }

}


// create a simple task and switch to the task
// params:
//   pcCommandBuffer: parameters passed to command by shell
//     type: type of task
//       1 (first type task) 
//       2 (second type task)
//     count: number of tasks to create
static void kCreateTestTask(const char *pcParameterBuffer) {
    PARAMETERLIST stList;
    char vcType[30];
    char vcCount[30];
    int i;

    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcType);
    kGetNextParameter(&stList, vcCount);

    switch(kAToI(vcType, 10)) {
        case 1:
            
            for (i = 0; i < kAToI(vcCount, 10); i++) {
                if (kCreateTask(TASK_FLAGS_LOW, (QWORD) kTestTask1) == NULL) {
                    break;
                }
            }
            kPrintf("Task1 %d Created\n", i);
            break;

        case 2:
        default:

            for (i = 0; i < kAToI(vcCount, 10); i++) {
                if (kCreateTask(TASK_FLAGS_LOW, (QWORD) kTestTask2) == NULL) {
                    break;
                }
            }
            kPrintf("Task2 %d Created\n", i);
            break;
    }
}


// change priority of a task
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   params:
//     taskID: id of a task to change priority
//     priority: priority that task will have
//       range: 0 ~ 4
static void kChangeTaskPriority(const char *pcParameterBuffer) {
    PARAMETERLIST stList;
    char vcID[30];
    char vcPriority[30];
    QWORD qwID;
    BYTE bPriority;

    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcID);
    kGetNextParameter(&stList, vcPriority);

    if (kMemCmp(vcID, "0x", 2) == 0) {
        qwID = kAToI(vcID+2, 16);
    }
    else {
        qwID = kAToI(vcID, 10);
    }

    bPriority = kAToI(vcPriority, 10);
    kPrintf("Change Task Priority ID [0x%q] Priority[%d] ", qwID, bPriority);

    if (kChangePriority(qwID, bPriority)) {
        kPrintf("Success\n");
    }
    else {
        kPrintf("Fail\n");
    }
}


// show information about currently created tasks
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kShowTaskList does have any parameters
static void kShowTaskList(const char *pcParameterBuffer) {
    TCB *pstTCB;
    int iCount = 0;

    kPrintf("=========== Task Total Count [%d] ===========\n", kGetTaskCount());
    for (iCount = 0; iCount < TASK_MAXCOUNT; iCount++) {
        pstTCB = kGetTCBInTCBPool(iCount);
        if ((pstTCB->stLink.qwID >> 32) != 0) {
            if ((iCount != 0) && ((iCount % 10) == 0)) {
                kPrintf("Press any key to continue... ('q' is exit) : \n");
                if (kGetCh() == 'q') {
                    break;
                }
            }

            kPrintf(
                "[%d] Task ID[0x%Q], Priority[%d] Flags[0x%Q]\n",
                1 + iCount,
                pstTCB->stLink.qwID,
                GETPRIORITY(pstTCB->qwFlags),
                pstTCB->qwFlags
            );
        }
    }
}


// kill a task by ID
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   params:
//     id: task id
//         if id is 0xFFFFFFFF, tasks whose id above 0x200000001 are killed
static void kKillTask(const char *pcParameterBuffer) {
    PARAMETERLIST stList;
    char vcID[30];
    QWORD qwID;
    TCB *pstTCB;
    int i;

    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcID);

    if (kMemCmp(vcID, "0x", 2) == 0) {
        qwID = kAToI(vcID + 2, 16);
    }
    else {
        qwID = kAToI(vcID, 10);
    }

    if (qwID != 0xFFFFFFFF) {
        kPrintf("Kill Task ID [0x%q] ", qwID);
        if (kEndTask(qwID)) {
            kPrintf("Success\n");
        }
        else {
            kPrintf("Fail\n");
        }
    }
    else {
        for (i = 2; i < TASK_MAXCOUNT; i++) {
            pstTCB = kGetTCBInTCBPool(i);
            qwID = pstTCB->stLink.qwID;
            
            if ((qwID >> 32) != 0) {
                kPrintf("Kill Task ID [0x%q] ", qwID);
                if (kEndTask(qwID)) {
                    kPrintf("Success\n");
                }
                else {
                    kPrintf("Fail\n");
                }
            }
        }
    }
}


// show processor usage in percentage
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kCPULoad does have any parameters
static void kCPULoad(const char *pcParameterBuffer) {
    kPrintf("Processor Load: %d%%\n", kGetProcessorLoad());
}


static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;

// add 1 to a global variable with mutex lock
// and print the number in the variable
static void kPrintNumberTask(void) {
    int i;
    int j;
    QWORD qwTickCount;

    // wait until console shell prints all messages
    qwTickCount = kGetTickCount();
    while ((kGetTickCount() - qwTickCount) < 50) {
        kSchedule();
    }

    for (i = 0; i < 5; i++) {
        // kLock(&gs_stMutex);
        kPrintf(
            "Task ID [0x%Q] Value [%d]\n",
            kGetRunningTask()->stLink.qwID,
            gs_qwAdder
        );

        gs_qwAdder += 1;
        // kUnlock(&gs_stMutex);

        // modern CPU is really fast so this function can be
        // done before scheduler interrupt occurs. In order to show
        // synchronization problem caused by multitasking scheduler
        // it is necessary to put a finite loop here
        for (j = 0; j < 30000; j++);
    }

    // wait for other task that does the same work to print all messages.
    // this is because kExitTask prints a message to console. I do not want
    // it to interrupt 
    qwTickCount = kGetTickCount();
    while ((kGetTickCount() - qwTickCount) < 1000) {
        kSchedule();
    }

    kExitTask();
}


// creates two tasks that adds 1 to the same variable. and check if
// mutex synchronization technique works
// params:
//   kTestMutex: parameters passed to command by shell
// info:
//   kTestMutex does have any parameters
static void kTestMutex(const char *pcCommandBuffer) {
    int i;
    gs_qwAdder = 1;

    kInitializeMutex(&gs_stMutex);

    for (i = 0; i < 3; i++) {
        kCreateTask(TASK_FLAGS_LOW, (QWORD) kPrintNumberTask);
    }
    kPrintf("Wait Until %d Task End...\n", i);
    kGetCh();
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
static void access(const char *pcParameterBuffer) {
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
static void printMemoryMap(const char *pcParameterBuffer) {
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
static void banner(const char *pcParameterBuffer) {
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


// show information about scheduler lists
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   params:
//     queueName: name of queue to print
//       values: [waiting, ready]
//     priority: priority of ready queue (valid only if queueName is ready)
//       range: [0, TASK_MAXREADYLISTCOUNT)
static void kShowSchedulerList(const char *pcParameterBuffer) {
    extern SCHEDULER gs_stScheduler;

    PARAMETERLIST stList;
    char vcQueueName[30];
    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcQueueName);
 
    LIST *pstQueue;
    if (kMemCmp("waiting", vcQueueName, 7) == 0) {
        pstQueue = &(gs_stScheduler.stWaitList);
    }
    else if (kMemCmp("ready", vcQueueName, 5) == 0) {
        char vcPriority[2];
        int iPriority;
        kGetNextParameter(&stList, vcPriority);
        iPriority = kAToI(vcPriority, 10);

        if (iPriority < 0 || TASK_MAXREADYLISTCOUNT <= iPriority) {
            kPrintf("wrong priority: %d\n", iPriority);
        }
        pstQueue = &(gs_stScheduler.vstReadyList[iPriority]);
    }
    else {
        kPrintf("wrong list name: %s\n", vcQueueName);
    }

    LISTLINK *stLink = kGetHeaderFromList(pstQueue);

    while (stLink) {
        kPrintf("stLink ID: [%q]\n", stLink->qwID);
        stLink = kGetNextFromList(stLink);
    }
}