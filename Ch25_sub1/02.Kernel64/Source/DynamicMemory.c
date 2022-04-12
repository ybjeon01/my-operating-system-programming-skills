#include "DynamicMemory.h"
#include "Utility.h"
#include "Task.h"
#include "Synchronization.h"
#include "Console.h"

static DYNAMICMEMORY gs_stDynamicMemory;


// initialize dynamic memory area including meta data about buddy blocks
void kInitializeDynamicMemory(void) {
    QWORD qwDynamicMemorySize;
    int i;
    int j;

    BYTE *pbCurrentBitmapPosition;
    int iBlockCountOfLevel;
    int iMetaBlockCount;

    
    /* initialize layout of dynamic memory */

    // get available memory size for dynamic memory allocation
    qwDynamicMemorySize = kCalculateDynamicMemorySize();

    // get required number of minimum size blocks for meta data
    iMetaBlockCount = kCalculateMetaBlockCount(qwDynamicMemorySize);

    // number of min size blocks for buddy block pool
    gs_stDynamicMemory.iBlockCountOfSmallestBlock = 
        (qwDynamicMemorySize / DYNAMICMEMORY_MIN_SIZE) - iMetaBlockCount;
    
    
    /* initialize bitmap binary tree */

    for (i = 0; (gs_stDynamicMemory.iBlockCountOfSmallestBlock >> i) > 0; i++) {
        // get maximum level of binary tree and put te value to variable i.
        // loop until number of nodes becomes zero.
        // (gs_stDynamicMemory.iBlockCountOfSmallestBlock >> i) represents
        // number of nodes at i level.
    }
    gs_stDynamicMemory.iMaxLevelCount = i;

    // start address of pbAllocatedBlockListIndex is the first part of dynamic
    // memory part
    gs_stDynamicMemory.pbAllocatedBlockListIndex = 
        (BYTE *) DYNAMICMEMORY_START_ADDRESS;
    
    // initialize the array with 0xFF
    for (i = 0; i < gs_stDynamicMemory.iBlockCountOfSmallestBlock; i++) {
        gs_stDynamicMemory.pbAllocatedBlockListIndex[i] = 0xFF;
    }

    // start address of metadata for bitmap binary tree
    gs_stDynamicMemory.pstBitmapOfLevel = (BITMAP *) (
        DYNAMICMEMORY_START_ADDRESS +
        sizeof(BYTE) * gs_stDynamicMemory.iBlockCountOfSmallestBlock
    );

    // start address of bitmap binary tree
    pbCurrentBitmapPosition = (
        (BYTE *) gs_stDynamicMemory.pstBitmapOfLevel +
        sizeof(BITMAP) * gs_stDynamicMemory.iMaxLevelCount
    );

    // initialize metadata and bitmap binary tree
    for (j = 0; j < gs_stDynamicMemory.iMaxLevelCount; j++) {
        gs_stDynamicMemory.pstBitmapOfLevel[j].pbBitmap = 
            pbCurrentBitmapPosition;
        gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 0;

        iBlockCountOfLevel = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> j;

        // set 0x00 to bitmap of current level.
        // each byte has 8 blocks. 
        for (i = 0; i < iBlockCountOfLevel / 8; i++) {
            *pbCurrentBitmapPosition = 0x00;
            pbCurrentBitmapPosition++;
        }

        // initialize last byte of current bitmap if number of blocks is not 
        // divided by 8
        if ((iBlockCountOfLevel % 8) != 0) {
            *pbCurrentBitmapPosition = 0x00;

            i = iBlockCountOfLevel % 8;
            // in case that number of blocks at current level is not even.
            // examples: when number of leaves are 20 or 21
            //           number of nodes at root is 1
            // question: isn't it possible that part of memory might not be used
            if ((i % 2) == 1) {
                *pbCurrentBitmapPosition |= (DYNAMICMEMORY_EXIST << (i - 1));
                gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 1;
            }
            pbCurrentBitmapPosition++;
        }
    }

    gs_stDynamicMemory.qwStartAddress = (
        DYNAMICMEMORY_START_ADDRESS + iMetaBlockCount * DYNAMICMEMORY_MIN_SIZE
    );
    gs_stDynamicMemory.qwEndAddress = (
        kCalculateDynamicMemorySize() + DYNAMICMEMORY_START_ADDRESS
    );
    gs_stDynamicMemory.qwUsedSize = 0;

}


