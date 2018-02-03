#include <stdio.h>
#include <string.h>

#include "jsmn.h"
#include "buffer_file.h"
#include "orx_parsejson.h"


/**********************************************************************/
/**********************************************************************/
/*** AVL Tree handling for OJI (OpNav Report Items); lookup is ->keyString */
/**********************************************************************/
/**********************************************************************/

/***************************************/
/* Comparator for avltree in OJITEM */
int
oji_comparator(const void* payload1, const void* payload2) {
  return strcmp(((pOJITEM)payload1)->keyString,((pOJITEM)payload2)->keyString);
}

/**********************************************************************/
/* Free one OJITEM */
void
cleanupOji(void* pPayload) {
pOJITEM pOji = (pOJITEM) pPayload;
  if (pOji) {
    if (pOji->strKeyMalloced && pOji->keyString) { free(pOji->keyString); }
    if (pOji->strPayloadMalloced && pOji->sPayload) { free(pOji->sPayload); }
    memset(pOji,0,sizeof(OJITEM));
    free(pOji);
  }
  return;
}

/**********************************************************************/
/* Allocate new OJITEM, fill in AVLTREE items from another pointer
 * - prepend keyPrefix if keyPrefix a valid pointer and non-null, to ->keyString
 */
pOJITEM
newOji(pOJITEM pSource, char* keyPrefix, int lenStrJson) {
pOJITEM rtn;
int lenKeyPfx = (keyPrefix && *keyPrefix) ? strlen(keyPrefix) : 0;
int lenKeySfx;
int lenKeyTotal;
int szof;

  /* Ensure pSource contains necessary data */
  if (!pSource) return 0;
  if (!pSource->keyString) return 0;
  if (!pSource->sPayload) return 0;
  if (lenStrJson < 0) return 0;

  lenKeySfx = strlen(pSource->keyString);

  lenKeyTotal = lenKeyPfx + lenKeySfx;

  /* Get size required for OJITEM, for keyString, and for payload string */
  szof = sizeof(OJITEM)
       + (lenKeyTotal + 1)
       + ((lenStrJson > 0 ? lenStrJson : 0) + 1)
       ;

  /* Allocate the space */
  rtn = malloc(szof);
  if (!rtn) return rtn;

  /* Copy data from pSource */
  memcpy((void*)rtn,(void*)pSource,sizeof(OJITEM));

  /* Point ->keyString string at end of OJITEM; copy key prefix & suffix */
  rtn->keyString = (char*)(rtn + 1);
  if (lenKeyPfx > 0) { strncpy(rtn->keyString, keyPrefix, lenKeyPfx); }
  strncpy(rtn->keyString + lenKeyPfx, pSource->keyString, lenKeySfx);
  rtn->keyString[lenKeyTotal] = '\0';

  /* Point ->sPayload at first character after ->keyString terminator, 
   * and copy payload string, plus null terminator
   */
  rtn->sPayload = rtn->keyString + lenKeyTotal + 1;
  strncpy(rtn->sPayload, pSource->sPayload, lenStrJson);
  rtn->sPayload[lenStrJson] = '\0';

  /* Point union string pointer at sPayload */
  if (rtn->payloadType == OJI_STRING) { rtn->uPayload.aString = rtn->sPayload; }

  /* Set flags to indicate that string pointers need not be freed */
  rtn->strKeyMalloced =
  rtn->strPayloadMalloced = 0;
  
  /* Set ->avltree parameters */ 
  rtn->avltree.comparator = oji_comparator;
  rtn->avltree.cleanupPayload = cleanupOji;
  rtn->avltree.payload = (void*)rtn;

  return rtn;
}

////////////////////////////////////////////////////////////////////////
// Routines to get data from OJI/AVL tree
// - modeled after SPICE GIPOOL, GDPOOL, GCPOOL

