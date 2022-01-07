#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Console.h"
#include "Synchronization.h"


/* singleton data structure of Scheduler and Task pool manager */

SCHEDULER gs_stScheduler;
TCBPOOLMANAGER gs_stTCBPoolManager;


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
    int i = GETTCBOFFSET(qwID);

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
//   pvMemoryAddress: process memory address which are available to threads
//   qwMemorySize: size of the process memory
//   qwEntryPointAddress: start address of a code
// return:
//   a ready to use task which is put into task queue
//   if task pool is full, null is returned
TCB *kCreateTask(
    QWORD qwFlags,
    void *pvMemoryAddress,
    QWORD qwMemorySize,
    QWORD qwEntryPointAddress
) {
    TCB *pstTask;
    TCB *pstProcess;
    void *pvStackAddress;
    BOOL bPreviousFlag;

    bPreviousFlag = kLockForSystemData();
    pstTask = kAllocateTCB();
    kUnlockForSystemData(bPreviousFlag);

    if (pstTask == NULL) {
        return NULL;
    }

    pstProcess = kGetProcessByThread(kGetRunningTask());
    // for debugging purpose
    // if I did not make mistake, this condition statement should not be
    // executed 
    if (pstProcess == NULL) {
        kFreeTCB(pstTask->stLink.qwID);
        return NULL;
    }

    // if thread makes another thread
    if (qwFlags & TASK_FLAGS_THREAD) {
        pstTask->qwParentProcessID = pstProcess->stLink.qwID;
        pstTask->pvMemoryAddress = pstProcess->pvMemoryAddress;
        pstTask->qwMemorySize = pstProcess->qwMemorySize;
        bPreviousFlag = kLockForSystemData();
        kAddListToTail(
            &(pstProcess->stChildThreadList),
            &(pstTask->stThreadLink)
        );
        kUnlockForSystemData(bPreviousFlag);
    }
    // if process is created
    else {
        pstTask->qwParentProcessID = pstProcess->stLink.qwID;
        pstTask->pvMemoryAddress = pvMemoryAddress;
        pstTask->qwMemorySize = qwMemorySize;
    }

    pstTask->stThreadLink.qwID = pstTask->stLink.qwID;

    // every task has its own stack
    pvStackAddress = (void *) (
        TASK_STACKPOOLADDRESS + 
        (TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID))
    );

    kSetUpTask(
        pstTask,
        qwFlags,
        qwEntryPointAddress,
        pvStackAddress,
        TASK_STACKSIZE
    );

    kInitializeList(&(pstTask->stChildThreadList));

    // initialize FPU related members
    pstTask->bFPUUsed = FALSE;

    bPreviousFlag = kLockForSystemData();
    kAddTaskToReadyList(pstTask);
    kUnlockForSystemData(bPreviousFlag);

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
    
    // set kExitTask function at return address region
    // so kExitTask is run after command is run
    *(QWORD *)(pvStackAddress + qwStackSize - 8) = (QWORD) kExitTask;

    pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = 
        (QWORD) pvStackAddress + qwStackSize - 8;
    
    pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = 
        (QWORD) pvStackAddress + qwStackSize - 8;


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
    TCB *pstTask;


    // initialize Task Pool
    kInitializeTCBPool();

    // initialize scheduler task queues
    for (int i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
        kInitializeList(&(gs_stScheduler.vstReadyList[i]));
        gs_stScheduler.viExecuteCount[i] = 0;
    }
    kInitializeList(&(gs_stScheduler.stWaitList));
 
    // the empty TCB is for code that executed booting
    // when context switch happens, the empty TCB will
    // have the code context
    // stack area of the code is from 6 MB to 7 MB
    pstTask = kAllocateTCB();
    gs_stScheduler.pstRunningTask = pstTask;
    gs_stScheduler.pstRunningTask->qwFlags = (
        TASK_FLAGS_HIGHEST |
        TASK_FLAGS_PROCESS |
        TASK_FLAGS_SYSTEM
    );
    pstTask->qwParentProcessID = pstTask->stLink.qwID;
    pstTask->pvMemoryAddress = (void *) 0x100000;
    pstTask->qwMemorySize = 0x500000;
    
    pstTask->pvStackAddress = (void *) 0x600000;
    pstTask->qwStackSize = 0x100000;
    kInitializeList(&(pstTask->stChildThreadList));

    // initialize FPU parts

    pstTask->bFPUUsed = FALSE;
    

    // initialize variables that help to calculate processor load
    gs_stScheduler.qwSpendProcessorTimeinIdleTask = 0;
    gs_stScheduler.qwProcessorLoad = 0;

    // initialize FPU related members
    gs_stScheduler.qwLastFPUUsedTaskID = TASK_INVALIDID;
}


