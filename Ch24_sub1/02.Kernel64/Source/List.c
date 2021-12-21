#include "List.h"


// initialize List structure
// params:
//   pstList: list to initialize
void kInitializeList(LIST *pstList) {
    pstList->iItemCount = 0;
    pstList->pvHeader = NULL;
    pstList->pvTail = NULL;
}


// get the number of items in a list
// params:
//   pstList: list to count the number
// return:
//   number of items
int kGetListCount(const LIST *pstList) {
    return pstList->iItemCount;
}


// add a item to the end of a list
// params:
//   pstList: list to have the item
//   pvItem: item to add
void kAddListToTail(LIST *pstList, void *pvItem) {
    LISTLINK *pstLink = (LISTLINK *) pvItem;
    pstLink->pvNext = NULL;

    if (!pstList->pvHeader) {
        pstList->pvHeader = pstLink;
        pstList->pvTail = pstLink;
    }
    else {
        pstLink = (LISTLINK *) pstList->pvTail;
        pstLink->pvNext = pvItem;

        pstList->pvTail = pvItem;
    }
    pstList->iItemCount++;
}


// add a item to the head of a list
// params:
//   pstList: list to have the item
//   pvItem: item to add
void kAddListToHeader(LIST *pstList, void *pvItem) {
    LISTLINK *pstLink = (LISTLINK *) pstLink;

    if (!pstList->pvHeader) {
        pstList->pvHeader = pvItem;
        pstList->pvTail = pvItem;
        pstList->iItemCount = 1;
        return;
    }
    else {
        pstLink->pvNext = pstList->pvHeader;
        pstList->pvHeader = pvItem;
        pstList->iItemCount++;
    }
}


// find a item by ID and remove it from a list
// params:
//   pstList: a list to search the item
//   qwID: the item ID
// return:
//   a pointer to the item. If the item does not exist, null is returned
void *kRemoveList(LIST *pstList, QWORD qwID) {

    /* find item by looping */

    LISTLINK *pstPrevLink = (LISTLINK *) pstList->pvHeader;
    LISTLINK *pstLink  = pstPrevLink;
    while (pstLink != NULL) {
        if (pstLink->qwID == qwID) {
            break;
        }
        pstPrevLink = pstLink;
        pstLink = pstLink->pvNext;
    }


    /* process the result of loop */


    /**  empty list or wanted item not in the list **/
    if (pstLink == NULL) {
        return NULL;
    }

    /**  item found and only one item in the list **/
    if (pstList->iItemCount == 1) {
        pstList->pvHeader = NULL;
        pstList->pvTail = NULL;
        pstList->iItemCount = 0;

        return (void *) pstLink;    
    }

    /** item found and more than one item in the list **/

    // item is at header   
    if (pstList->pvHeader == pstLink) {
        pstList->pvHeader = (void *) pstLink->pvNext;
    }
    // item is at tail
    else if (pstList->pvTail == pstLink) {
        pstPrevLink->pvNext = NULL;
        pstList->pvTail = (void *) pstPrevLink;
    }
    // item is in the mid
    else {
        pstPrevLink->pvNext = pstLink->pvNext;
    } 

    pstList->iItemCount--;
    pstLink->pvNext = NULL;
    return (void *) pstLink;
}


// remove the first item of list
// params:
//   pstList: a list to search the item
// return:
//   a pointer to the item. If list is empty, null is returned
void *kRemoveListFromHeader(LIST *pstList) {
    LISTLINK *pstLink;

    if (pstList->iItemCount == 0) {
        return NULL;
    }

    pstLink = (LISTLINK *) pstList->pvHeader;
    return kRemoveList(pstList, pstLink->qwID);
}


// remove the last item of list
// params:
//   pstList: a list to search the item
// return:
//   a pointer to the item. If list is empty, null is returned
void *kRemoveListFromTail(LIST *pstList) {
    LISTLINK *pstLink;

    if (pstList->iItemCount == 0) {
        return NULL;
    }
    pstLink = (LISTLINK *) pstList->pvTail;
    return kRemoveList(pstList, pstLink->qwID);
}


// find a item without removing it from the list
// params:
//   pstList: a list to search the item
//   qwID: the item ID
// return:
//   a pointer to the item. If the item does not exist, null is returned
void *kFindList(const LIST *pstList, QWORD qwID) {

    LISTLINK *pstLink  = pstList->pvHeader;
    while (pstLink != NULL) {
        if (pstLink->qwID == qwID) {
            break;
        }
        pstLink = pstLink->pvNext;
    }
    return (void *) pstLink;
}


// return list's header
// params:
//   pstList: a list
// return:
//   header of the list (it can be NULL)
void *kGetHeaderFromList(const LIST *pstList) {
    return pstList->pvHeader;
}


// return list's tail
// params:
//   pstList: a list
// return:
//   a tail of the list (it can be NULL)
void *kGetTailFromList(const LIST *pstList) {
    return pstList->pvTail;
}


// get next item of the given item
// params:
//   pstCurrent:
//     Current item to get the next item.
//     This item should not be NULL. Otherwise, result is unpredictable 
// return:
//   next item of the current item. It can be NULL
void *kGetNextFromList(void *pstCurrent) {
    LISTLINK *pstLink = (LISTLINK *) pstCurrent;
    return (void *) pstLink->pvNext;
}