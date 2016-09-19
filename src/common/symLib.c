#include <wrboot.h>
#include <hashLib.h>
#include <symbol.h>
#include <symLib.h>
#include <sllLib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SYM_TBL_HASH_SIZE_LOG2  8       /* 256 entry hash table symbol table */

#define SYM_HFUNC_SEED  1370364821              /* magic seed */

#define MAX_SYS_SYM_LEN 256 /* system symbols will not exceed this limit */

#define N_EXT   1       /* External symbol (OR'd in with one of above)  */


extern SYMBOL       standTbl[]; /* standalone symbol table array */
extern unsigned int standTblSize;   /* symbols in standalone table */

SYMTAB_ID       sysSymTbl;              /* system symbol table id */

/* locals */

LOCAL int  symCount = 0;                /* number of symbols printed */

/* globals */

unsigned int symLkupPgSz = 22;              /* max symbols displayed at a time */

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x) 
#endif

typedef struct          /* RTN_DESC - routine descriptor */
    {
    FUNCPTR     routine;        /* user routine passed to symEach() */
    int         routineArg;     /* user routine arg passed to symEach() */
    } RTN_DESC;

char * (*cplusDemangleFunc) (char *, char *, int) = 0;

extern int ffsMsb (unsigned int i);

/*******************************************************************************
*
* handleInit - initialize a handle
*
* This routine initializes a handle.
*
* RETURNS: OK, or ERROR if handle could not be initialized.
*
*/

int handleInit
    (
    HANDLE_ID       handleId,   /* pointer to handle to initialize */
    enum handleType type        /* type of handle to initialize    */
    )
    {
    handleId->magic       = (unsigned long) handleId;
    handleId->type    = type;
    handleId->context     = NULL;
    handleId->contextType = handleContextTypeNone;
    handleId->safeCnt     = 0;

    return (OK);
    }
/*******************************************************************************
*
* symKeyCmpName - compare two symbols' names 
*
* This routine returns TRUE if the match symbol's type masked by the specified
* symbol mask, is equivalent to the second symbol's type also masked by the
* symbol mask, and if the symbol's names agree.
*
* If the <symTypeMask> is SYM_MASK_EXACT, the API checks that the pointers
* match exactly instead. 
*
* RETURNS: TRUE if symbols match, FALSE if they differ.
*/

LOCAL int symKeyCmpName
    (
    SYMBOL *        pMatchSymbol,   /* pointer to match criteria symbol */
    SYMBOL *        pSymbol,    /* pointer to symbol */
    unsigned int    symTypeMask /* symbol type bits that matter */
    )
    {
    SYM_TYPE    mask;                   /* symbol type bits that matter */

    /*
     * If symTypeMask is equal to SYM_MASK_EXACT, then check to see if the
     * pointers match exactly.
     */

    if (symTypeMask == SYM_MASK_EXACT)
        return (pMatchSymbol == pSymbol ? TRUE : FALSE);

    mask = (SYM_TYPE) symTypeMask;
    return (((pSymbol->type & mask) == (pMatchSymbol->type & mask)) &&
        (strcmp (pMatchSymbol->name, pSymbol->name) == 0));
    }

/*******************************************************************************
*
* symHFuncName - symbol name hash function
*
* This routine applies a checksum algorithm to the name and uses 
* that as the input to a multiplicative hashing function provided 
* by hashFuncMultiply().
*
* RETURNS: An integer between 0 and (elements - 1).
* 
* \NOMANUAL
*/

LOCAL int symHFuncName
    (
    int         elements,       /* no. of elements in hash table */
    SYMBOL      *pSymbol,       /* pointer to symbol */
    int         seed            /* seed to be used as scalar */
    )
    {
    int         hash;
    char    *tkey;
    int         key = 0;

    /* checksum the string and use a multiplicative hashing function */

    for (tkey = pSymbol->name; *tkey != '\0'; tkey++)
    key = key + (unsigned int) *tkey;

    hash = key * seed;              /* multiplicative hash func */

    hash = hash >> (33 - ffsMsb (elements));    /* take only the leading bits */

    return (hash & (elements - 1));     /* mask hash to (0,elements-1)*/
    }

/*******************************************************************************
*
* symTblCreate - create a symbol table
*
* This routine creates and initializes a symbol table with a hash table of a
* specified size.  The size of the hash table is specified as a power of two.
* For example, if <hashSizeLog2> is 6, a 64-entry hash table is created.
*
* If the <sameNameOk> parameter is FALSE, attempting to add a symbol with
* the same name and type as an already-existing symbol in the symbol table
* will result in an error.  This behavior cannot be changed once the
* symbol table has been created.
*
* RETURNS: Symbol table ID, or NULL if sufficient memory is not available or
* another fatal error occurred.
*/

