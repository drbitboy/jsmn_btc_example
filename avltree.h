#ifndef __AVLTREE_H__
#define __AVLTREE_H__
typedef struct AVLTREEstr {
  int balance;
  struct AVLTREEstr* pLeft;
  struct AVLTREEstr* pRight;
  struct AVLTREEstr* pParent;
  struct AVLTREEstr** ppSelf;

  void* payload;

  /* Comparator returns
   *   negative for *payload1<*payload2, 
   *   zero for *payload1==*payload2,
   *   positive for *payload1>*payload2.
   */
  int (*comparator)(const void* payload1, const void* payload2);
  void (*cleanupPayload)(void* payload);
} AVLTREE, *pAVLTREE, **ppAVLTREE;

void rotateRightAVL(pAVLTREE pRoot);
void rotateLeftAVL(pAVLTREE pRoot);
int insertAvl(ppAVLTREE ppRoot, pAVLTREE pNewAvl);
void* getAVL(pAVLTREE pRoot, void *pPayloadWithKey, int* pCount);
void traverseFromRightAvl(pAVLTREE pRoot, int level, void (*func)(pAVLTREE, int, void**), void** args);
void cleanupAVL(ppAVLTREE ppRoot);
#endif
