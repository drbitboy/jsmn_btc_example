////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// Item structure, for parsing JSON format
// Created for KinetX, Inc, for OSIRIS-REx mission
// Brian Carcich, January, 2018
//
// Etymology:  OJI => Osiris-rex Json Item
//
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
#ifndef __ORX_PARSEJSON_H__
#define __ORX_PARSEJSON_H__

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "avltree.h"


////////////////////////////////////////////////////////////////////////
typedef enum
{ OJI_UNKNOWN=0
, OJI_NULL     // JSMN_PRIMITIVE; null
, OJI_BOOLEAN  // JSMN_PRIMITIVE; true or false
, OJI_SCALAR   // JSMN_PRIMITIVE; [-]N[.M[e[-+]EXPONENT] floating point
, OJI_STRING   // JSMN_STRING; "a null-terminated string in quotes"
} OJIENUM;

typedef enum     // Boolean
{ OJI_FALSE=0
, OJI_TRUE
} OJIBOOL;

typedef struct OJITEMstr {
  char* keyString;         // Lookup keyString for instance (Note 1)
  char* sPayload;          // Pointer to riginal string from JSMN (Note 1)
  int strKeyMalloced;      // Set if the pointer is direct malloc() result
  int strPayloadMalloced;  // Set if the pointer is direct malloc() result
  OJIENUM payloadType;     // Payload type (see OJIENUM above)

  union {                  // uPayload: union containing values
    OJIBOOL aBool;         // - single boolean; 0=false
    double aScalar;        // - single number
    char* aString;         // - single string pointer (Note 1)
  } uPayload;

  AVLTREE avltree;         // for AVL tree of multiple instances
} OJITEM, *pOJITEM;

// Note 1:  the payload string (OJITEMstr.sPayload) and the key string
// (OJITEMstr.keyString) will be allocated with the struct OJITEMstr,
// and these pointers set.  the strKeyMalloced and strPayloadMalloced
// flags will be set if their respective strings were independently
// allocated, and freed accord to how those flag are set
//
// In practice, the key and payload strings will be malloc'ed as addons
// to, and with, the OJITEMstr, along with space for null terminators,
// so freeing the pOJITEM will free the space for these strings.

pOJITEM newOji(pOJITEM pSource, char* keyPrefix, int lenStrJson);
void printOjiPayload(pOJITEM pOji, FILE* fOut, char* pfxArg);
void printOjiAvl(pAVLTREE pAvl, int level, void** args);

void orx_getAnyOji(pAVLTREE pAvlRoot, char* searchKeyString, void *pOut, int *pFound, OJIENUM requestedOjiType, int stringOutSize);

void orx_getNullOji(pAVLTREE pAvlRoot, char* searchKeyString, int* pFound);
void orx_getDoubleOji(pAVLTREE pAvlRoot, char* searchKeyString, double* pOut, int* pFound);
void orx_getBooleanOji(pAVLTREE pAvlRoot, char* searchKeyString, OJIBOOL* pOut, int* pFound);
void orx_getStringOji(pAVLTREE pAvlRoot, char* searchKeyString, int stringOutSize, char* pOut, int* pFound);

int readOjiAvl(char* filepath, ppAVLTREE ppAvlTree, char* pfx, FILE *fOut);

#endif // __ORX_PARSEJSON_H__