SYMTAB_ID symTblCreate
    (
    int    hashSizeLog2,       /* size of hash table as a power of 2 */
    int    sameNameOk         /* allow 2 symbols of same name & type */
    )
    {
    SYMTAB_ID symTblId = (SYMTAB_ID) kmalloc (sizeof (SYMTAB));

    /* kmalloc sets the errno, if any */

    printf ("\nDEBUG: symTblCreate creating symbol table. \n");  
    printf ("\nDEBUG: symTblId = 0x%x. \n", symTblId);

    if (symTblId != NULL)
    {
    symTblId->nameHashId = hashTblCreate (hashSizeLog2,
                          symKeyCmpName,
                          symHFuncName,
                          SYM_HFUNC_SEED);

    if (symTblId->nameHashId == NULL)   /* hashTblCreate failed? */
        {
        kfree ((void *) symTblId);
        printf ("\nDEBUG: symTblCreate failed to create hash "
                  "table. \n");  
        return NULL;
        }

    symTblId->sameNameOk = sameNameOk;       /* name clash policy */
    symTblId->nsymbols   = 0;                /* initial number of syms */

    /* initialize handle */

    handleInit (&symTblId->handle, handleTypeSymTbl);
    }

    printf ("\nDEBUG: symTblCreate succeeded id: 0x%x. \n", symTblId);  

    return symTblId;
    }

/******************************************************************************
*
* symTblIdVerify - verify that a symTblId is valid
*
* Created this function to avoid code duplication (null pointer check
* followed by HANDLE_VERIFY) and to enforce checking that the pointer
* to the structure containing the handle is non-null before calling
* the HANDLE_VERIFY.  All calls to HANDLE_VERIFY and concerning a
* symbol table id (at least within this compilation unit) should be
* contained within this function, unless there is a need for different
* behavior, like not setting the errno.
*
* ERRNO:
* Possible errnos set by this routine include:
* \ml
* \m + 
* S_symLib_INVALID_SYM_ID
* \me
*
* For a complete description of the errnos, see the reference documentation
* for symLib.
*
* RETURNS: OK or ERROR if the symTbl id is not valid.
*
* \NOMANUAL 
*/

LOCAL int symTblIdVerify
    (
    SYMTAB_ID   symTblId    /* ID of module to verify */
    )
    {
    /* 
     * Make sure symTblId is a valid symTbl ID.   Also make sure we're
     * not getting a NULL pointer, HANDLE_VERIFY doesn't check for
     * that.  
     */

    if ((symTblId == NULL) ||
        (HANDLE_VERIFY (&symTblId->handle, handleTypeSymTbl) != OK))
    {
    printf ("\nDEBUG: symTblIdVerify not OK. \n");
    return ERROR;               /* invalid symbol table ID */
    }

    return OK;
    }

/*******************************************************************************
*
* symTblAdd - add a symbol to a symbol table
*
* This routine adds a symbol to a symbol table.
*
* ERRNO:
* Possible errnos set by this routine include:
* \ml
* \m + 
* S_symLib_NAME_CLASH
* \me
*
* For a complete description of the errnos, see the reference documentation
* for symLib.
*
* RETURNS: OK, or ERROR if invalid symbol table, or symbol couldn't be added.
*
* INTERNAL: symTblSymAdd would probably be a more appropriate name.
* IMP: the point of this routine is not obvious from either its name
* or its documentation.  Improve one or both of those so you can tell
* the difference between symAdd and symTblAdd at a glance.  
* 
* \NOMANUAL
*/

int symTblAdd
    (
    SYMTAB_ID   symTblId,   /* symbol table to add symbol to */
    SYMBOL *    pSymbol     /* pointer to symbol to add */
    )
    {
#if 0
    if (symTblIdVerify (symTblId) != OK)
        return ERROR;                  /* symTblIdVerify sets the errno */
#endif

    if ((!symTblId->sameNameOk) &&
    (hashTblFind (symTblId->nameHashId, &pSymbol->nameHNode,
              (int) SYM_MASK_ALL) != NULL))
        return ERROR;
    
    if (hashTblPut (symTblId->nameHashId, &pSymbol->nameHNode) != OK)
        return ERROR; /* hashTblPut() sets the errno, if any */

    symTblId->nsymbols ++;          /* increment symbol count */

    return OK;
    }