// - Type-agnostic OJI tree search; returns pointer to (AVLTREE).payload
pOJITEM
orx_getOji(pAVLTREE pAvlRoot, char* searchKeyString) {
OJITEM oji;
  // Load search string into local OJI, for getAVL to use comparator
  oji.keyString = searchKeyString;
  // getAVL returns void*, either to payload matching keystring or to NULL
  return (pOJITEM) getAVL( pAvlRoot, &oji, (int*)0);
}

////////////////////////////////////////////////////////////////////////
// Get one value from OJI/AVL tree
// - Design is modeled on NAIF/SPICE CSPICE toolkit gipool_c/gdpool_c/gcpool_c
//   - http://naif.jpl.nasa.gov/
//   - Good coders borrow; great coders steal
// - Arguments
//   - pAvlRoot is root of AVLTREE for OJITEMs
//   - searchKeyString is string key to match
//   - pOut is a pointer to output array to which to copy values
//     - this is a (void*) pointer so all OJIENUM types can be handled
//   - pFound is a pointer to indicate whether the key was found in the AVLTREE
//   - requestedOjiType is the type of value expected, and the type of the array pointed to by (void*)pOut
//     - OJI_BOOLEAN:  pOut points to OJIBOOL
//     - OJI_SCALAR:  pOut points to double (OJITEM->payloadType may be OJI_SCALAR or OJI_VECTOR)
//     - OJI_STRING:  pOut points to first char of first element in array of char[stringOutSize]
//     - OJI_NULL:  pOut is ignore; pFound is set if value is in tree
//   - stringOutSize is length of string(s) pointed to by pOut
void
orx_getAnyOji(pAVLTREE pAvlRoot, char* searchKeyString
             , void *pOut, int *pFound
             , OJIENUM requestedOjiType, int stringOutSize) {
pOJITEM pOji;

double* pDoubleOut   = (double*)   pOut;
int* pIntOut         = (int*)      pOut;
OJIBOOL* pBooleanOut = (OJIBOOL*)  pOut;
char* pStringOut     = (char*)     pOut;

  // Set found boolean for failure
  if (!pFound) return;
  *pFound = 0;

  if (!searchKeyString) return;
  if (!pOut) return;

  // Search for matching key string, return on failures:
  pOji = orx_getOji(pAvlRoot, searchKeyString);

  // - Fail if no match for key string
  if (!pOji) return;

  // - Fail if payload->payloadType is not compatible with requested OJI type
  if (pOji->payloadType != requestedOjiType) return;
    

  // Set ending and next pointers
  switch (pOji->payloadType) {

  default: return;

  case OJI_NULL:
    /* Nothing to do */
    break;

  case OJI_BOOLEAN:
    *((OJIBOOL*) pOut) = pOji->uPayload.aBool;
    break;

  case OJI_SCALAR:
    *((double*) pOut) = pOji->uPayload.aScalar;
    break;

  case OJI_STRING:
    strncpy(pStringOut, pOji->uPayload.aString ? pOji->uPayload.aString : "<missing>", stringOutSize);
    pStringOut[stringOutSize-1] = '\0';
    break;
  }

  // - Set found to true even if no items will be transferred
  *pFound = 1;

  return;
}

