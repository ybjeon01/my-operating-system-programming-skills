#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "Utility.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "Task.h"
#include "PIT.h"
#include "DynamicMemory.h"


void Main(void) {
    /* Initialize Console Shell */
    int iCursorX, iCursorY;
    kInitializeConsole(0, 10);


    /* IA-32e mode success messages */

    kPrintf("Switch To IA-32e Mode Success~!!\n");
    kPrintf("IA-32e C Language Kernel Start............. [Pass]\n");
    
    /* Console success message */

    kPrintf("Initialize Console..........................[Pass]\n");


    /* 
     * load GDT again and load IDT for handling interrupts 
     * This GDT is at 1MB and contains segment descriptors and TSS descriptor
     * TSS descriptor is for offering stack to interrupt handler
     */ 

    kGetCursor(&iCursorX, &iCursorY);
    kPrintf("GDT Switch For IA-32e Mode..................[    ]");
    kInitializeGDTTableAndTSS();
    kLoadGDTR(GDTR_STARTADDRESS);
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");

    kPrintf("TSS Segment Load............................[    ]");
    kLoadTR(GDT_TSSSEGMENT);
    kSetCursor(45, iCursorY++);
    kPrintf("PASS\n");

    kPrintf("IDT Initialization..........................[    ]");
    kInitializeIDTTables();
    kLoadIDTR(IDTR_STARTADDRESS);
    kSetCursor(45, iCursorY++);
    kPrintf("PASS\n");


    /* check total ram size of the computer */
    
    kPrintf("Total RAM Size Check........................[    ]");
    kCheckTotalRAMSize();
    kSetCursor(45, iCursorY++);
    kPrintf("Pass], Size = %d MB\n", kGetTotalRAMSize());


    /* Initialize TCB pool and task scheduler */

    kPrintf("TCB Pool And Scheduler Initialize...........[Pass]\n");
    iCursorY++;
    kInitializeScheduler();


    /* Initialize Dynamic Memory Manager */

    kPrintf("Dynamic Memory initialize...................[Pass]\n");
    iCursorY++;
    kInitializeDynamicMemory();

    /* Initialize Programmable Interrupt Timer */

    kInitializePIT(MSTOCOUNT(1), 1);


    /* Activate Keyboard and initialize keyboard buffer */

    kPrintf("Keyboard Activate And Queue Initialize......[    ]");

    if (kInitializeKeyboard()) {
        kSetCursor(45, iCursorY++);
        kPrintf("Pass\n");
        // set Num Lock, Caps Lock, Scroll Lock off
        kChangeKeyboardLED(FALSE, FALSE, FALSE);
    }
    else {
        kSetCursor(45, iCursorY++);
        kPrintf("Fail\n");
        while (1);
    }


    /* Initialize PIC controller */

    kPrintf("PIC Controller And Interrupt Initialize.....[    ]");
    kInitializePIC();
    // unmask all interrupts
    kMaskPICInterrupt(0);
    // interrupt was deactivated in 01.Kernel32/EntryPoint.s 
    kEnableInterrupt();
    kSetCursor(45, iCursorY++);
    kPrintf("Pass\n");    


    /* create IDLE task */

    // flag: lowest priority and idle task
    kCreateTask(
        (
            TASK_FLAGS_LOWEST |
            TASK_FLAGS_IDLE |
            TASK_FLAGS_SYSTEM |
            TASK_FLAGS_THREAD
        ),
        0,
        0,
        (QWORD) kIdleTask
    );

    /* simple shell */
    
    kStartConsoleShell();
}