#define SYM_ADD_ERROR_COUNT 10

/*******************************************************************************
*
* symNameGet - get name of a symbol
* 
* This routine is currently intended only for internal use. 
*
* It provides the name of the symbol specified by the SYMBOL_ID
* <symbolId>.  The SYMBOL_ID of a symbol may be obtained by using the
* routine symFindSymbol().  A pointer to the symbol table's copy of the
* symbol name is returned in <pName>.
*
* RETURNS: OK, or ERROR if either <pName> or <symbolId> is NULL.
*
* \NOMANUAL 
*/

int symNameGet
    (
    SYMBOL_ID  symbolId,    /* symbol table ID */
    char **    pName        /* pointer to symbol name */
    )
    {
    if ((symbolId == NULL) || (pName == NULL))
    return ERROR;

    *pName = symbolId->name;

    return OK;
    }

/******************************************************************************
*
* usrStandaloneInit - initialize the built-in symbol table
*/ 

void usrStandaloneInit (void)
    {
    unsigned long       ix;
    unsigned int        errCnt = 0;
    char *      symName;
    static char     symNameErr [] = "symNameGet() error!";

    if (sysSymTbl == NULL)
        return;

    printf ("\nAdding %d symbols for standalone @ 0x%x\n", standTblSize, standTbl);

    /* Fill system symbol table from the builtin one */

    for (ix = 0; ix < standTblSize; ix++)
        {
        if (symTblAdd (sysSymTbl, &standTbl[ix]) != OK)
            {
            if (symNameGet (&standTbl[ix], &symName) != OK)
                symName = symNameErr;

            if (errCnt < SYM_ADD_ERROR_COUNT)
                printf ("Error adding '%s' to the standalone symbol table "
                  "(idx = %ld)!\n", symName, ix);

            else if (errCnt == SYM_ADD_ERROR_COUNT)
                printf ("...\n");

            errCnt++;
            }
        }

    /* 
     * It is normal for the number of reported registration errors to not match 
     * standTblSize if STORE_ABS_SYMBOLS == FALSE.
     */

    if (errCnt != 0)
    printf ("%ld symbols could not be registered.\n", errCnt);
    }

/*******************************************************************************
*
* symEachRtn - call a user routine for a hashed symbol
*
* This routine supports hashTblEach(), by unpackaging the routine descriptor
* and calling the user routine specified to symEach() with the right calling
* sequence.
*
* RETURNS: Boolean result of user specified symEach routine.
*
* NOMANUAL
*/

LOCAL int symEachRtn
    (
    SYMBOL      *pSymbol,       /* ptr to symbol */
    RTN_DESC    *pRtnDesc       /* ptr to a routine descriptor */
    )
    {
    return ((* pRtnDesc->routine) (pSymbol->name, (int) pSymbol->value,
                                   pSymbol->type, pRtnDesc->routineArg,
                                   pSymbol->group, pSymbol));
    }

/*******************************************************************************
*
* symEach - call a routine to examine each entry in a symbol table
*
* This routine calls a user-supplied routine to examine each entry in the
* symbol table; it calls the specified routine once for each entry.  The
* routine should be declared as follows:
* .CS
*     int routine
*         (
*         char      *name,  /@ entry name                  @/
*         int       val,    /@ value associated with entry @/
*         SYM_TYPE  type,   /@ entry type                  @/
*         int       arg,    /@ arbitrary user-supplied arg @/
*         unsigned short    group   /@ group number                @/
*         )
* .CE
* The user-supplied routine should return TRUE if symEach() is to continue
* calling it for each entry, or FALSE if it is done and symEach() can exit.
*
* RETURNS: A pointer to the last symbol reached,
* or NULL if all symbols are reached.
*
* INTERNAL
* In addition to the parameters given, it also passes a pointer to a symbol
* as the last arguement.
*
*/

SYMBOL *symEach
    (
    SYMTAB_ID   symTblId,       /* pointer to symbol table */
    FUNCPTR     routine,        /* func to call for each tbl entry */
    int         routineArg      /* arbitrary user-supplied arg */
    )
    {
    SYMBOL   *pSymbol;
    RTN_DESC rtnDesc;

    /* fill in a routine descriptor with the routine and argument to call */

    rtnDesc.routine    = routine;
    rtnDesc.routineArg = routineArg;

    pSymbol = (SYMBOL *) hashTblEach (symTblId->nameHashId, symEachRtn,
                                      (int) &rtnDesc);

    return (pSymbol);                           /* symbol we stopped on */
    }


