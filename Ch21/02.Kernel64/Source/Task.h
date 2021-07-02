#ifndef __TASK_H__
#define __TASK_H__

#include "Types.h"
#include "List.h"


/* default constants for task code */


/** register related constants in task context struct **/

// SS, RSP, RFLAGS, CS, RIP + 19 regs to store in ISR.asm
// 24 regs to store or restore when task switches
#define TASK_REGISTERCOUNT  (5 + 19)

// reg size in long mode
#define TASK_REGISTERSIZE   8

#define TASK_GSOFFSET       0
#define TASK_FSOFFSET       1
#define TASK_ESOFFSET       2
#define TASK_DSOFFSET       3

#define TASK_R15OFFSET      4
#define TASK_R14OFFSET      5
#define TASK_R13OFFSET      6
#define TASK_R12OFFSET      7
#define TASK_R11OFFSET      8
#define TASK_R10OFFSET      9
#define TASK_R9OFFSET       10
#define TASK_R8OFFSET       11
#define TASK_RSIOFFSET      12
#define TASK_RDIOFFSET      13
#define TASK_RDXOFFSET      14
#define TASK_RCXOFFSET      15
#define TASK_RBXOFFSET      16
#define TASK_RAXOFFSET      17
#define TASK_RBPOFFSET      18
#define TASK_RIPOFFSET      19
#define TASK_CSOFFSET       20
#define TASK_RFLAGSOFFSET   21
#define TASK_RSPOFFSET      22
#define TASK_SSOFFSET       23


/** task pool related constants **/

#define TASK_TCBPOOLADDRESS     0x800000
#define TASK_MAXCOUNT           1024

#define TASK_STACKPOOLADDRESS   ( \
    TASK_TCBPOOLADDRESS + sizeof(TCB) * TASK_MAXCOUNT \
)
#define TASK_STACKSIZE  8192

#define TASK_INVALIDID  0xFFFFFFFFFFFFFFFF


/* task scheduler related constants */

// available processor time (ms) for each task.
#define TASK_PROCESSORTIME  5

// number of multi level priority queues */
#define TASK_MAXREADYLISTCOUNT 5

// task priorities
#define TASK_FLAGS_HIGHEST  0
#define TASK_FLAGS_HIGH     1
#define TASK_FLAGS_MEDIUM   2
#define TASK_FLAGS_LOW      3
#define TASK_FLAGS_LOWEST   4

// flag to show that a task is waiting to be removed
#define TASK_FLAGS_WAIT     0xFF

// flag to show that a task is ended
#define TASK_FLAGS_ENDTASK  0x8000000000000000

// flag to show that a task is a idle task
#define TASK_FLAGS_IDLE     0x0800000000000000

// macros to get scheduler related flags
#define GETPRIORITY(x) ((x) & 0xFF)

#define SETPRIORITY(x, priority) ((x) = ((x) & 0xFFFFFFFFFFFFFF00 | (priority)))

#define GETTCBOFFSET(x) ((x) & 0xFFFFFFFF)


/* task related structures */

#pragma pack(push, 1)

// context to save or restore when context switch happens
typedef struct kContextStruct {
    QWORD vqRegister[TASK_REGISTERCOUNT];
} CONTEXT;


// Task Control Block that contains all task-related data.
// This is a item of a linked list (TCB Pool).
typedef struct kTaskControlBlockStruct {
    LISTLINK stLink;
    
    CONTEXT stContext;
    QWORD qwFlags;

    // stack start address
    void *pvStackAddress;
    // stack size
    QWORD qwStackSize;
} TCB;


// manges the state of TCB pool 
typedef struct kTCBPoolManagerStruct {
    // start address of a TCB pool
    TCB *pstStartAddress;

    // maximum number of tasks
    int iMaxCount;
    // number of tasks in use
    int iUseCount;

    // number of tasks executed since OS booting
    int iAllocatedCount;
} TCBPOOLMANAGER;


