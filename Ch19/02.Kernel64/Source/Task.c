#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"
#include "AssemblyUtility.h"



/* singleton data structure of Scheduler and Task pool manager */

static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;


/* Task pool related functions */


// initialize task pool
void kInitializeTCBPool(void) {
    kMemSet(&(gs_stTCBPoolManager), 0, sizeof(gs_stTCBPoolManager));

    gs_stTCBPoolManager.pstStartAddress = (TCB *) TASK_TCBPOOLADDRESS;

    // low 32 bits are index of array high 32 bits are iAllocatedCount
    // and it is used to calculate task stack address
    for (int i = 0; i < TASK_MAXCOUNT; i++) {
        gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
    }

    gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
    
    // let the allocatedCount starts from 1
    // This is because if high 32 bits are zero,
    // TCBManager considers the task as empty task
    gs_stTCBPoolManager.iAllocatedCount = 1;
}

// allocate TCB
// return:
//   pointer to a empty TCB
//   if TCB pool is full, null is returned
TCB *kAllocateTCB(void) {
    TCB *pstEmptyTCB;

    if (gs_stTCBPoolManager.iMaxCount == gs_stTCBPoolManager.iUseCount) {
        return NULL;
    }

    for (int i = 0; i < gs_stTCBPoolManager.iMaxCount; i++) {
        // high 32 bits of TCB not in use is zero
        pstEmptyTCB = &(gs_stTCBPoolManager.pstStartAddress[i]);
        if ((pstEmptyTCB->stLink.qwID >> 32) == 0) {
            break;
        }
    }

    // set high 32 bits with iAllocatedCount
    pstEmptyTCB->stLink.qwID |= (
        ((QWORD) gs_stTCBPoolManager.iAllocatedCount) << 32
    );
    
    /* change the state of TCBPoolManager */

    gs_stTCBPoolManager.iUseCount++;
    gs_stTCBPoolManager.iAllocatedCount++;
    
    return pstEmptyTCB;
}


// free allocated TCB
// params:
//   qwID: ID of TCB
// info:
//   the qwID must be valid task id in use. Otherwise the result
//   is unpredictable
void kFreeTCB(QWORD qwID) {
    // get index of task pool
    int i = qwID & 0xFFFFFFFF;

    kMemSet(
        &(gs_stTCBPoolManager.pstStartAddress[i].stContext),
        0,
        sizeof(CONTEXT)
    );
    gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
    
    gs_stTCBPoolManager.iUseCount--;
}


// a handy function that creates a task
// params:
//   qwFlags: a flag that determines if other task can interrupt this task
//   qwEntryPointAddress: start address of a code
// return:
//   a ready to use task which is put into task queue
//   if task pool is full, null is returned
TCB *kCreateTask(QWORD qwFlags, QWORD qwEntryPointAddress) {
    TCB *pstTask;
    void *pvStackAddress;

    pstTask = kAllocateTCB();
    if (pstTask == NULL) {
        return NULL;
    }

    // every task has its own stack
    pvStackAddress = (void *) (
        TASK_STACKPOOLADDRESS + 
        (TASK_STACKSIZE * pstTask->stLink.qwID & 0xFFFFFFFF)    
    );

    kSetUpTask(
        pstTask,
        qwFlags,
        qwEntryPointAddress,
        pvStackAddress,
        TASK_STACKSIZE
    );

    kAddTaskToReadyList(pstTask);
    return pstTask;
}


// create a initial task structure based on the parameters
// params:
//   pstTCB: an unused TCB struct
//   qwFlags: a flag that determines if other task can interrupt this task
//   qwEntryPointAddress: start address of a code
//   pvStackAddress: start address of a stack used for the task
//   qwStackSize:  size of the stack
void kSetUpTask(
    TCB *pstTCB,
    QWORD qwFlags,
    QWORD qwEntryPointAddress,
    void *pvStackAddress,
    QWORD qwStackSize
) {
    // initialize TCB struct
    kMemSet(
        pstTCB->stContext.vqRegister,
        0,
        sizeof(pstTCB->stContext.vqRegister)
    );


    /* set rsp and rbp */
    
    pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = 
        (QWORD) pvStackAddress + qwStackSize;
    
    pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = 
        (QWORD) pvStackAddress + qwStackSize;


    /* set segment selectors */

    pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT;

    pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT;


    /* set IP */
    
    pstTCB->stContext.vqRegister[TASK_RIPOFFSET] = qwEntryPointAddress;


    /* set interrupt flag in RFLAGS by default */

    pstTCB->stContext.vqRegister[TASK_RFLAGSOFFSET] |= 0x200;

    /*  stack, and flag */

    pstTCB->pvStackAddress = pvStackAddress;
    pstTCB->qwStackSize = qwStackSize;
    pstTCB->qwFlags = qwFlags;
}