/*******************************************************************************
*
* strMatch - find an occurrence of a string in another string
*
* This is a simple pattern matcher used by symPrint().  It looks for an
* occurence of <str2> in <str1> and returns a pointer to that occurrence in
* <str1>.  If it doesn't find one, it returns 0.
*/

LOCAL char *strMatch
    (
    FAST char *str1,            /* where to look for match */
    FAST char *str2             /* string to find a match for */
    )
    {
    FAST int str2Length = strlen (str2);
    FAST int ntries     = strlen (str1) - str2Length;

    for (; ntries >= 0; str1++, --ntries)
        {
        if (strncmp (str1, str2, str2Length) == 0)
            return (str1);      /* we've found a match */
        }

    return (NULL);
    }


/*******************************************************************************
*
* symbolStartOf - skip leading underscore
*
* RETURNS: pointer to the start of the symbol name after any compiler 
*          prepended leading underscore.
*        
*
* NOMANUAL
*/

LOCAL char *symbolStartOf (
                          char *str
                          )
{
    if (FALSE && (str [0] == '_'))
        return str + 1;
    else
        return str;
}


/******************************************************************************
*
* cplusDemangle - demangle symbol
*
* This function takes a symbol (source), removes any compiler prepended 
* underscore, and then attempts to demangle it. If demangling succeeds
* the (first n chars of) the demangled string are placed in dest [] and
* a pointer to dest is returned. Otherwise a pointer to the start of
* the symbol proper (after any compiler prepended underscore) is returned.

* RETURNS:
* A pointer to a human readable version of source.
*
* NOMANUAL
*/

char * cplusDemangle
    (
    char        * source,               /* mangled name */
    char        * dest,                 /* buffer for demangled copy */
    int           n             /* maximum length of copy */
    )
    {
    source = symbolStartOf (source);
    if (cplusDemangleFunc == 0  )
        {
        return source;
        }
    else
        {
        return cplusDemangleFunc (source, dest, n);
        }
    }


/*******************************************************************************
*
* symPrint - support routine for symShow()
*
* This routine is called by symEach() to deal with each symbol in the table.
* If the symbol's name contains <substr>, this routine prints the symbol.
* Otherwise, it doesn't.  If <substr> is NULL, every symbol is printed.
*/

LOCAL int symPrint
    (
    char *      name,
    int         val,
    char        type,
    char *      substr
    )
    {
    char        demangled [MAX_SYS_SYM_LEN + 1];
    char *      nameToPrint;

    if (substr == NULL || strMatch (name, substr) != NULL)
        {
        nameToPrint = cplusDemangle (name, demangled, MAX_SYS_SYM_LEN + 1);
        printf ("%s 0x%x\n", nameToPrint, val);
        }

    return (TRUE);
    }

/******************************************************************************
*
* symTypeNameGet - get symbol type name 
*
* This routine returns a string containing the symbol type name.
*
* RETURNS: a string containing symbol type name
*
*/

LOCAL const char * symTypeNameGet
    (
    unsigned int      type    /* symbol type */
    )
    {
    static const char * absStr = "abs";         /* string for abs. symbols */
    static const char * textStr = "text";       /* string for text symbols */
    static const char * dataStr = "data";       /* string for data symbols */
    static const char * bssStr = "bss";         /* string for bss symbols */
    static const char * commStr = "comm";       /* string for common symbols */
    static const char * unknownStr = "???";     /* string for unknown symbols */

    /* mask to keep only type we want to manage */

    type &= (SYM_COMM|SYM_BSS|SYM_DATA|SYM_TEXT|SYM_ABS);

    /* Note that common symbol type can be ORed */

    if ((type & SYM_COMM) == SYM_COMM)
        return (commStr);
    else if (type == SYM_BSS)
        return (bssStr);
    else if (type == SYM_DATA)
        return (dataStr);
    else if (type == SYM_TEXT)
        return (textStr);
    else if (type == SYM_ABS)
        return (absStr);
    else
        return (unknownStr);
    }

 /*******************************************************************************
*
* symSysTblPrint - support routine for symShow()
*
* This routine is called by symEach() to deal with each symbol in the 
* system symbol table.  If the symbol's name contains <substr>, this routine
* prints the symbol.  Otherwise, it doesn't.  If <substr> is NULL, every
* symbol is printed.  The type is printed along with the symbol value.
*/

