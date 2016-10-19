#include <wrboot.h>
#include <hashLib.h>
#include <symbol.h>
#include <symLib.h>
#include <sllLib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SYM_TBL_HASH_SIZE_LOG2  8          /* 256 entry hash table symbol table */
#define SYM_HFUNC_SEED          1370364821 /* magic seed */

extern SYMBOL       standTbl[];            /* standalone symbol table array */
extern unsigned int standTblSize;          /* symbols in standalone table */
extern int ffsMsb (unsigned int i);

SYMTAB_ID       sysSymTbl;                 /* system symbol table id */

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
    int    sameNameOk          /* allow 2 symbols of same name & type */
    )
    {
    SYMTAB_ID symTblId = (SYMTAB_ID) kmalloc (sizeof (SYMTAB));

    /* kmalloc sets the errno, if any */

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

    }

    return symTblId;
    }

/*******************************************************************************
*
* symTblAdd - add a symbol to a symbol table
*
* This routine adds a symbol to a symbol table.
*/

int symTblAdd
    (
    SYMTAB_ID   symTblId,   /* symbol table to add symbol to */
    SYMBOL *    pSymbol     /* pointer to symbol to add */
    )
    {
    if (symTblId == NULL)
        return ERROR;

    if ((!symTblId->sameNameOk) &&
    (hashTblFind (symTblId->nameHashId, &pSymbol->nameHNode,
              (int) SYM_MASK_ALL) != NULL))
        return ERROR;

    if (hashTblPut (symTblId->nameHashId, &pSymbol->nameHNode) != OK)
        return ERROR; /* hashTblPut() sets the errno, if any */

    symTblId->nsymbols++;          /* increment symbol count */

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
    unsigned long   ix;
    unsigned int    errCnt = 0;
    char *          symName;
    static char     symNameErr [] = "symNameGet() error!";

    if (sysSymTbl == NULL)
        return;

    printf ("\nAdding Standalone symbol table[0x%x-%d] into sysSymTbl (0x%x)\n",standTbl,standTblSize,sysSymTbl);

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

    if (errCnt != 0)
    printf ("%ld symbols could not be registered.\n", errCnt);
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

    printf ("DEBUG: symTblCreate creating symbol table @0x%x. \n",sysSymTbl);  

    if (sysSymTbl == NULL)
        printf ("sym_table_init: error creating the system symbol table.\n");

    usrStandaloneInit();

    symShow(sysSymTbl, "boot");
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
* For a complete description of the errnos, see the reference documentation
* for symLib. 
*
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

    if (symTblId == NULL)
    return ERROR;

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