// calculate total size of dynamic memory area
// return:
//   total size of dynamic memory area
// info:
//   currently, MINT64OS supports up to 2 GB
static QWORD kCalculateDynamicMemorySize(void) {
    QWORD qwRAMSize;

    // Because part of memory addresses above 3GB is used for video memory
    // The available ram is not consecutive. So, until this split problem is
    // resolved, only 2 GB will be available.
    qwRAMSize = (kGetTotalRAMSize() * 1024 * 1024);
    if (qwRAMSize > (QWORD) 2 * 1024 * 1024 * 1024) {
        qwRAMSize = (QWORD) 2 * 1024 * 1024 * 1024;
    }
    return qwRAMSize - DYNAMICMEMORY_START_ADDRESS;
}


// calculate necessary amount of blocks for meta data
// params:
//   qwDynamicRAMSize: total ram size for dynamic memory area
// return:
//   number of blocks for meta data
// info:
//   meta data = (bitmap + bitmap metadata + array for allocated memory bitmap
//   level index)
static int kCalculateMetaBlockCount(QWORD qwDynamicRAMSize) {
    long lBlockCountOfSmallestBlock;
    DWORD dwSizeOfAllocatedBlockListIndex;
    DWORD dwSizeOfBitmap;
    long i;


    /* calculate size of array for allocated memory bitmap level index */

    lBlockCountOfSmallestBlock = qwDynamicRAMSize / DYNAMICMEMORY_MIN_SIZE;
    dwSizeOfAllocatedBlockListIndex = lBlockCountOfSmallestBlock * sizeof(BYTE);


    /* calculate size of bitmap meta data and bitmap */

    dwSizeOfBitmap = 0;
    for (i = 0; (lBlockCountOfSmallestBlock >> i) > 0; i++) {
        // bitmap metadata
        dwSizeOfBitmap += sizeof(BITMAP);

        // aligned bitmap 
        dwSizeOfBitmap += ((lBlockCountOfSmallestBlock >> i) + 7) / 8;
    }

    return (
        (
            dwSizeOfAllocatedBlockListIndex + 
            dwSizeOfBitmap + 
            DYNAMICMEMORY_MIN_SIZE - 1
        ) / 
        DYNAMICMEMORY_MIN_SIZE
    );
}


// allocate memory
// params:
//   qwSize: requested memory size
// return:
//   memory address of requested size
void *kAllocateMemory(QWORD qwSize) {
    QWORD qwAlignedSize;
    QWORD qwRelativeAddress;
    long lOffset;
    int iSizeArrayOffset;
    int iIndexOfBlockList;

    // get size of buddy block in which qwSize fits
    qwAlignedSize = kGetBuddyBlockSize(qwSize);
    if (qwAlignedSize == 0) {
        return NULL;
    }

    // if requested memory size is bigger than straightly
    // available memory
    if (
        (gs_stDynamicMemory.qwStartAddress +
        gs_stDynamicMemory.qwUsedSize +
        qwAlignedSize) > gs_stDynamicMemory.qwEndAddress     
    ) {
        return NULL;
    }

    // block index of total number of blocks at the level that
    // qwAlignedSize block exist
    lOffset = kAllocationBuddyBlock(qwAlignedSize);
    if (lOffset == -1) {
        return NULL;
    }

    // get binary tree level at which block of qwAlignedSize exist
    iIndexOfBlockList = kGetBlockListIndexOfMatchSize(qwAlignedSize);

    // relative address from block pool
    qwRelativeAddress = qwAlignedSize * lOffset;
    iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;

    gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] =
        (BYTE) iIndexOfBlockList;
    gs_stDynamicMemory.qwUsedSize += qwAlignedSize;

    return (void *) (qwRelativeAddress + gs_stDynamicMemory.qwStartAddress);
}


// get size of buddy block in which qwSize fits
// params:
//   qwSize: requested memory size
// return:
//   size of buddy block in which qwSize fits
static QWORD kGetBuddyBlockSize(QWORD qwSize) {
    long i;
    for (i = 0; i < gs_stDynamicMemory.iMaxLevelCount; i++) {
        if (qwSize <= (DYNAMICMEMORY_MIN_SIZE << i)) {
            return (DYNAMICMEMORY_MIN_SIZE << i);
        }
    }
    return 0;
}