// Convenience wrappers for orx_getAnyOji
void
orx_getNullOji(pAVLTREE pAvlRoot, char* searchKeyString, int *pFound) {
void* pOut = (void*) 1;
  orx_getAnyOji(pAvlRoot, searchKeyString, pOut, pFound, OJI_NULL, 0);
  return;
}
void
orx_getDoubleOji(pAVLTREE pAvlRoot, char* searchKeyString, double *pOut, int *pFound) {
  orx_getAnyOji(pAvlRoot, searchKeyString, (void*)pOut, pFound, OJI_SCALAR, 0);
  return;
}
void
orx_getBooleanOji(pAVLTREE pAvlRoot, char* searchKeyString, OJIBOOL *pOut, int *pFound) {
  orx_getAnyOji(pAvlRoot, searchKeyString, (void*)pOut, pFound, OJI_BOOLEAN, 0);
  return;
}
void
orx_getStringOji(pAVLTREE pAvlRoot, char* searchKeyString, int stringOutSize, char *pOut, int *pFound) {
  orx_getAnyOji(pAvlRoot, searchKeyString, (void*)pOut, pFound, OJI_STRING, stringOutSize);
  return;
}

 
/*************************/
/* Print contents of OJI */
void
printOjiPayload(pOJITEM pOji, FILE* fOut, char* pfxArg) {
int iVector;
char* pfx = pfxArg ? pfxArg : "";

  if (!fOut) return;

  switch (pOji->payloadType) {
  case OJI_NULL:
    fprintf(fOut, "%s<null>", pfx);
    break;
  case OJI_SCALAR:
    fprintf(fOut, "%sSCALAR=%lg", pfx, pOji->uPayload.aScalar);
    break;
  case OJI_STRING:
    fprintf(fOut, "%sSTRING=<%s>", pfx, pOji->uPayload.aString);
    break;
  case OJI_BOOLEAN:
    fprintf(fOut, "%sBOOLEAN=<%s>", pfx, pOji->uPayload.aBool ? "TRUE" : "FALSE");
    break;
  default:
    fprintf(fOut, "%sERROR", pfx);
    break;
  }
  return;
}

/**********************************************************************/
/* Copy one OJITEM and insert it into a destination AVLTREE
 * - Callback routine for traverseFromRightAvl to copy a whole tree
 * - N.B. WARNING:  Deletes entire destination AVLTREE if newOji (malloc) fails
 */
void
copyOneOjiAvlTree(pAVLTREE pAvl, int level, void** args)  {
pOJITEM pOji;
ppAVLTREE ppAvlTreeRootDest;
  if (!pAvl) return;
  if (!args) return;
  if (!(ppAvlTreeRootDest=(ppAVLTREE)*args)) return;

  /* Allocate a new OJITEM and copy the payload to it */
  if ((pOji = newOji((pOJITEM) pAvl->payload,0,strlen(((pOJITEM) pAvl->payload)->sPayload)))) {

    /* - if successful, insert the new item into the AVLTREE */
    insertAvl(ppAvlTreeRootDest,&pOji->avltree);

  } else {
    /* - if not successful
     *   - cleanup the copied tree to this point
     *   - set the root pointer of the copied tree to 0
     *     so copyWholeOjiAvlTree will return null
     *   - set the pointer to the root pointer of the copied tree to 0
     *     so no more OJITEMs will be allocated, copied and added
     */
    cleanupAVL(ppAvlTreeRootDest);
    *ppAvlTreeRootDest = 0;
    *args = 0;
  }
  return;
}

/**********************************************************************/
/* Copy an entire AVLTREE of OJITEMs */
pAVLTREE
copyWholeOjiAvlTree(pAVLTREE pOjiAvlTreeSource) {
pAVLTREE pAvlTreeDest = 0;
void* args[1] = { (void*) &pAvlTreeDest };
  traverseFromRightAvl(pOjiAvlTreeSource, 0, copyOneOjiAvlTree, args);
  return pAvlTreeDest;
}

