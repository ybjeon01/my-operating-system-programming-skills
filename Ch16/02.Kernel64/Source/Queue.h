#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "Types.h"

#pragma pack(push ,1)

// general type circular array queue
typedef struct kQueueManagerStruct {
    // size of item
    int iDataSize;
    // maximum number of data for array in queue
    int iMaxDataCount;

    // array that holds data
    void *pvQueueArray;
    // index for put method
    int iPutIndex;
    // index for get method
    int iGetIndex;

    // check if the queue is empty or full
    BOOL bLastOperationPut;
} QUEUE;

#pragma pack(pop)


// public function that initializes a generic type circular array queue
// params:
//   pstQueue: pointer to queue structure in Queue.h
//   pvQueueBuffer: pointer to array that items are stored
//   iMaxDataCount: maximum number of items for the queue
//   iDataSize: size of a item
// info:
//   pvQueueBuffer must be equal or bigger than iMaxDataCount * iDataSize
void kInitializeQueue(
    QUEUE *pstQueue,
    void *pvQueueBuffer,
    int iMaxDataCount,
    int iDataSize
);


// public function that checks if queue is full
// params:
//   pstQueue: pointer to queue structure in Queue.h
// return:
//   True if queue is full. Otherwise, False
BOOL kIsQueueFull(const QUEUE *pstQueue);


// public function that checks if queue is empty
// params:
//   pstQueue: pointer to queue structure in Queue.h
// return:
//   True if queue is empty. Otherwise, False
BOOL kIsQueueEmpty(const QUEUE *pstQueue);


// public function that puts a data to queue
// params:
//   pstQueue: pointer to queue structure in Queue.h
//   pvData: a pointer to data whose size is equal to pstQueue->iDataSize
// return:
//   True if adding a item succeeded. Otherwise, False
BOOL kPutQueue(QUEUE *pstQueue, const void *pvData);


// public function that gets data from queue
// params:
//   pstQueue: pointer to queue structure in Queue.h
//   pvData: a pointer to a variable that will hold the data
// return:
//   True if getting a item succeeded. Otherwise, False
BOOL kGetQueue(QUEUE *pstQueue, void *pvData);

#endif /* __QUEUE_H__ */