// return the currently running task
// return:
//   pointer to currently running task's TCB
TCB *kGetRunningTask(void) {
    return gs_stScheduler.pstRunningTask;
}


// remove the next task in the multi-level priority queues
// return:
//   The higest task's TCB in the multi-level priority queues
//   if there are no tasks in the queues, null is returned
TCB *kGetNextTaskToRun(void) {
    TCB *pstTarget = NULL;

    int iTaskCount;
    LIST *pstReadyList;

    // if tasks in every queue are executed once, it is possible that
    // no task is selected because every task yields CPU. To make sure
    // to get next task, looping twice is necessary  
    for (int j = 0; j < 2; j++) {

        // check from the highest queue to the lowest queue to select a task
        for (int i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
            pstReadyList = &(gs_stScheduler.vstReadyList[i]);
            iTaskCount = kGetListCount(pstReadyList);

            if (gs_stScheduler.viExecuteCount[i] < iTaskCount) {
                pstTarget = (TCB *) kRemoveListFromHeader(pstReadyList);
                gs_stScheduler.viExecuteCount[i]++;
                break;
            }
            else {
                gs_stScheduler.viExecuteCount[i] = 0;
            }
        }

        if (pstTarget) {
            break;
        }

    }
    return pstTarget;
}


// add a non-empty task to multi-level priority queues
// params:
//   pstTask: a non-empty task to put into queues
// return:
//   True if the function succeed. Otherwise, False
BOOL kAddTaskToReadyList(TCB *pstTask) {
    BYTE bPriority = GETPRIORITY(pstTask->qwFlags);

    if (bPriority >= TASK_MAXREADYLISTCOUNT) {
        return FALSE;
    }
    kAddListToTail(&(gs_stScheduler.vstReadyList[bPriority]), pstTask);
    return TRUE;
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
    TCB *pstRunningTask;
    TCB *pstNextTask;
    BOOL bPreviousFlag;

    if (kGetReadyTaskCount() < 1) {
        return;
    }

    bPreviousFlag = kLockForSystemData();
    pstNextTask = kGetNextTaskToRun();

    if (!pstNextTask) {
        kUnlockForSystemData(bPreviousFlag);
        return;
    }

    pstRunningTask = gs_stScheduler.pstRunningTask;


    /* calculate Idle task related statistics */

    if (pstRunningTask->qwFlags & TASK_FLAGS_IDLE) {
        gs_stScheduler.qwSpendProcessorTimeinIdleTask +=
            TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;
    }


    /* FPU related code */

    // if the next task to execute equals to the current task
    // do not change TS bit in CR0, so exception will not be raised
    if (gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID) {
        kSetTS();
    }
    else {
        kClearTS();
    }

    /* Switch from current task to next task */ 

    // update scheduler
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
    gs_stScheduler.pstRunningTask = pstNextTask;

    // when current task is finished
    if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
        kAddListToTail(&gs_stScheduler.stWaitList, pstRunningTask);
        kSwitchContext(NULL, &(pstNextTask->stContext));
    }
    // when current task is just yielding CPU
    else {
        kAddTaskToReadyList(pstRunningTask);
        kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
    }

    // If the current task is scheduled to run again by kernel, code is executed
    // from here
    kUnlockForSystemData(bPreviousFlag);
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


    /* calculate Idle task related statistics */

    if (pstRunningTask->qwFlags & TASK_FLAGS_IDLE) {
        gs_stScheduler.qwSpendProcessorTimeinIdleTask += TASK_PROCESSORTIME;
    }


    /* FPU related code */

    // if the next task to execute equals to the current task
    // do not change TS bit in CR0, so exception will not be raised
    if (gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID) {
        kSetTS();
    }
    else {
        kClearTS();
    }


    /* Switch from current task to next task */ 

    // update scheduler
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
    gs_stScheduler.pstRunningTask = pstNextTask;
    

    // when current task is finished
    if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
        kAddListToTail(&gs_stScheduler.stWaitList, pstRunningTask);
    }
    // when current task is just yielding CPU
    else {
        kMemCpy(
            &(pstRunningTask->stContext.vqRegister),
            pcContextAddress,
            sizeof(CONTEXT)
        );
        kAddTaskToReadyList(pstRunningTask);
    }

    // switch to next task
    kMemCpy(
        pcContextAddress,
        &(pstNextTask->stContext.vqRegister),
        sizeof(CONTEXT)
    );

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