/**********************************************************************/
void
printOjiAvl(pAVLTREE pAvl, int level, void** args) {
FILE* fOut = (FILE*) (args ? args[0] : 0);
char fmt[32];
pOJITEM pOji;
OJITEM localOji;
pAVLTREE pOjiAvlRoot = args ? (args[1] ? *((ppAVLTREE)args[1]) : 0) : 0;
int rtnCount;
int found = -99;
int lenPayloadPlus1;

  if (!pAvl) return;
  if (!fOut) return;
  pOji = (pOJITEM)pAvl->payload;
  if (!pOji) return;

  sprintf(fmt, "%%%ds[(bal=%%d;level=%%d)%%s", level*5);
  fprintf(fOut, fmt, "", pAvl->balance, level, pOji->keyString);
  printOjiPayload(pOji, fOut, ";");

  if (pOjiAvlRoot) {
    switch (pOji->payloadType) {

    case OJI_BOOLEAN:
      orx_getBooleanOji(pOjiAvlRoot, pOji->keyString, &localOji.uPayload.aBool, &found);
      found &= (localOji.uPayload.aBool == pOji->uPayload.aBool) ? 1 : 0;
      break;

    case OJI_SCALAR:
      orx_getDoubleOji(pOjiAvlRoot, pOji->keyString, &localOji.uPayload.aScalar, &found);
      found &= localOji.uPayload.aScalar == pOji->uPayload.aScalar ? 1 : 0;
      break;

    case OJI_NULL:
      orx_getNullOji(pOjiAvlRoot, pOji->keyString, &found);
      break;

    case OJI_STRING:
      lenPayloadPlus1 = pOji->sPayload ? (1+strlen(pOji->sPayload)) : 0;
      if (lenPayloadPlus1) {
        localOji.uPayload.aString =
        localOji.sPayload = (char*) malloc(lenPayloadPlus1);
        if (localOji.sPayload) {
          *localOji.sPayload = '\0';
          orx_getStringOji(pOjiAvlRoot, pOji->keyString, lenPayloadPlus1, localOji.uPayload.aString, &found);
          found &= !strcmp(localOji.uPayload.aString,pOji->uPayload.aString) ? 1 : 0;
          free(localOji.sPayload);
        }
      }
      break;

    default:
      break;
    }
  }
  fprintf(fOut,";%s]\n", found ? (found==-99?"get*Ldt untested":"get*Oji succeeded") : "get*Oji failed");

  return;
}
////////////////////////////////////////////////////////////////////////