LOCAL int symSysTblPrint
    (
    char *      name,
    int         val,
    char        type,
    char *      substr,
    unsigned short      group
    )
    {
    char        demangled [MAX_SYS_SYM_LEN + 1];
    char *      nameToPrint;

    if (substr == NULL || strMatch (name, substr) != NULL)
        {
        nameToPrint = cplusDemangle (name, demangled, MAX_SYS_SYM_LEN + 1);
        printf ("%s 0x%x %s ", nameToPrint, val,
                symTypeNameGet (type));

        if ((type & N_EXT) == 0)
            printf (" (local)");

        printf ("\n");
        }


    return (TRUE);
    }

/*******************************************************************************
*
* symShow - show the symbols of specified symbol table with matching substring
*
* This routine lists all symbols in the specified symbol table whose names
* contain the string <substr>.  If <substr> is is an empty * string (""), all
* symbols in the table will be listed.  If <substr> is NULL then the symbol
* table structure will be summarized.
*
* RETURNS: OK, or ERROR if invalid symbol table id.
*
* SEE ALSO: symLib, symEach()
*/

int symShow
    (
    SYMTAB *    pSymTbl,        /* pointer to symbol table to summarize */
    char *      substr          /* substring to match */
    )
    {
    if (substr == NULL)
        {
        printf ("%s: %d\n", "Number of Symbols", pSymTbl->nsymbols);
        printf ("%s: 0x%x\n", "Symbol Hash Id",
                                                (int) pSymTbl->nameHashId);
        printf ("%s: %s\n", "Name Clash Policy", 
                (pSymTbl->sameNameOk) ? "Allowed" : "Disallowed");
        }
    else
        {
        if (pSymTbl == sysSymTbl)
            {
            symEach (pSymTbl, (FUNCPTR) symSysTblPrint, (int) substr);
            symCount = 0;
            }
        else
            symEach (pSymTbl, (FUNCPTR) symPrint, (int) substr);
        }

    return (OK);
    }

/******************************************************************************
*
* sym_table_init - initialize the system symbol table
*
* This routine initializes the system symbol table <sysSymTbl>.
*
* RETURNS: N/A
*
*/ 

void sym_table_init(void)
{
    /* create system and status symbol tables */

    sysSymTbl = symTblCreate (SYM_TBL_HASH_SIZE_LOG2, TRUE);
    printf("symTblCreate sysSymTbl @ 0x%x done!\n",sysSymTbl);

    if (sysSymTbl == NULL)
        printf ("sym_table_init: error creating the system symbol table.\n");

    usrStandaloneInit();

//    symShow(sysSymTbl, "symShow");
}


/*******************************************************************************
*
* symFindSymbol - find symbol with matching name and type in a symbol table 
*
* This routine is a generic routine for retrieving a symbol from the
* specified symbol table.  It can be used to perform searches by name,
* by name and type, by value, or by value and type.  
* 
* If the 'name' parameter is non-NULL, a search by name will be
* performed and the value parameter will be ignored.  If the name
* parameter is NULL, a search by value will be performed.  
* 
* In a search by value, if no matching entry is found, the symbol 
* with the next lowest value, among symbols whose types match the specified 
* type, is selected.  (If the type is NULL, as in a simple search by value, 
* the symbol returned will simply be whatever symbol in the symbol table 
* has the next lowest address.)
*
* In both the search by name and search by value cases, potential
* matches will have their types compared to <type>, subject to the
* <mask>, and only symbols which match the specified type will be
* returned.  
*
* To have the type matched exactly, set the <type> parameter to the desired 
* type and set the <mask> parameter to SYM_MASK_EXACT_TYPE.  To search only 
* by name or only by value and have the <type> parameter ignored, set 
* the <mask> parameter to SYM_MATCH_ANY_TYPE.
*
* \IFSET_START KERNEL
* To search the VxWorks (kernel) symbol table, specify \f3sysSymTbl\f1
* as the <symTblId> parameter.
* \IFSET_END KERNEL
*
* INTERNAL
* This routine contains a weird hack designed to deal with some additional
* symbols the loader and the linker put in the symbol table.  
* 
* The loader adds three symbols
* to the symbol table: <filename>_text, <filename>_data, <filename>_bss.
* 
* These symbols may have the same address (i.e. value in the symbol table)
* as real routine or variable names.  When looking up a symbol by value 
* for display it is desirable to find the real routine or variable names 
* in preference to these made up names.  For example, loading "demo.o" 
* will cause a symbol "demo.o_text" to be added to the symbol table with 
* the same value as the real symbol "_demo".  In a disassembly or "i" 
* printout, etc, we would rather see "_demo".  So the test inside the loop 
* in this routine, which normally terminates the search if we find an exact 
* match, has been changed to keep searching if the match it finds ends 
* in "_text", "_data", or "_bss".  
* 
* IMP: If no other exact match is found, the special symbols will be returned
* anyway. Thus this routine simply has a "bias" against such symbols, but
* will not return an erroneous result in any event. This nonsense is to be
* removed when the loader does not add these symbols anymore.
*
* The GNU toolkit adds symbols of the form "gcc2_compiled." and "xxx.o".
* These symbols are also biased against.
*
* ERRNO:
* Possible errnos set by this routine include:
* \ml
* \m +
* S_symLib_INVALID_SYMTAB_ID
* \m + 
* S_symLib_INVALID_SYM_ID_PTR
* \m + 
* S_symLib_SYMBOL_NOT_FOUND
* \me
*
* For a complete description of the errnos, see the reference documentation
* for symLib. 
*
* RETURNS: OK, or ERROR if <symTblId> is invalid, <pSymbolId> is NULL,
*          symbol not found (for a search by name), <value> is less than
*          the lowest value in the table (for search by value) or an error
*          occurred.
*
* \NOMANUAL 
*/

