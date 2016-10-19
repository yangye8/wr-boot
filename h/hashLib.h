/* hashLib.h - hash table library header */

#ifndef __INChashLibh
#define __INChashLibh

#include <sllLib.h>

typedef int (*FUNCPTR) ();

#ifdef __cplusplus
extern "C" {
#endif

/* status codes */
#define M_hashLib               (58 << 16)
#define S_hashLib_KEY_CLASH     (M_hashLib | 1)

typedef SL_NODE HASH_NODE;  /* HASH_NODE */


/* size of hash table given number of elements in hash table log 2 */

typedef int (* HASH_KEY_CMP_FUNC)(HASH_NODE *pNode, HASH_NODE *pHNode,int arg);
typedef int (*HASH_FUNC) (int elements, HASH_NODE * pHNode, int arg);

#define HASH_TBL_SIZE(sizeLog2)                     \
    (((1 << (sizeLog2)) * sizeof (SL_LIST)) + sizeof (HASH_TBL))

/* type definitions */

typedef struct hashtbl      /* HASH_TBL */
    {
    HASH_KEY_CMP_FUNC keyCmpRtn;/* comparator function */
    HASH_FUNC     keyRtn;   /* hash function */
    SL_LIST *     pHashTbl; /* pointer to hash table array */
    int keyArg; /* hash function argument */
    int       elements; /* number of elements in table */
    } HASH_TBL;

/* type definitions */

typedef struct hashtbl * HASH_ID;

/* These hash nodes are used by the hashing functions in hashLib(1) */

typedef struct          /* H_NODE_INT */
    {
    HASH_NODE     node;         /* linked list node (must be first) */
    int key;            /* hash node key */
    int data;           /* hash node data */
    } H_NODE_INT;

typedef struct          /* H_NODE_STRING */
    {
    HASH_NODE   node;           /* linked list node (must be first) */
    char *  string;         /* hash node key */
    int data;           /* hash node data */
    } H_NODE_STRING;

typedef int (*HASH_FUNC) (int elements, HASH_NODE * pHNode, int arg);
typedef int (* HASH_KEY_CMP_FUNC)(HASH_NODE *pNode, HASH_NODE *pHNode, 
                               int arg);

/* function declarations */

extern int      hashKeyCmp (H_NODE_INT * pMatchHNode,
                    H_NODE_INT * pHNode, 
                    int keyCmpArg);
extern int      hashKeyStrCmp (H_NODE_STRING * pMatchHNode,
                       H_NODE_STRING * pHNode, 
                       int keyCmpArg);
extern HASH_ID      hashTblCreate (int sizeLog2, FUNCPTR keyCmpRtn,
                       FUNCPTR keyRtn, int keyArg);
extern HASH_NODE *  hashTblEach (HASH_ID hashId,
                     int (* routine)
                     (
                     HASH_NODE * pNode,
                     int routineArg
                     ),
                     int routineArg);
extern HASH_NODE *  hashTblFind (HASH_ID hashId, HASH_NODE * pMatchNode,
                                 int keyCmpArg);
extern int      hashLibInit (void);
extern int      hashTblDelete (HASH_ID hashId);
extern int      hashTblDestroy (HASH_ID hashId, int dealloc);
extern int      hashTblInit (HASH_ID pHashTbl, SL_LIST * pTblMem,
                     int sizeLog2, FUNCPTR keyCmpRtn,
                     FUNCPTR keyRtn, int keyArg);
extern int      hashTblPut (HASH_ID hashId, HASH_NODE * pHashNode);
extern int      hashTblRemove (HASH_ID hashId, HASH_NODE * pHashNode);
extern int      hashTblTerminate (HASH_ID hashId);
extern int      hashFuncIterScale (int elements, H_NODE_STRING * pHNode,
                       int seed);
extern int      hashFuncModulo (int elements, H_NODE_INT * pHNode,
                            int divisor);
extern int      hashFuncMultiply (int elements, H_NODE_INT * pHNode,
                                      int multiplier);

#ifdef __cplusplus
}
/*
 * Inlined C++ wrapper for the old-style FUNCPTR based hashTblEach function
 * prototype.
 */

extern HASH_NODE * hashTblEach (HASH_ID hashId, FUNCPTR routine,
                int routineArg) \
       _WRS_DEPRECATED ("please use fully qualified function pointer "
                "version of API");

inline HASH_NODE * hashTblEach
    (
    HASH_ID hashId,
    FUNCPTR routine,
    int routineArg
    )
    {
    return hashTblEach (hashId,
            (int (*)(HASH_NODE * pNode,
                  int routineArg))routine,
            routineArg);
    }
#endif /* __cplusplus */

#endif /* __INChashLibh */