int
jsmn_dump_to_avl( ppAVLTREE ppAvlTree
                , const uint8_t* json_buffer
                , jsmntok_t* pToks
                , size_t count
                , char* pKeypfx
                , size_t keyPfxSize
                ) {
size_t keypfxpos = pKeypfx ? strlen(pKeypfx) : 0;
size_t keypfxroom = keyPfxSize - keypfxpos;
char* pKeypfxend = pKeypfx + keypfxpos;
int i;
int j;
char* pLclKeypfx = pKeypfx;
char* pLclKeypfxend = pKeypfxend;
OJITEM localOji;
pOJITEM pOji = 0;

  if (count == 0) { return 0; }
  if (keyPfxSize == 0) { return 0; }
  if (!pKeypfx) { return 0; }

  if (pToks->type == JSMN_STRING || pToks->type == JSMN_PRIMITIVE) {

    localOji.keyString = pKeypfx;
    localOji.sPayload = (uint8_t*) json_buffer + pToks->start;
    localOji.strKeyMalloced =
    localOji.strPayloadMalloced = 0;

    if (pToks->type == JSMN_STRING) {

      localOji.payloadType = OJI_STRING;
      localOji.uPayload.aString = localOji.sPayload;

    } else {
      switch (*localOji.sPayload) {

      case 'n':                             // null
        localOji.payloadType = OJI_NULL;
        break;

      case 't':                             // true
      case 'f':                             // false
        localOji.payloadType = OJI_BOOLEAN;
        localOji.uPayload.aBool = (*localOji.sPayload=='t') ? OJI_TRUE : OJI_FALSE;
        break;

      default:                              // a number
        if ( 1 == sscanf(localOji.sPayload, "%lf", &localOji.uPayload.aScalar)) {
          localOji.payloadType = OJI_SCALAR;
        } else {
          localOji.payloadType = OJI_UNKNOWN;
        }
      } /* switch (*localOji.sPayload) */
    }

    /* Allocate a new OJITEM and copy the payload from localOji to it */
    if ((pOji = newOji(&localOji, 0, pToks->end - pToks->start))) {
      /* - if successful, insert the new item into the AVLTREE */
      insertAvl(ppAvlTree, &pOji->avltree);
      return 1;
    }


  } else if (pToks->type == JSMN_ARRAY || pToks->type == JSMN_OBJECT) {
  int lenAdd;
  char* pSfx;

    /* Loop over tokens contained in container pTok[0]
     * - Start at -1 for array to store arry .length field
     */
    j = 0;
    for (i = -1; i < pToks->size; i++) {

      /* Determine length of suffix to append to keypfx */

      if (pToks->type == JSMN_ARRAY) {

        /* Array:  add index "[i]" or .length - assume 30 characters max */
        lenAdd = 30;

      } else {                 /* JSMN_OBJECT */

        /* Skip -1 case:  length not required for JSMN_OBJECT */
        if (i < 0) continue;

        /* Object:  add name ".<name>" - 1 for dot plus length of name */
        lenAdd = 1 + pToks[1+j].end - pToks[1+j].start;

        /* Save starting address of name */
        pSfx = (uint8_t*) json_buffer + pToks[1+j].start;

        /* Increment index into JSMN tokens */
        ++j;
      }

      /* Ensure pLclKeypfx points to a buffer large enough to append
       * lenAdd characters; double buffer as needed
       */
      while (pLclKeypfx && lenAdd >= keypfxroom) {
        
        if (pLclKeypfx==pKeypfx) {
          /* pLclKeypfx points to buffer argument; malloc new space ... */
          if ((pLclKeypfx=malloc(keyPfxSize <<= 1))) {
            /* ... and copy buffer argument */
            strncpy(pLclKeypfx, pKeypfx, keypfxpos+1);
          }

        } else {
        char* pSave;
          /* pLclKeypfx points to malloced space; realloc double the size */
          pSave = pLclKeypfx;
          if (!(pLclKeypfx=realloc( pLclKeypfx, keyPfxSize <<= 1))) {
            free(pSave);
          }
        }
        pLclKeypfxend = pLclKeypfx + keypfxpos;
        keypfxroom = keyPfxSize - keypfxpos;
      } /* while (pLclKeypfx && lenAdd >= keypfxroom) */

      if (i == -1) {
        strcpy(pLclKeypfxend, ".length");
      } else if (pToks->type == JSMN_ARRAY) {
        sprintf(pLclKeypfxend,"[%d]", i);
      } else {
        sprintf(pLclKeypfxend,".%.*s", lenAdd-1, pSfx);
      }

      if (i == -1) {
      char sBuffer30[30];
      jsmntok_t tmpTok;

        sprintf(sBuffer30, "%d", pToks->size);

        tmpTok.type = JSMN_PRIMITIVE;
        tmpTok.start = 0;
        tmpTok.end = strlen(sBuffer30);
        /* Ignore .size and .parent */

        jsmn_dump_to_avl( ppAvlTree
                        , sBuffer30
                        , &tmpTok
                        , 1
                        , pLclKeypfx
                        , strlen(pLclKeypfx)
                        );
        continue;
      } else {

        j += jsmn_dump_to_avl( ppAvlTree
                             , json_buffer
                             , pToks+1+j
                             , count-j
                             , pLclKeypfx
                             , keyPfxSize
                             );

      }
    } // for (j=i=0; ... )
    if (pLclKeypfx!=pKeypfx) {
      free(pLclKeypfx);
    }
    if (pKeypfx) {
      *pKeypfxend = '\0';
    }
    return j+1;
  } /* else if (pToks->type == JSMN_ARRAY && pToks->type == JSMN_OBJECT) */
  return 0;
}