// get block index of total number of blocks at the level that
// qwAlignedSize block exist
// params:
//   qwAlignedSize: requested buddy block size
// return:
//   block index of total number of blocks at the level
// info:
//   qwAlignedSize must be one of buddy block sizes
static int kAllocationBuddyBlock(QWORD qwAlignedSize) {
    int iBlockListIndex;
    int iFreeOffset;
    int i;
    BOOL bPreviousInterruptFlag;

    // level of bitmap binary tree for qwAlignedSize
    iBlockListIndex = kGetBlockListIndexOfMatchSize(qwAlignedSize);
    if (iBlockListIndex == -1) {
        return -1;
    }

    // lock for using dynamic memory manager
    bPreviousInterruptFlag = kLockForSystemData();

    // search from iBlockListIndex level
    for (i = iBlockListIndex; i < gs_stDynamicMemory.iMaxLevelCount; i++) {
        iFreeOffset = kFindFreeBlockInBitmap(i);
        if (iFreeOffset != -1) {
            break;
        }
    }

    // if iFreeOffset is -1 after searching the last block list
    // example:
    //   external fragment
    if (iFreeOffset == -1) {
        kUnlockForSystemData(bPreviousInterruptFlag);
        return -1;
    }

    kSetFlagInBitmap(i, iFreeOffset, DYNAMICMEMORY_EMPTY);

    // if offset of the found block is at above level, divide the block
    if (i > iBlockListIndex) {
        for (i = i - 1; i >= iBlockListIndex; i--) {            
            // set empty flag to left node
            kSetFlagInBitmap(i, iFreeOffset * 2, DYNAMICMEMORY_EMPTY);
            
            kSetFlagInBitmap(i, iFreeOffset * 2 + 1, DYNAMICMEMORY_EXIST);

            // now block at i level is available, so change the iOffset
            iFreeOffset = iFreeOffset * 2;
        }
    }
    kUnlockForSystemData(bPreviousInterruptFlag);
    return iFreeOffset;
}


// get binary tree level (block list index) at which block of qwAlignedSize
// exist
// params:
//   qwAlignedSize: requested buddy block size
// return:
//   binary tree level (block list index)
static int kGetBlockListIndexOfMatchSize(QWORD qwAlignedSize) {
    int i;

    for (i = 0; i < gs_stDynamicMemory.iMaxLevelCount; i++) {
        if (qwAlignedSize <= (DYNAMICMEMORY_MIN_SIZE << i)) {
            return i;
        }
    }
    return -1;
}


// search bitmap at specified level index for available block offset
// params:
//   iBlockListIndex: binary tree level (block list index)
// return:
//   available block index (index of total number of blocks at the level)
static int kFindFreeBlockInBitmap(int iBlockListIndex) {
    int i;
    int iMaxCount;
    BYTE *pbBitmap;
    QWORD *pqwBitmap;
    BITMAP *pstBitmapOfLevel;

    // if bitmap at iBlockListIndex is full
    pstBitmapOfLevel = gs_stDynamicMemory.pstBitmapOfLevel + iBlockListIndex;
    if (pstBitmapOfLevel->qwExistBitCount == 0) {
        return -1;
    }

    // total number of blocks at iBlockListIndex
    iMaxCount =
        gs_stDynamicMemory.iBlockCountOfSmallestBlock >> iBlockListIndex;
    pbBitmap = pstBitmapOfLevel->pbBitmap;

    // examine every 64 bits of bitmap
    for (i = 0; i < iMaxCount;) {
        // if 64 bits are full, keep looping
        if (((iMaxCount - i) / 64) > 0) {
            pqwBitmap = (QWORD *) &(pbBitmap[i / 8]);
            if (*pqwBitmap == 0) {
                i += 64;
                continue;
            }
        }

        // if above 64 bits is not full, examine each bit
        if ((pbBitmap[i / 8] & (DYNAMICMEMORY_EXIST << (i % 8))) != 0) {
            return i;
        }
        i++;
    }
    return -1;
}


// set flag to bitmap
// params:
//   iBlockListIndex: binary tree level (block list index)
//   iOffset: index of total number of blocks at iBlockListIndex level
//   bFlag: flag to set
static void kSetFlagInBitmap(int iBlockListIndex, int iOffset, BYTE bFlag) {
    BITMAP *pstBitmapOfLevel;
    BYTE *pbBitmap;

    pstBitmapOfLevel = gs_stDynamicMemory.pstBitmapOfLevel + iBlockListIndex;
    pbBitmap = pstBitmapOfLevel->pbBitmap;

    // record that block at iOffset is free
    if (bFlag == DYNAMICMEMORY_EXIST) {
        // if the block is not free
        if ((pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) == 0) {
            pstBitmapOfLevel->qwExistBitCount++;
        }
        pbBitmap[iOffset / 8] |= (0x01 << (iOffset % 8));
    }
    else {
        if ((pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) != 0) {
            pstBitmapOfLevel->qwExistBitCount--;
        }
        pbBitmap[iOffset / 8] &= ~(0x01 << (iOffset % 8));
    }
}


