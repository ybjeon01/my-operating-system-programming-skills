#ifndef __LIST_H__
#define __LIST_H__

#include "Types.h"

#pragma pack(push, 1)


/* basic structures to use for list */

// generic struct that should be inserted into item struct as first member
typedef struct kListLinkStruct {
    void *pvNext;
    QWORD qwID;
} LISTLINK;


typedef struct kListManagerStruct {
    int iItemCount;
    void *pvHeader;
    void *pvTail;
} LIST;

#pragma pack(pop)


/* List methods */

// initialize List structure
// params:
//   pstList: list to initialize
void kInitializeList(LIST *pstList);


// get the number of items in a list
// params:
//   pstList: list to count the number
// return:
//   number of items
int kGetListCount(const LIST *pstList);


// add a item to the end of a list
// params:
//   pstList: list to have the item
//   pvItem: item to add
void kAddListToTail(LIST *pstList, void *pvItem);


// add a item to the head of a list
// params:
//   pstList: list to have the item
//   pvItem: item to add
void kAddListToHeader(LIST *pstList, void *pvItem);


// find a item by ID and remove it from a list
// params:
//   pstList: a list to search the item
//   qwID: the item ID
// return:
//   a pointer to the item. If the item does not exist, null is returned
void *kRemoveList(LIST *pstList, QWORD qwID);


// remove the first item of list
// params:
//   pstList: a list to search the item
// return:
//   a pointer to the item. If list is empty, null is returned
void *kRemoveListFromHeader(LIST *pstList);


// remove the last item of list
// params:
//   pstList: a list to search the item
// return:
//   a pointer to the item. If list is empty, null is returned
void *kRemoveListFromTail(LIST *pstList);


// find a item without removing it from the list
// params:
//   pstList: a list to search the item
//   qwID: the item ID
// return:
//   a pointer to the item. If the item does not exist, null is returned
void *kFindList(const LIST *pstList, QWORD qwID);


// return list's header
// params:
//   pstList: a list
// return:
//   header of the list (it can be NULL)
void *kGetHeaderFromList(const LIST *pstList);


// return list's tail
// params:
//   pstList: a list
// return:
//   a tail of the list (it can be NULL)
void *kGetTailFromList(const LIST *pstList);


// get next item of the given item
// params:
//   pstCurrent:
//     Current item to get the next item.
//     This item should not be NULL. Otherwise, result is unpredictable 
// return:
//   next item of the current item. It can be NULL
void *kGetNextFromList(void *pstCurrent);

#endif /* __LIST_H__ */