// manages the state of MINT64OS scheduler
typedef struct kSchedulerStruct {
    // currently running task
    TCB *pstRunningTask;

    // remaining process time for the current task
    int iProcessorTime;

    // task queues that have tasks ready to run
    LIST vstReadyList[TASK_MAXREADYLISTCOUNT];

    // number of execution of each queue
    // this array deterines which queue to select when selecting next task
    int viExecuteCount[TASK_MAXREADYLISTCOUNT];

    // current processor load determines how long idle task should be run
    QWORD qwProcessorLoad;

    // processor time that was spent for idle task
    QWORD qwSpendProcessorTimeinIdleTask;

    // wait task queue that have tasks ready to end
    LIST stWaitList;
} SCHEDULER;

#pragma pack(pop)


/* task pool related functions */


// initialize task pool
void kInitializeTCBPool(void);


// allocate TCB
// return:
//   pointer to a empty TCB
//   if TCB pool is full, null is returned
TCB *kAllocateTCB(void);


// free allocated TCB
// params:
//   qwID: ID of TCB
// info:
//   the qwID must be valid task id in use. Otherwise the result
//   is unpredictable
void kFreeTCB(QWORD qwID);


// a handy function that creates a task
// params:
//   qwFlags: a flag that determines if other task can interrupt this task
//   qwEntryPointAddress: start address of a code
// return:
//   a ready to use task which is put into task queue
//   if task pool is full, null is returned
TCB *kCreateTask(QWORD qwFlag, QWORD qwEntryPointAddress);


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
);


/* task scheduler related functions */


// initialize scheduler
// info:
//   this function also initializes TCB pool together
//   Therefore, no need to initialize TCB pool
void kInitializeScheduler(void);


// return the currently running task
// return:
//   pointer to currently running task's TCB
TCB *kGetRunningTask(void);


// remove the next task in the multi-level priority queues
// return:
//   The higest task's TCB in the multi-level priority queues
//   if there are no tasks in the queues, null is returned
TCB *kGetNextTaskToRun(void);


// add a non-empty task to multi-level priority queues
// params:
//   pstTask: a non-empty task to put into queues
// return:
//   True if the function succeed. Otherwise, False
BOOL kAddTaskToReadyList(TCB *pstTask);


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
void kSchedule(void);


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
BOOL kScheduleInInterrupt(void);


// decrease processor time of current task
void kDecreaseProcessorTime(void);


// check if processor time of current task is expired
// return:
//   True if expired. Otherwise, False
BOOL kIsProcessorTimeExpired(void);


// remove task from multi level priority queues
// params:
//   qwTaskID: task id to remove
// return:
//  task that has qwTaskID
//  if qwTaskID is invalid, null is returned
TCB *kRemoveTaskFromReadyList(QWORD qwTaskID);


// change priority of a task
// params:
//   qwTaskID: task id to change priority
//   bPriority: priority that you want
// return:
//   True if it succeeds. Otherwise False
//   if qwTaskId or bPriority is invalid, False is returned
BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority);


// find a task and finish the task
// params:
//   qwTaskID: task id to end
// return:
//   True if the task is finished.
//   False if the task does not exist
BOOL kEndTask(QWORD qwTaskID);


// let current task end itself
void kExitTask(void);


// get number of tasks in the multi level priority queues
// return:
//   total number of tasks in the queues
int kGetReadyTaskCount(void);


// return overall number of tasks
// return:
//   total number of valid tasks
// note:
//   TCBPool manager has iUseCount variable. I need to ask author why
//   he did not use iCount
int kGetTaskCount(void);


// get a reference to TCB object by TCB index
// params:
//   iOffset: index of TCB to get
// return:
//   pointer to a TCB 
TCB *kGetTCBInTCBPool(int iOffset);


// test if TCB of qwID is valid
// params:
//   qwID: id of a TCB
// return:
//   True if the TCB is a allocated TCB
//   False if the TCB is free or invalid
BOOL kIsTaskExist(QWORD qwID);


// return the usage of a processor
// return:
//   usage of a processor
QWORD kGetProcessorLoad(void);


/* IDLE task related functions */


// this idle task frees all tasks in the wait queue,
// and depending on the processor usage, let the CPU to take a rest
// by using HLT, a assembly instruction
void kIdleTask(void);


// let the CPU to take a rest depending on the load
void kHaltProcessorByLoad(void);

#endif /* __TAsK_H__ */