// free allocated memory
// params:
//   pvAddress: allocated memory address
// return:
//   True on success. Otherwise False
BOOL kFreeMemory(void *pvAddress) {
    QWORD qwRelativeAddress;
    BYTE *pbAllocatedBlockListIndex;
    int iSizeArrayOffset;
    QWORD qwBlockSize;
    int iBlockListIndex;
    int iBitmapOffset;

    if (pvAddress == NULL) {
        return FALSE;
    }

    qwRelativeAddress = ((QWORD) pvAddress) - gs_stDynamicMemory.qwStartAddress;
    iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;
    pbAllocatedBlockListIndex = gs_stDynamicMemory.pbAllocatedBlockListIndex;

    // if pvAddress was not allocated
    if ( pbAllocatedBlockListIndex[iSizeArrayOffset] == 0xFF) {
        return FALSE;
    }

    /* postprocess for pbAllocatedBlockListIndex */

    iBlockListIndex = (int) pbAllocatedBlockListIndex[iSizeArrayOffset];
    pbAllocatedBlockListIndex[iSizeArrayOffset] = 0xFF;

    /* postprocess for bitmap */

    qwBlockSize = DYNAMICMEMORY_MIN_SIZE << iBlockListIndex;
    iBitmapOffset = qwRelativeAddress / qwBlockSize;

    if (kFreeBuddyBlock(iBlockListIndex, iBitmapOffset) == TRUE) {
        gs_stDynamicMemory.qwUsedSize -= qwBlockSize;
        return TRUE;
    }

    return FALSE;
}


// free buddy block from bitmap
// params:
//   iBlockListIndex: binary tree level (block list index)
//   iBlockOffset: index of the block to free from iBlockList
// return:
//   True on success. Otherwise False
static BOOL kFreeBuddyBlock(int iBlockListIndex, int iBlockOffset) {
    int iBuddyBlockOffset;
    int i;
    BOOL bFlag;
    BOOL bPreviousInterruptFlag;

    // lock for using memory manager
    bPreviousInterruptFlag = kLockForSystemData();

    for (i = iBlockListIndex; i < gs_stDynamicMemory.iMaxLevelCount; i++) {
        kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EXIST);

        /* check if sibling node is free */

        // if current buddy block is left node and sibling is right node
        if (iBlockOffset % 2 == 0) {
            iBuddyBlockOffset = iBlockOffset + 1;
        }
        // if current buddy block is right node and sibling is left node
        else {
            iBuddyBlockOffset = iBlockOffset - 1;
        }
        bFlag = kGetFlagInBitmap(i, iBuddyBlockOffset);

        /* postprocess on bitmap recursively */ 

        // sibling is not free
        if (bFlag == DYNAMICMEMORY_EMPTY) {
            break;
        }

        // set empty to blocks at current level
        kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EMPTY);
        kSetFlagInBitmap(i, iBuddyBlockOffset, DYNAMICMEMORY_EMPTY);

        // set exist to block at upper level in the next loop
        iBlockOffset = iBlockOffset / 2;
    }
    
    kUnlockForSystemData(bPreviousInterruptFlag);
    return TRUE;
}


// get bitmap flag on iOffset of iBlockList
// params:
//   iBlockListIndex: binary tree level (block list index)
//   iOffset: index of the block to free from iBlockList
// return:
//   bitmap flag on iOffset of iBlockList
static BYTE kGetFlagInBitmap(int iBlockListIndex, int iOffset) {
    BYTE *pbBitmap;

    pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;

    if ((pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) != 0x00) {
        return DYNAMICMEMORY_EXIST;
    }
    return DYNAMICMEMORY_EMPTY;
}


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
) {
    *pqwDynamicMemoryStartAddress = DYNAMICMEMORY_START_ADDRESS;
    *pqwDynamicMemoryTotalSize = kCalculateDynamicMemorySize();
    *pqwMetaDataSize = kCalculateMetaBlockCount(
        *pqwDynamicMemoryTotalSize * DYNAMICMEMORY_MIN_SIZE
    );
    *pqwUsedMemorySize = gs_stDynamicMemory.qwUsedSize;
}


// get dynamic memory manager
// return:
//   pointer to dynamic memory manager
DYNAMICMEMORY *kGetDynamicMemoryManager(void) {
    return &gs_stDynamicMemory;
}