// remove task from multi level priority queues
// params:
//   qwTaskID: task id to remove
// return:
//  task that has qwTaskID
//  if qwTaskID is invalid, null is returned
TCB *kRemoveTaskFromReadyList(QWORD qwTaskID) {
    TCB *pstTarget;
    BYTE bPriority;
    
    // if task id is invalid
    if (GETTCBOFFSET(qwTaskID) >= TASK_MAXCOUNT) {
        return NULL;
    }

    // get TCB from task pool by using index in the task id
    pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
    if (pstTarget->stLink.qwID != qwTaskID) {
        return NULL;
    }

    // get priority so the function knows which queue has the task
    bPriority = GETPRIORITY(pstTarget->qwFlags);

    pstTarget = kRemoveList(&(gs_stScheduler.vstReadyList[bPriority]), qwTaskID);
    return pstTarget;
}


// change priority of a task
// params:
//   qwTaskID: task id to change priority
//   bPriority: priority that you want
// return:
//   True if it succeeds. Otherwise False
//   if qwTaskId or bPriority is invalid, False is returned
BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority) {
    TCB *pstTarget;
    BOOL bPreviousFlag;

    if (bPriority > TASK_MAXREADYLISTCOUNT) {
        return FALSE;
    }

    bPreviousFlag = kLockForSystemData();

    /* when qwTaskID is currently running task */

    // when interrupt happens, scheduler will put the task into
    // the priority queue
    pstTarget = gs_stScheduler.pstRunningTask;
    if (qwTaskID == pstTarget->stLink.qwID) {
        SETPRIORITY(pstTarget->qwFlags, bPriority);
        return TRUE;
    }


    /* when qwTaskID is inside ready-to-run queues */
    pstTarget = kRemoveTaskFromReadyList(qwTaskID);
    if (pstTarget) {
        SETPRIORITY(pstTarget->qwFlags, bPriority);
        return TRUE;
    }


    /* when qwTaskID is inside task pool
     * 
     * if task is not in the ready queues, it means that
     * the task is allocated, but not in the queues yet
     */
    // get the task by using index
    pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
    if (pstTarget) {
        SETPRIORITY(pstTarget->qwFlags, bPriority);
        return TRUE;
    }

    kUnlockForSystemData(bPreviousFlag);
    return FALSE;
}


