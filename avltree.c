#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "avltree.h"

/***********************************************************************
 *** Create and manage AVL trees, given pointers to the data and a comparator
 **********************************************************************/

/***************/
/* Rotate left */

void rotateRightAvl(pAVLTREE pRoot) {
/* Save old pRoot->pLeft in pPivot */
pAVLTREE pPivot = pRoot->pLeft;

  /* Transfer pRight in pivot to pLeft of pRoot */
  pRoot->pLeft = pPivot->pRight;
  if (pRoot->pLeft) {
    pRoot->pLeft->pParent = pRoot;
    pRoot->pLeft->ppSelf = &pRoot->pLeft;
  }

  /* Transfer pRoot parent to pPivot parent */
  pPivot->ppSelf = pRoot->ppSelf;
  *pPivot->ppSelf = pPivot;
  pPivot->pParent = pRoot->pParent;

  /* Transfer pRoot to pRight of pPivot */
  pRoot->ppSelf = &pPivot->pRight;
  *pRoot->ppSelf = pRoot;
  pRoot->pParent = pPivot;

  return;
}

/****************/
/* Rotate right */

void rotateLeftAvl(pAVLTREE pRoot) {
pAVLTREE pPivot = pRoot->pRight;

  /* Transfer pLeft in pivot to pRight of pRoot */
  pRoot->pRight = pPivot->pLeft;
  if (pRoot->pRight) {
    pRoot->pRight->pParent = pRoot;
    pRoot->pRight->ppSelf = &pRoot->pRight;
  }

  /* Transfer pRoot parent to pPivot parent */
  pPivot->ppSelf = pRoot->ppSelf;
  *pPivot->ppSelf = pPivot;
  pPivot->pParent = pRoot->pParent;

  /* Transfer pRoot to pLeft of pPivot */
  pRoot->ppSelf = &pPivot->pLeft;
  *pRoot->ppSelf = pRoot;
  pRoot->pParent = pPivot;

  return;
}

/********************************/
/* Delete tree starting at root */
void
cleanupAVL(ppAVLTREE ppRoot) {
  if (ppRoot) {
  pAVLTREE pRoot = *ppRoot;
    if (pRoot) {
    pAVLTREE pLeft = pRoot->pLeft;
    pAVLTREE pRight = pRoot->pRight;
      if (pRoot->cleanupPayload) {
        pRoot->cleanupPayload(pRoot->payload);
      }
      cleanupAVL(&pLeft);
      cleanupAVL(&pRight);
    }
    *ppRoot = 0;
  }
  return;
}

/*************/
/* Insertion */

int insertAvl(ppAVLTREE ppRoot, pAVLTREE pNewAvl) {
pAVLTREE pRoot = *ppRoot;
int comp;

  /* If pRoot (*ppRoot) is null, add pNewAvl there */
  if (! pRoot) {
    pNewAvl->pLeft = pNewAvl->pRight = pNewAvl->pParent = pRoot;
    pNewAvl->balance = 0;
    pNewAvl->ppSelf = ppRoot;
    *ppRoot = pNewAvl;
    return 1;
  }

  /* Compare pNewAvl to pRoot (ie. to *ppRoot) */
  comp = pNewAvl->comparator(pNewAvl->payload, pRoot->payload);

  if (comp==0) {
    /* *pNewAVL is equal to *pRoot; replace *pRoot with *pNewAvl */

    /* - Copy balance from *pRoot to *pNewAvl */
    pNewAvl->balance = pRoot->balance;

    /* - Copy links from *pRoot to its children and parents to *pNewAvl */
    pNewAvl->pLeft = pRoot->pLeft;
    pNewAvl->pRight = pRoot->pRight;
    pNewAvl->pParent = pRoot->pParent;
    pNewAvl->ppSelf = pRoot->ppSelf;

    /* - Reset pointer to *pRoot in *pRoot parent to be pointer to *pNewAvl */
    *pNewAvl->ppSelf = pNewAvl;

    /* - Reset parent pointers in children */
    if (pNewAvl->pLeft) {
      pNewAvl->pLeft->ppSelf= &pNewAvl->pLeft;
      pNewAvl->pLeft->pParent = pNewAvl;
    }
    if (pNewAvl->pRight) {
      pNewAvl->pRight->ppSelf= &pNewAvl->pRight;
      pNewAvl->pRight->pParent = pNewAvl;
    }

    pRoot->pLeft = pRoot->pRight = 0;
    cleanupAVL(&pRoot);

    return 0;
  }

  /* *pNewAVL is less or greater than *pRoot; add to, or as, pLeft or pRight */
  if (insertAvl(comp < 0 ? &pRoot->pLeft : &pRoot->pRight, pNewAvl)) {

    /* To here, we just added pNewAvl as pRoot->pLeft or ->pRight */
    /* - update pNewAvl parent */
    pNewAvl->pParent = pRoot;

    /* Do rotations as needed to re-balance the tree */
    do {

      if (pNewAvl==pRoot->pLeft) {

        if (pRoot->balance==1) {
          if (pNewAvl->balance==-1) {
            /* Convert Left-Right case to Left-Left case */
            pNewAvl->balance = (pNewAvl->pRight->balance==-1) ? 1 : 0;
            pRoot->balance = (pNewAvl->pRight->balance==1) ? -1 : 0;
            pNewAvl->pRight->balance = 0;
            rotateLeftAvl(pNewAvl);
          } else {
            pNewAvl->balance =
            pRoot->balance = 0;
          }
          /* Balance Left-Left case */
          rotateRightAvl(pRoot);
          break;
        }

        if (++pRoot->balance==0) break;

      } else {

        if (pRoot->balance==-1) {
          if (pNewAvl->balance==1) {
            /* Convert Right-Left case to Right-Right case */
            pNewAvl->balance = (pNewAvl->pLeft->balance==1) ? -1 : 0;
            pRoot->balance = (pNewAvl->pLeft->balance==-1) ? 1 : 0;
            pNewAvl->pLeft->balance = 0;
            rotateRightAvl(pNewAvl);
          } else {
            pNewAvl->balance =
            pRoot->balance = 0;
          }
          /* Balance Right-Right case */
          rotateLeftAvl(pRoot);
          break;
        }


        if (--pRoot->balance==0) break;
      }

      pNewAvl = pRoot;
      pRoot = pRoot->pParent;

    } while (pRoot);
  }
  return 0;
}

/******************************************/
/* Find item matching key, or return NULL */

void* getAVL(pAVLTREE pRoot, void *pPayloadWithKey, int* pCount) {
int comp;
  if (pCount) ++*pCount;
  if (!pRoot) return (void*) NULL;
  comp = pRoot->comparator(pPayloadWithKey, pRoot->payload);
  if (comp==0) return pRoot->payload;
  if (comp>0) return getAVL(pRoot->pRight, pPayloadWithKey, pCount);
  return getAVL(pRoot->pLeft, pPayloadWithKey, pCount);
}

/********************************/
/* Traverse tree, right to left, call handler for each pointer,
 * whether null or not
 */
void traverseFromRightAvl(pAVLTREE pRoot, int level, void (*handler)(pAVLTREE, int, void**), void** args) {
  if (!pRoot) return;
  traverseFromRightAvl(pRoot->pRight,level+1,handler,args);
  handler(pRoot,level,args);
  traverseFromRightAvl(pRoot->pLeft,level+1,handler,args);
  return;
}
