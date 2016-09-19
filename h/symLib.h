/* symLibP.h - private symbol library header file */

#ifndef __INCsymLibPh
#define __INCsymLibPh

#include <hashLib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* defines */

/* allow write access to symbol table through symEachCall() */

#define SYM_EACH_CALL_WRITE 0x1 

/* Maximum number of concurrent symbol table reader */

#define MAX_SYM_READERS 10

/* Supported search type for symFind() routine (Can be OR'ed) */

typedef enum            /* symFind() mask values (Can be OR'ed) */
    {
    SYM_FIND_BY_NAME    = 0x1,      /* 0x1: Search symbol by name */
    SYM_FIND_BY_VALUE   = 0x2,      /* 0x2: Search symbol by value */
    SYM_FIND_BY_TYPE    = 0x4       /* 0x4: Search symbol by type */
    } SYM_FIND_MASK;

typedef struct symbol_desc   /* SYMBOL_DESC - symbol descriptor for symFind() */
    {
    SYM_FIND_MASK   mask;   /* In: Search filter mask */
    char *          name;     /* In - If SYM_FIND_BY_NAME not specified:      */
                              /*      Pointer to buffer to return symbol name.*/
                              /*      NULL to let symFind() allocate buffer   */
                              /* Out: Pointer to buffer that holds symbol     */
                              /*      name. This pointer has to be freed by   */
                              /*      the caller if a NULL pointer value      */
                              /*      was provided (buffer allocated by       */
                              /*      symFind() itself)                       */
    unsigned long          nameLen;  /* In:  Length of <name> buffer if provided     */
                              /* Out: Length of entire symbol name with EOS   */
    SYM_VALUE       value;  /* In: Value of symbol to find */
                    /* Out: Exact symbol value */
    SYM_TYPE        type;   /* In: Type of symbol to find */
                    /* Out: Exact symbol type */
    SYM_TYPE        typeMask; /* In:  Bits in <type> to pay attention to      */
    } SYMBOL_DESC; 

/* typedefs */

typedef int (*SYM_EACH_RTN_FUNCPTR) (char * name, SYM_VALUE val,
                     SYM_TYPE type, int arg,
                     SYM_GROUP group);

/* structure definitions */

typedef struct symtab   /* SYMTAB - symbol table */
    {
    HANDLE  handle;     /* object maintanance */
    HASH_ID nameHashId; /* hash table for names */
    int sameNameOk; /* symbol table name clash policy */
    unsigned long   nsymbols;   /* current number of symbols in table */
    } SYMTAB;

typedef struct symtab * SYMTAB_ID;

/* Hooks for symbol management library */

extern SYMBOL_ID (* _func_symEach)
    (
    SYMTAB_ID   symTblId,   /* ID of symbol table to look in */
    int (*routine)  /* func to call for each tbl entry  */
            (
            char * name,    /* symbol/entry name        */
            SYM_VALUE val,  /* symbol/entry value       */
            SYM_TYPE type,  /* symbol/entry type        */
            int arg,    /* arbitrary user-supplied arg  */
            SYM_GROUP group /* symbol/entry group number    */
            ),
    int routineArg  /* arbitrary user-supplied arg */
    );

extern SYMBOL_ID (* _func_symEachCall)
    (
    SYMTAB_ID   symTblId,   /* ID of symbol table to look in */
    int (*routine)  /* func to call for each tbl entry  */
            (
            char * name,    /* symbol/entry name        */
            SYM_VALUE val,  /* symbol/entry value       */
            SYM_TYPE type,  /* symbol/entry type        */
            int arg,    /* arbitrary user-supplied arg  */
            SYM_GROUP group /* symbol/entry group number    */
            ),
    int routineArg, /* arbitrary user-supplied arg */
    int     flags
    );

extern int (* _func_symFind)
    (
    SYMTAB_ID       symTblId,   /* ID of symbol table to look in */
    SYMBOL_DESC *   pSymDesc    /* pointer to symbol descriptor */
    ) ;

extern int (* _func_symFindSymbol)
    (
    SYMTAB_ID   symTblId,       /* symbol table ID */
    char *      name,           /* name to search for */
    SYM_VALUE   value,      /* value of symbol to search for */
    SYM_TYPE    type,           /* symbol type */
    SYM_TYPE    mask,           /* type bits that matter */
    SYMBOL_ID * pSymbolId       /* where to return id of matching symbol */
    );

extern int (* _func_symNameGet)
    (
    SYMBOL_ID  symbolId,    /* symbol table ID */
    char **    pName        /* pointer to symbol name */
    );

extern int (* _func_symValueGet)
    (
    SYMBOL_ID   symbolId,       /* symbol table ID */
    SYM_VALUE * pValue          /* pointer to symbol value */
    );

extern int (* _func_symTypeGet)
    (
    SYMBOL_ID  symbolId,    /* symbol table ID */
    SYM_TYPE * pType        /* pointer to symbol type */
    );

extern SYMBOL_ID (* _func_symRegister)
    (
    SYMTAB_ID   symTblId,   /* Symbol table to add symbol to      */
    char *  name,       /* Pointer to symbol name string      */
    SYM_VALUE   value,      /* Symbol's address               */
    SYM_TYPE    type,           /* Symbol's type  */
    SYM_GROUP   group,          /* symbol group */
    SYM_REF     symRef,     /* module id, SYMREF_SHELL, or SYMREF_NONE */ 
    int        callSyncRtn     /* Try to call the synchronization rtn? */
    );

extern int (* _func_symTblShutdown)
    (
    SYMTAB_ID symTblId          /* ID of symbol table to delete */
    );

/* function declarations */

extern int  symByCNameFind      (SYMTAB_ID symTblId, char * name,
                     SYM_VALUE * pValue, SYM_TYPE * pType);

extern int  symFindSymbol       (SYMTAB_ID symTblId, char * name,
                     SYM_VALUE value, SYM_TYPE type,
                     SYM_TYPE mask, SYMBOL_ID * pSymbolId);
extern int  symNameGet      (SYMBOL_ID symbolId, char ** pName);
extern SYMBOL_ID symRegister        (SYMTAB_ID symTblId, char * name,
                     SYM_VALUE value, SYM_TYPE type,
                     SYM_GROUP group, SYM_REF symRef, 
                     int callSyncRtn);
extern int  symTblShutdown      (SYMTAB_ID symTblId );
extern int  symTblSymSetRemove  (SYMTAB_ID symTblId,
                     SYM_REF reference, SYM_TYPE type,
                     SYM_TYPE typeMask);
extern int  symTypeGet      (SYMBOL_ID symbolId, SYM_TYPE * pType);
extern int  symValueGet     (SYMBOL_ID symbolId,
                     SYM_VALUE * pValue);
extern int  symFree         (SYMTAB_ID symTblId, SYMBOL * pSymbol);
extern int  symTblAdd       (SYMTAB_ID symTblId, SYMBOL * pSymbol);
extern int  symTblRemove        (SYMTAB_ID symTblId, SYMBOL * pSymbol);
extern SYMBOL_ID symEachCall        (SYMTAB_ID symTblId,
                     SYM_EACH_RTN_FUNCPTR routine,
                     int routineArg, int flags);
extern int symFind (SYMTAB_ID symTblId,SYMBOL_DESC *pSymDesc); 

#ifdef __cplusplus
}
#endif

#endif /* __INCsymLibPh */