/**********************************************************************/
int
readOjiAvl(char* filepath, ppAVLTREE ppAvlTree, char* pfx, FILE *fOut) {
size_t json_len;
uint8_t* json_buffer;
size_t tokcount = 64;
jsmntok_t* pToks;
jsmn_parser jp;
int rtn = 0;
int parse_rtn;
char keypfx[BUFSIZ] = { "json" };

# define PRTERR(S,RTN) fprintf(stderr, "%s\n", S); rtn = RTN

  if (!rtn && !ppAvlTree) {
    PRTERR("readOjiAvl(...) null ppAVLTREE pointer", 1);
  }
  if (!rtn && !(json_buffer=buffile_file_to_puint8( filepath, &json_len, 0))) {
    PRTERR("readOjiAvl(...) failed to read file into memory buffer", 2);
  }
  if (!rtn && !(pToks = realloc(0, sizeof(jsmntok_t) * tokcount))) {
    PRTERR("readOjiAvl(...) failed to allocate tokens", 3);
  }

  jsmn_init(&jp);

  while (!rtn && JSMN_ERROR_NOMEM == (parse_rtn = jsmn_parse(&jp, json_buffer, json_len, pToks, tokcount))) {
  void* old_pToks;
    old_pToks = pToks;
    pToks = realloc( old_pToks, sizeof(jsmntok_t) * (tokcount <<= 1));
    if (!pToks) {
      free(old_pToks);
      PRTERR("readOjiAvl(...) failed to allocate tokens", 3);
      rtn = 4;
      continue;
    }
  }

  if (!rtn && parse_rtn == JSMN_ERROR_INVAL) {
    PRTERR("readOjiAvl(...) jsmn_parse() error; e.g. invalid character", 4);
  }
  if (!rtn && parse_rtn == JSMN_ERROR_PART) {
    PRTERR("readOjiAvl(...) jsmn_parse() error; incomplete JSON", 5);
  }
  if (!rtn && parse_rtn == 0) {
    PRTERR("readOjiAvl(...) jsmn_parse() error; possbly empty JSON file", 6);
  }
  if (!rtn && parse_rtn < 0) {
    PRTERR("readOjiAvl(...) jsmn_parse() error; unknown cause", 7);
  }

  if (!rtn) {
    jsmn_dump_to_avl(ppAvlTree, json_buffer, pToks, jp.toknext, keypfx, BUFSIZ);
  }

  if (json_buffer) { free(json_buffer); }
  if (pToks) { free(pToks); }
  return 0;
} // int readOjiAvl(char* filepath, ppAVLTREE ppAvlTree, char* pfx , FILE *fOut) {
/**********************************************************************/
/*** End of library functions ****************************************/
/**********************************************************************/
/**********************************************************************/


/**********************************************************************/
/*** Test program ***/
/*
 * Usage:
 *
 *   ./test_orx_parsejson a.json
 *
 * Compile and link:
 *
 *  % gcc -DDO_MAIN orx_parsejson.c -o test_orx_parsejson
 *
 */
#ifdef DO_MAIN

#include "jsmn.c"
#include "avltree.c"
#define main MAIN_BUFFILE
#include "buffer_file.c"
#undef main

int
main(int argc, char** argv) {

/* Pointer to AVL tree */
pAVLTREE pOjiAvlTree = (pAVLTREE) NULL;
pAVLTREE pOjiAvlTreeCopy = (pAVLTREE) NULL;

void* pVoid2[2] = { (void*) stdout, (void*) &pOjiAvlTree };

  while (--argc) {
    readOjiAvl(argv[argc], &pOjiAvlTree, 0, stdout);

    traverseFromRightAvl(pOjiAvlTree, 0, printOjiAvl, pVoid2);

    pOjiAvlTreeCopy = copyWholeOjiAvlTree(pOjiAvlTree);
    fprintf(stdout,"\n#######################################################################\n");

    pVoid2[1] = (void*) &pOjiAvlTreeCopy;
    traverseFromRightAvl(pOjiAvlTreeCopy, 0, printOjiAvl, pVoid2);

    cleanupAVL(&pOjiAvlTree);

    fprintf(stdout,"\n#######################################################################\n");
    traverseFromRightAvl(pOjiAvlTreeCopy, 0, printOjiAvl, pVoid2);
    cleanupAVL(&pOjiAvlTreeCopy);
  }

  return 0;
}
#endif // DO_MAIN