int symFindSymbol
    (
    SYMTAB_ID   symTblId,       /* symbol table ID */
    char *      name,           /* name to search for */
    SYM_VALUE   value,      /* value of symbol to search for */
    SYM_TYPE    type,           /* symbol type */
    SYM_TYPE    mask,           /* type bits that matter */
    SYMBOL_ID * pSymbolId       /* where to return id of matching symbol */
    )
    {
    HASH_NODE *         pNode;      /* node in symbol hash table */
    SYMBOL              keySymbol;  /* dummy symbol for search by name */
    int                 index;      /* counter for search by value */
    SYMBOL *            pSymbol;    /* current symbol, search by value */
    SYMBOL *            pBestSymbol = NULL; 
                                   /* symbol with lower value, matching type */
    char *      pUnder;    /* string for _text, etc., check */
    SYM_VALUE       bestValue = NULL; 
                                    /* current value of symbol with matching 
                       type */ 

    if (symTblIdVerify (symTblId) != OK)
    return ERROR;   /* symTblIdVerify() sets the errno, if any */

    if (pSymbolId == NULL) 
        {
    return ERROR; 
    }

    if (name != NULL) 
        {
    /* Search by name or by name and type: */
      
    /* fill in keySymbol */

    keySymbol.name = name;          /* match this name */
    keySymbol.type = type;          /* match this type */

    pNode = hashTblFind (symTblId->nameHashId, &keySymbol.nameHNode, 
                 (unsigned int) mask);

    if (pNode == NULL)
        {
        return ERROR;
        }

    *pSymbolId = (SYMBOL_ID) pNode;
    }
     else 
        {
    /* Search by value or by value and type: */

    for (index = 0; index < symTblId->nameHashId->elements; index++)
        {
        pSymbol = 
            (SYMBOL *) SLL_FIRST(&symTblId->nameHashId->pHashTbl [index]);

        while (pSymbol != NULL)         /* list empty */
            {
        if (((pSymbol->type & mask) == (type & mask)) &&
            (pSymbol->value == value) &&
            (((pUnder = strrchr (pSymbol->name, '_')) == NULL) ||
             ((strcmp (pUnder, "_text") != 0) &&
              (strcmp (pUnder, "_data") != 0) &&
              (strcmp (pUnder, "_bss") != 0) &&
              (strcmp (pUnder, "_compiled.") != 0))) &&
            (((pUnder = strrchr (pSymbol->name, '.')) == NULL) ||
             ((strcmp (pUnder, ".o") != 0))) &&
            (!(pSymbol->type & SYM_ABS)))
            {
            /* We've found the entry.  Return it. */

            *pSymbolId = pSymbol;
            
            return OK;
            }

        else if (((pSymbol->type & mask) == (type & mask)) &&
             ((pSymbol->value <= value) &&
              (pSymbol->value > bestValue)) &&
             (!(pSymbol->type & SYM_ABS)))
            {
             
            /* 
             * This symbol is of correct type and closer than the last
             * one 
             */

            bestValue   = pSymbol->value;
            pBestSymbol = pSymbol;
            }

        pSymbol = (SYMBOL *) SLL_NEXT (&pSymbol->nameHNode);
        }
        }

    if (bestValue == NULL || pBestSymbol == NULL) /* any closer symbol? */
        {
        return ERROR;
        }

    *pSymbolId = pBestSymbol;
    }

    return OK;
    }