/* Task Scheduler related functions */

// initialize scheduler
// info:
//   this function also initializes TCB pool together
//   Therefore, no need to initialize TCB pool
void kInitializeScheduler(void) {
    
    // initialize Task Pool
    kInitializeTCBPool();

    // initialize scheduler task queue
    kInitializeList(&(gs_stScheduler.stReadyList));
 
    // the empty TCB is for code that executed booting
    // when context switch happens, the empty TCB will
    // have the code context
    // stack area of the code is from 6 MB to 7 MB
    gs_stScheduler.pstRunningTask = kAllocateTCB();
}


// return the currently running task
// return:
//   pointer to currently running task's TCB
TCB *kGetRunningTask(void) {
    return gs_stScheduler.pstRunningTask;
}


// remove the first task in the ready-to-run task queue
// return:
//   the first task's TCB in the ready-to-run task queue
//   if there is no tasks in the queue, null is returned
TCB *kGetNextTaskToRun(void) {
    if (kGetListCount(&(gs_stScheduler.stReadyList)) == 0) {
        return NULL;
    }
    return (TCB *) kRemoveListFromHeader(&(gs_stScheduler.stReadyList));
}


// add a non-empty task to ready-to-run-queue
// params:
//   pstTask: a non-empty task to put into queue
void kAddTaskToReadyList(TCB *pstTask) {
    kAddListToTail(&(gs_stScheduler.stReadyList), pstTask);
}


// schedule a task to run now
// info:
//   If there is no next task to run, current task is keeping running
//
//   This function can be explicitely called by current task if it wants to
//   yield CPU
//
//   this function disables interrupt. PIT interrupt while executing this code,
//   causes scheduler not to work properly. In other words, interrupt handler
//   and this function shares the same resource named scheduler
void kSchedule(void) {
    TCB *pstRunningTask, *pstNextTask;
    BOOL bPreviousFlag;

    bPreviousFlag = kSetInterruptFlag(FALSE);
    pstNextTask = kGetNextTaskToRun();

    if (!pstNextTask) {
        kSetInterruptFlag(bPreviousFlag);
        return;
    }

    // put current task to task queue
    pstRunningTask = gs_stScheduler.pstRunningTask;
    kAddTaskToReadyList(pstRunningTask);

    // switch from current task to next task
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
    gs_stScheduler.pstRunningTask = pstNextTask;

    kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));

    // If the current task is scheduled to run again by kernel, code is executed
    // from here
    kSetInterruptFlag(bPreviousFlag);
}


// schedule a task to run now
// return:
//   True if another task will be running. Otherwise False 
// info:
//    this function must be only called by interrupt or exception handler
//    because it contains code that handles IST stack. If this is called
//    outside interrupt handler, current task will not work properly when
//    it is scheduled again later
//
//    This function returns because it expects interrupt handler to restore
//    context of the next task
BOOL kScheduleInInterrupt(void) {
    TCB *pstRunningTask = gs_stScheduler.pstRunningTask;
    TCB *pstNextTask = kGetNextTaskToRun();

    if (!pstNextTask) {
        return FALSE;
    }

    // IST start address of context
    // When interrupt happens, ISR kSAVECONTEXT macro stores context of current
    // task to IST stack
    char *pcContextAddress = (
        (char *) IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT)
    );

    /* store running task */
    kMemCpy(
        &(pstRunningTask->stContext.vqRegister),
        pcContextAddress,
        sizeof(CONTEXT)
    );
    kAddTaskToReadyList(pstRunningTask);


    /* restore the next task */
    gs_stScheduler.pstRunningTask = pstNextTask;
    kMemCpy(
        pcContextAddress,
        &(pstNextTask->stContext.vqRegister),
        sizeof(CONTEXT)
    );

    // update scheduler manager
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
    return TRUE;
}


// decrease processor time of current task
void kDecreaseProcessorTime(void) {
    if (gs_stScheduler.iProcessorTime > 0) {
        gs_stScheduler.iProcessorTime--;
    }
}


// check if processor time of current task is expired
// return:
//   True if expired. Otherwise, False
BOOL kIsProcessorTimeExpired(void) {
    if (gs_stScheduler.iProcessorTime == 0) {
        return TRUE;
    }
    return FALSE;
}