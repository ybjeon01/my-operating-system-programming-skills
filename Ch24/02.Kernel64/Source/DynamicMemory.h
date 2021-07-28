#ifndef __DYNAMICMEMORY_H__
#define __DYNAMICMEMORY_H__

#include "Types.h"
#include "Task.h"

// dynamic address is aligned on 1MB boundaries
#define DYNAMICMEMORY_START_ADDRESS ( \
    (TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * TASK_MAXCOUNT) + 0xFFFFF) & \
    0xFFFFFFFFFFF00000 \
)

// minimum size of buddy block
#define DYNAMICMEMORY_MIN_SIZE  (1 * 1024) // 1KB

// BITMAP flags

// flag for a free block
#define DYNAMICMEMORY_EXIST 0x01

// flag for a used block
#define DYNAMICMEMORY_EMPTY 0x00

#pragma pack(push, 1)

// Binary Tree structure is used to implement Buddy Block algorithm and
// Bitmap structure is used to implement Binary Tree. 
// kBitmapStruct manages nodes of binary tree at each level
typedef struct kBitmapStruct {
    BYTE *pbBitmap;
    QWORD qwExistBitCount;
} BITMAP;

// kDynamicMemoryManagerStruct manages binary tree which is meta data
// for buddy block pool
typedef struct kDynamicMemoryManagerStruct {
    int iMaxLevelCount; // level of binary tree
    int iBlockCountOfSmallestBlock; // total number of binary tree leaves
    QWORD qwUsedSize;   // size of used memory

    QWORD qwStartAddress;   // start address of buddy block pool
    QWORD qwEndAddress; // end address of buddy block pool

    // IN MINT64OS, process requesting dynamic memory gets only memory address.
    // this means when the memory is deallocated, there is no way to know which
    // node of bitmap binary tree correspond to the memory address. This is 
    // because bitmap binary tree nodes at different level can map to the
    //  same memory address. To prevent this problem, below member pointing to
    // an array saves binary map level for each allocated memory.
    // size of the array is number of binary tree leaves because the maximum
    // number of allocation cannot exceed the number of the smallest buddy
    // blocks
    BYTE *pbAllocatedBlockListIndex;
    BITMAP *pstBitmapOfLevel;   // bitmap binary tree
} DYNAMICMEMORY;

#pragma pack(pop)


/* dynamic memory related functions */

// initialize dynamic memory area including meta data about buddy blocks
void kInitializeDynamicMemory(void);


// allocate memory
// params:
//   qwSize: requested memory size
// return:
//   memory address of requested size
void *kAllocateMemory(QWORD qwSize);


// free allocated memory
// params:
//   pvAddress: allocated memory address
// return:
//   True on success. Otherwise False
BOOL kFreeMemory(void *pvAddress);


// get information about dynamic memory
// params:
//   pqwDynamicMemoryStartAddress: address to save dynamic memory start address
//   pqwDynamicMemoryTotalSize: address to save size of dynamic memory
//   pqwMetaDataSize: address to save size of metadata about dynamic memory
//   pqwUsedMemorySize: address to save currently used memory size
void kGetDynamicMemoryInformation(
    QWORD *pqwDynamicMemoryStartAddress,
    QWORD *pqwDynamicMemoryTotalSize,
    QWORD *pqwMetaDataSize,
    QWORD *pqwUsedMemorySize
);


// get dynamic memory manager
// return:
//   pointer to dynamic memory manager
DYNAMICMEMORY *kGetDynamicMemoryManager(void);


// calculate total size of dynamic memory area
// return:
//   total size of dynamic memory area
// info:
//   currently, MINT64OS supports up to 3 GB
static QWORD kCalculateDynamicMemorySize(void);


// calculate necessary amount of blocks for meta data
// params:
//   qwDynamicRAMSize: total ram size for dynamic memory area
// return:
//   number of blocks for meta data
// info:
//   meta data = (bitmap + bitmap metadata + array for allocated memory bitmap
//   level index)
static int kCalculateMetaBlockCount(QWORD qwDynamicRAMSize);


// get block index of total number of blocks at the level that
// qwAlignedSize block exist
// params:
//   qwAlignedSize: requested buddy block size
// return:
//   block index of total number of blocks at the level
// info:
//   qwAlignedSize must be one of buddy block sizes
static int kAllocationBuddyBlock(QWORD qwAlignedSize);


// get size of buddy block in which qwSize fits
// params:
//   qwSize: requested memory size
// return:
//   size of buddy block in which qwSize fits
static QWORD kGetBuddyBlockSize(QWORD qwSize);


// get binary tree level (block list index) at which block of qwAlignedSize
// exist
// params:
//   qwAlignedSize: requested buddy block size
// return:
//   binary tree level (block list index)
static int kGetBlockListIndexOfMatchSize(QWORD qwAlignedSize);


// search bitmap at specified level index for available block offset
// params:
//   iBlockListIndex: binary tree level (block list index)
// return:
//   available block index (index of total number of blocks at the level)
static int kFindFreeBlockInBitmap(int iBlockListIndex);


// set flag to bitmap
// params:
//   iBlockListIndex: binary tree level (block list index)
//   iOffset: index of total number of blocks at iBlockListIndex level
//   bFlag: flag to set
static void kSetFlagInBitmap(int iBlockListIndex, int iOffset, BYTE bflag);


// free buddy block from bitmap
// params:
//   iBlockListIndex: binary tree level (block list index)
//   iBlockOffset: index of the block to free from iBlockList
// return:
//   True on success. Otherwise False
static BOOL kFreeBuddyBlock(int iBlockListIndex, int iBlockOffset);


// get bitmap flag on iOffset of iBlockList
// params:
//   iBlockListIndex: binary tree level (block list index)
//   iOffset: index of the block to free from iBlockList
// return:
//   bitmap flag on iOffset of iBlockList
static BYTE kGetFlagInBitmap(int iBlockListIndex, int iOffset);


#endif /* __DYNAMICMEMORY_H__ */