/*******************************************************************************
*
* symFind - find a symbol matching given search criteria
*
* This routine searches symbol table <symTblId> for a symbol matching the search
* criteria described by the SYMBOL_DESC structure that is pointed to by
* <pSymDesc>. The same structure is used for the output of the symFind()
* routine if information about the symbol is found.
*
* Before updating the structure with the search criteria, clear the SYMBOL_DESC
* structure using <memset (pSymDesc, 0, sizeof (SYMBOL_DESC))>. The entire
* structure must be cleared before it is updated with search criteria to make
* sure that each field is by default set to NULL (This will ensure functional
* backward-compatibility if new fields are added to the structure in a future
* release).
*
* EXPAND ../../h/symLib.h SYMBOL_DESC
*
* The type <mask> field describes the search criteria. The following
* enum lists the possible mask values. Values can be OR'ed, however at
* least one of SYM_FIND_BY_NAME or SYM_FIND_BY_VALUE must be set.
*
* EXPAND ../../h/symLib.h SYM_FIND_MASK
*
* SYM_FIND_BY_VALUE specifies searching a symbol table for a symbol whose
* value matches the specified <value>. If there is no matching entry, the table
* entry with the next lowest value is selected.
* If the <name> field of SYMBOL_DESC is NULL, then symFind() allocates a buffer
* and copies the symbol name to that buffer. This buffer must be freed by the
* caller.
* To avoid memory allocation, a buffer can be passed to symFind() using <name>
* and <nameLen> fields of SYMBOL_DESC. When <name> field is different from
* NULL, symFind() copies the symbol name to the buffer specified by <name>
* without exceeding <nameLen> (including EOS character). If the buffer is too
* small to store the symbol name (with the terminating EOS), then the returned
* symbol name is truncated. Since the symbol length limit is defined by
* MAX_SYS_SYM_LEN, using a buffer which size is (MAX_SYS_SYM_LEN + 1) ensures
* that symbols are not truncated.
* In any cases, the actual value, type, and length of the entire symbol name
* with EOS are copied to <value>, <type> and <nameLen> fields.
*
* SYM_FIND_BY_NAME specifies searching a symbol table for a symbol matching
* the <name>. If a symbol is found, its value and type are copied to <value>
* and <type> fields.
* If multiple symbols have the same name, the routine returns the matching
* symbol most recently added to the symbol table.
*
* The SYM_FIND_BY_NAME or SYM_FIND_BY_VALUE can be OR'ed with SYM_FIND_BY_TYPE
* search critiera. In this case, the symbol must also match the specified
* type (<type>). The <typeMask> parameter can be used to match sub-classes
* of the type (including architecture specific types such as SYM_THUMB).
* Using SYM_MASK_ALL as <typeMask> parameter implies that the symbol type
* specified using the <type> parameter must match exactly the type of symbol
* including architecture-specific types such as SYM_THUMB.
*
* \IFSET_START KERNEL
* To search the global VxWorks symbol table, specify \f3sysSymTbl\f1
* as the <symTblId> parameter. 
* \IFSET_END KERNEL
*
* EXAMPLE
* The following example searches given symbol in kernel system symbol table:
*
* \ss
* #include \<vxWorks.h\>
* #include \<symLib.h\>
* #include \<string.h\>
* #include \<sysSymTbl.h\>
* #include \<stdio.h\>
* #include \<errnoLib.h\>
* 
* void symFindExample
*     (
*     char * symbol               /@ symbol to search for @/
*     )
*     {
*     SYMBOL_DESC symbolDesc;     /@ symFind() descriptor @/
* 
*     memset (&symbolDesc, 0, sizeof (SYMBOL_DESC));
*     symbolDesc.mask = SYM_FIND_BY_NAME;
*     symbolDesc.name = symbol;
* 
*     if (symFind (sysSymTbl, &symbolDesc) == OK)
*         {
*         printf ("Symbol name : %s\n", symbolDesc.name);
*         printf ("Symbol value: %#x\n", symbolDesc.value);
*         printf ("Symbol type : %#x\n", symbolDesc.type);
*         }
*     else
*         printf ("Error: Symbol not found (errno = %#x)\n", errnoGet ());
*     }
* \se
*
* ERRNO:
* Possible errnos set by this routine include:
* \ml
* \m + 
* S_symLib_INVALID_SYMTAB_ID
* \m +
* S_symLib_SYMBOL_NOT_FOUND
* \m +
* S_symLib_INVALID_ARGS
* \me
*
* For a complete description of the errnos, see the reference documentation
* for symLib.
*
* RETURNS: OK, or ERROR if the symbol table ID is invalid or the symbol
* cannot be found.
*/

