#include "Synchronization.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Task.h"

// lock all the system so a task can uses kernel data without sync problem 
// return:
//   True if locking successes. Otherwise, False
BOOL kLockForSystemData(void) {
    return kSetInterruptFlag(FALSE);
}


// unlock the system
// params:
//   bInterruptFlag: previous interrupt flag returned by kLockForSystemData 
void kUnlockForSystemData(BOOL bInterruptFlag) {
    kSetInterruptFlag(bInterruptFlag);
}


// initialize mutex
// params:
//   pstMutex: uninitialized mutex to use
void kInitializeMutex(MUTEX *pstMutex) {
    pstMutex->bLockFlag = FALSE;
    pstMutex->dwLockCount = 0;
    pstMutex->qwTaskID = TASK_INVALIDID;
}


// lock mutex
// params:
//   pstMutex: mutex to lock
void kLock(MUTEX *pstMutex) {

    // current task owns the mutex
    if (pstMutex->qwTaskID == kGetRunningTask()->stLink.qwID) {
        pstMutex->dwLockCount++;
        return;
    }


    /* failure on locking */

    // wait until the mutex is free
    while (kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE) {
        kSchedule();
    }


    /* success in locking */

    pstMutex->dwLockCount = 1;
    pstMutex->qwTaskID = kGetRunningTask()->stLink.qwID;
}


// unlock mutex
// params:
//   pstMutex: mutex to unlock
void kUnlock(MUTEX *pstMutex) {
    
    /* other task owns the mutex */

    if (pstMutex->qwTaskID != kGetRunningTask()->stLink.qwID) {
        return;
    }

    /* current task owns the mutex */

    if (pstMutex->dwLockCount > 1) {
        pstMutex->dwLockCount--;
        return;
    }

    pstMutex->dwLockCount = 0;
    pstMutex->qwTaskID = TASK_INVALIDID;
    pstMutex->bLockFlag = FALSE;
}