// find a task and finish the task
// params:
//   qwTaskID: task id to end
// return:
//   True if the task is finished.
//   False if the task does not exist
BOOL kEndTask(QWORD qwTaskID) {
    TCB *pstTarget;
    BYTE bPriority;
    BOOL bPreviousFlag;

    /* when qwTaskID is currently running task */

    pstTarget = gs_stScheduler.pstRunningTask;
    if (pstTarget->stLink.qwID == qwTaskID) {
        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

        // context switch happens and this ended task is never executed again
        kSchedule();
    }

    bPreviousFlag = kLockForSystemData();


    /* when qwTaskID is inside ready-to-run queues */

    pstTarget = kRemoveTaskFromReadyList(qwTaskID);
    if (pstTarget) {
        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
        kAddListToTail(&(gs_stScheduler.stWaitList), pstTarget);
        kUnlockForSystemData(bPreviousFlag);
        return TRUE;
    }

    /* when qwTaskID is inside task pool
     * 
     * if task is not in the ready queues, it means that
     * the task is allocated, but not in the queues yet
     */
    // get the task by using index
    pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
    if (pstTarget) {
        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
        kAddListToTail(&(gs_stScheduler.stWaitList), pstTarget);
        kUnlockForSystemData(bPreviousFlag);
        return TRUE;
    }

    kUnlockForSystemData(bPreviousFlag);
    return FALSE;
}


// let current task end itself
void kExitTask(void) {
    kEndTask(gs_stScheduler.pstRunningTask->stLink.qwID);
}


// get number of tasks in the multi level priority queues
// return:
//   total number of tasks in the queues
int kGetReadyTaskCount(void) {
    int iTotalCount = 0;
    BOOL bPreviousFlag;

    bPreviousFlag = kLockForSystemData();

    for (int i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
        iTotalCount += kGetListCount(&(gs_stScheduler.vstReadyList[i]));
    }

    kUnlockForSystemData(bPreviousFlag);
    return iTotalCount;
}


// return overall number of tasks
// return:
//   total number of valid tasks
// note:
//   TCBPool manager has iUseCount variable. I need to ask author why
//   he did not use iCount
int kGetTaskCount(void) {
    int iTotalCount;
    BOOL bPreviousFlag;

    // tasks in ready queues
    iTotalCount = kGetReadyTaskCount();

    bPreviousFlag = kLockForSystemData();

    // wait list + current task
    iTotalCount += kGetListCount(&(gs_stScheduler.stWaitList)) + 1;

    kUnlockForSystemData(bPreviousFlag);
    return iTotalCount;
}


// get a reference to TCB object by TCB index
// params:
//   iOffset: index of TCB to get
// return:
//   pointer to a TCB 
TCB *kGetTCBInTCBPool(int iOffset) {
    if (0 <= iOffset && iOffset < TASK_MAXCOUNT) {
        return &(gs_stTCBPoolManager.pstStartAddress[iOffset]);
    }
    return NULL;
}


// test if TCB of qwID is valid
// params:
//   qwID: id of a TCB
// return:
//   True if the TCB is a allocated TCB
//   False if the TCB is free or invalid
BOOL kIsTaskExist(QWORD qwID) {
    TCB *pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));

    if ((!pstTCB) || (pstTCB->stLink.qwID != qwID)) {
        return FALSE;
    }
    return TRUE;
}


// return the usage of a processor
// return:
//   usage of a processor
QWORD kGetProcessorLoad(void) {
    return gs_stScheduler.qwProcessorLoad;
}


/* Thread related functions */


// return process TCB that thread belongs to
// params:
//   pstThread: thread TCB
// return:
//   process TCB 
TCB *kGetProcessByThread(TCB *pstThread) {
    TCB *pstProcess;

    if (pstThread->qwFlags & TASK_FLAGS_PROCESS) {
        return pstThread;
    }

    pstProcess = kGetTCBInTCBPool(GETTCBOFFSET(pstThread->qwParentProcessID));

    if (
        (pstProcess == NULL) ||
        (pstProcess->stLink.qwID != pstThread->qwParentProcessID)
    ) {
        return NULL;
    }
    return pstProcess;
}



/* IDLE task related functions */