int symFind
    (
    SYMTAB_ID       symTblId,   /* ID of symbol table to look in */
    SYMBOL_DESC *   pSymDesc    /* pointer to symbol descriptor */
    )
    {
    SYMBOL_ID   symbolId;       /* internal symbol description */
    char *  name        = NULL;         /* default name */
    SYM_VALUE   value       = NULL;         /* default value */
    SYM_TYPE    type        = SYM_MASK_ANY_TYPE;    /* default type */
    SYM_TYPE    typeMask    = SYM_MASK_ANY_TYPE;    /* default mask */

    if ((pSymDesc->mask & (SYM_FIND_BY_NAME|SYM_FIND_BY_VALUE)) == 0)
        {
        /*
         * Given SYM_FIND_MASK combination is not valid:
         * symFind() does not support symbol search by type only.
         */
        
        /*
         * Set name field to NULL, so if customer free that field even
         * if symFind() returns ERROR, it will have no impact.
         */

        pSymDesc->name  = NULL;

        return (ERROR);
        }

    /* Use given name if specified */

    if ((pSymDesc->mask & SYM_FIND_BY_NAME) == SYM_FIND_BY_NAME)
        name = pSymDesc->name;

    /* Use given value if specified */

    if ((pSymDesc->mask & SYM_FIND_BY_VALUE) == SYM_FIND_BY_VALUE)
        value  = pSymDesc->value;

    /* Use given type and mask if specified */

    if ((pSymDesc->mask & SYM_FIND_BY_TYPE) == SYM_FIND_BY_TYPE)
        {
        type    = pSymDesc->type;
        typeMask= pSymDesc->typeMask;
        }

    /* Invoke internal symbol find routine */

    if (symFindSymbol (symTblId, name, value, type, typeMask,
               &symbolId) == ERROR)
        return (ERROR);

    /* Fill symbol descriptor with information on the found symbol */

    pSymDesc->value = symbolId->value;
    pSymDesc->type  = symbolId->type;

    if (((pSymDesc->mask & SYM_FIND_BY_VALUE) == SYM_FIND_BY_VALUE) &&
        ((pSymDesc->mask & SYM_FIND_BY_NAME) == 0))
        {
        unsigned int nameLen;       /* symbol name length */

        /* Was a buffer provided to store the symbol name?  */

        if (pSymDesc->name == NULL)
            {
            /* No buffer provided: Must allocate one */

        nameLen = strlen (symbolId->name) + 1;

        /* Allocate room to store symbol name */

        if ((pSymDesc->name = (char *) kmalloc (nameLen)) == NULL)
            return (ERROR);
            }
        else
            {
            /*
             * A buffer has been provided by symFind() caller. Verify that the
             * specified buffer length is coherent.
             */

            if (pSymDesc->nameLen == 0)
                return (ERROR);

            nameLen = pSymDesc->nameLen;
            }

        /*
         * Copy name to allocated or given buffer.
         * NOTE: If a buffer is specified by symFind() caller but is too small
         * to store the symbol name, then the symbol name can be truncated.
         */

        strncpy (pSymDesc->name, symbolId->name, nameLen);
   
        pSymDesc->name[nameLen - 1] = EOS;

        /* Return length of the entire symbol including EOS */

        pSymDesc->nameLen = nameLen;
        }

    return (OK);
    }


void symFind2
    (
    char * symbol
    )
    {
    SYMBOL_DESC symbolDesc;

    memset (&symbolDesc, 0, sizeof (SYMBOL_DESC));
    symbolDesc.mask = SYM_FIND_BY_NAME;
    symbolDesc.name = symbol;

    if (symFind (sysSymTbl, &symbolDesc) == OK)
        {
        printf ("Symbol name : %s\n", symbolDesc.name);
        printf ("Symbol value: %#x\n", symbolDesc.value);
        printf ("Symbol type : %#x\n", symbolDesc.type);
        }
    else
        printf ("Error: Symbol not found\n");
    }
