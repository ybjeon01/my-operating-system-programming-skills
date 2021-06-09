#include "Queue.h"
#include "Utility.h"

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
) {
    pstQueue->iMaxDataCount = iMaxDataCount;
    pstQueue->iDataSize = iDataSize;
    pstQueue->pvQueueArray = pvQueueBuffer;

    pstQueue->iPutIndex = 0; // index for put method
    pstQueue->iGetIndex = 0; // index for get method

    // iPutIndex == iGetIndex means two things in circular queue. One is when
    // queue is empty and the other is when queue is full. bLastOperationPut
    // check if the queue is empty or full
    pstQueue->bLastOperationPut = FALSE;
}


// public function that checks if queue is full
// params:
//   pstQueue: pointer to queue structure in Queue.h
// return:
//   True if queue is full. Otherwise, False
BOOL kIsQueueFull(const QUEUE *pstQueue) {
    if (
        (pstQueue->iGetIndex == pstQueue->iPutIndex) &&
        (pstQueue->bLastOperationPut == TRUE)
    ) {
        return TRUE;
    }
    return FALSE;
}


// public function that checks if queue is empty
// params:
//   pstQueue: pointer to queue structure in Queue.h
// return:
//   True if queue is empty. Otherwise, False
BOOL kIsQueueEmpty(const QUEUE *pstQueue) {
    if (
        (pstQueue->iGetIndex == pstQueue->iPutIndex) &&
        (pstQueue->bLastOperationPut == FALSE)
    ) {
        return TRUE;
    }
    return FALSE;
}


// public function that puts a data to queue
// params:
//   pstQueue: pointer to queue structure in Queue.h
//   pvData: a pointer to data whose size is equal to pstQueue->iDataSize
// return:
//   True if adding a item succeeded. Otherwise, False
BOOL kPutQueue(QUEUE *pstQueue, const void *pvData) {
    if (kIsQueueFull(pstQueue)) {
        return FALSE;
    }

    int stride = (pstQueue->iDataSize * pstQueue->iPutIndex);

    // copy pvData to queue's iPutIndex pointer
    kMemCpy(
        (char *) pstQueue->pvQueueArray + stride,
        pvData,
        pstQueue->iDataSize
    );

    // update iPutIndex and lastOperation
    // this queue is circular queue, so index can not just increased
    pstQueue->iPutIndex = (pstQueue->iPutIndex + 1) % pstQueue->iMaxDataCount;
    pstQueue->bLastOperationPut = TRUE;
    return TRUE;
}


// public function that gets data from queue
// params:
//   pstQueue: pointer to queue structure in Queue.h
//   pvData: a pointer to a variable that will hold the data
// return:
//   True if getting a item succeeded. Otherwise, False
BOOL kGetQueue(QUEUE *pstQueue, void *pvData) {
    if ( kIsQueueEmpty(pstQueue) ) {
        return FALSE;
    }

    int stride = (pstQueue->iDataSize * pstQueue->iGetIndex);

    // copy data in Queue to pvData
    kMemCpy(
        pvData,
        (char *) pstQueue->pvQueueArray + stride,
        pstQueue->iDataSize
    );

    // update iPutIndex and lastOperation
    // this queue is circular queue, so index can not just increased
    pstQueue->iGetIndex = (pstQueue->iGetIndex + 1) % pstQueue->iMaxDataCount;
    pstQueue->bLastOperationPut = FALSE;
    return TRUE;
}