// this idle task frees all tasks in the wait queue,
// and depending on the processor usage, let the CPU to take a rest
// by using HLT, a assembly instruction
void kIdleTask(void) {
    TCB *pstTask;
    TCB *pstChildThread;
    TCB *pstProcess;
    int i, iCount;
    void *pstThreadLink;
    QWORD qwLastMeasureTickCount;
    QWORD qwLastSpendTickInIdleTask;
    QWORD qwCurrentMeasureTickCount;
    QWORD qwCurrentSpendTickInIdleTask;

    BOOL bPreviousFlag;

    qwLastSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeinIdleTask;
    qwLastMeasureTickCount = kGetTickCount();

    while (TRUE) {
        qwCurrentMeasureTickCount = kGetTickCount();
        qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeinIdleTask;

        if (qwCurrentMeasureTickCount - qwLastMeasureTickCount == 0) {
            gs_stScheduler.qwProcessorLoad = 0;
        }
        else {
            gs_stScheduler.qwProcessorLoad = 
                100 - (qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) *
                100 / (qwCurrentMeasureTickCount - qwLastMeasureTickCount);
        }

        qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;
        qwLastMeasureTickCount = qwCurrentMeasureTickCount;

        kHaltProcessorByLoad();

        if (kGetListCount(&(gs_stScheduler.stWaitList)) > 0) {
            while (TRUE) {
                bPreviousFlag = kLockForSystemData();

                pstTask = kRemoveListFromHeader(&(gs_stScheduler.stWaitList));
                if (!pstTask) {
                    kUnlockForSystemData(bPreviousFlag);
                    break;
                }

                // if task is process, wait for all threads to end
                if (pstTask->qwFlags & TASK_FLAGS_PROCESS) {
                    iCount = kGetListCount(&(pstTask->stChildThreadList));
                    for (i = 0; i < iCount; i++) {
                        pstThreadLink = (TCB *) kRemoveListFromHeader(
                            &(pstTask->stChildThreadList)
                        );
                        if (pstThreadLink == NULL) {
                            break;
                        }
                        pstChildThread = GETTCBFROMTHREADLINK(pstThreadLink);
                        // when thread is in waitlist
                        // the link is disconnected at that time
                        kAddListToTail(
                            &(pstTask->stChildThreadList),
                            &(pstChildThread->stThreadLink)    
                        );

                        kEndTask(pstChildThread->stLink.qwID);
                    }

                    // if child thread is still running, wait for them to end
                    // but is it possible? they are ended up while system is
                    // locked
                    if (kGetListCount(&(pstTask->stChildThreadList)) > 0) {
                        kAddListToTail(&(gs_stScheduler.stWaitList), pstTask);

                        kUnlockForSystemData(bPreviousFlag);
                        continue;
                    }
                    // if all threads are removed
                    // free all allocated memory
                    else {
                        // TODO: add code later
                    }
                }
                else if (pstTask->qwFlags & TASK_FLAGS_THREAD) {
                    pstProcess = kGetProcessByThread(pstTask);

                    if (pstProcess != NULL) {
                        kRemoveList(
                            &(pstProcess->stChildThreadList),
                            pstTask->stLink.qwID
                        );
                    }
                }

                kPrintf(
                    "IDLE: Task ID[0x%q] is completely ended\n",
                    pstTask->stLink.qwID
                );
                kFreeTCB(pstTask->stLink.qwID);
                kUnlockForSystemData(bPreviousFlag);
            }
        }

        kSchedule();
    }
}


// let the CPU to take a rest depending on the load
void kHaltProcessorByLoad(void) {
    if (gs_stScheduler.qwProcessorLoad < 40) {
        kHlt();
        kHlt();
        kHlt();
    }
    else if (gs_stScheduler.qwProcessorLoad < 80) {
        kHlt();
        kHlt();
    }
    else if (gs_stScheduler.qwProcessorLoad < 95) {
        kHlt();
    }
}


/* FPU related functions */

// return ID of the last process that used FPU device
// return:
//   process ID
QWORD kGetLastFPUUsedTaskID(void) {
    return gs_stScheduler.qwLastFPUUsedTaskID;
}


// set which task used the FPU device
// params:
//   process iD
void kSetLastFPUUsedTaskID(QWORD qwTaskID) {
    // warning: original code does not lock system.
    // Isn't it necessary?
    gs_stScheduler.qwLastFPUUsedTaskID = qwTaskID;
}
