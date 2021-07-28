#ifndef __SYNCHRONIZATION_H__
#define __SYNCHRONIZATION_H__

#include "Types.h"

#pragma pack(push, 1)

// mutex data structure

typedef struct kMutexStruct {
    // owner of the mutex
    volatile QWORD qwTaskID;
    // lock flag
    volatile BOOL bLockFlag;
    // lock count
    volatile DWORD dwLockCount;

    // padding for aligning by 8 bytes
    BYTE vbPadding[3];
} MUTEX;

#pragma pack(pop)


/* mutex related functions */

// lock all the system so a task can uses kernel data without sync problem 
// return:
//   True if locking successes. Otherwise, False
BOOL kLockForSystemData(void);


// unlock the system
// params:
//   bInterruptFlag: previous interrupt flag returned by kLockForSystemData 
void kUnlockForSystemData(BOOL bInterruptFlag);


// initialize mutex
// params:
//   pstMutex: uninitialized mutex to use
void kInitializeMutex(MUTEX *pstMutex);


// lock mutex
// params:
//   pstMutex: mutex to lock
void kLock(MUTEX *pstMutex);


// unlock mutex
// params:
//   pstMutex: mutex to unlock
void kUnlock(MUTEX *pstMutex);

#endif /* __SYNCHRONIZATION_H__ */