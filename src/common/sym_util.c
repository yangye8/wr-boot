#include <wrboot.h>
#include <hashLib.h>
#include <symbol.h>
#include <symLib.h>
#include <sllLib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_SYS_SYM_LEN         256        /* system symbols will not exceed this limit */
#define N_EXT                   1          /* External symbol (OR'd in with one of above)  */

extern SYMTAB_ID       sysSymTbl;                 /* system symbol table id */

typedef struct          /* RTN_DESC - routine descriptor */
    {
    FUNCPTR     routine;        /* user routine passed to symEach() */
    int         routineArg;     /* user routine arg passed to symEach() */
    } RTN_DESC;

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
    char *      nameToPrint = name;

    if (substr == NULL || strMatch (name, substr) != NULL)
        {
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
    char *      nameToPrint = name;

    if (substr == NULL || strMatch (name, substr) != NULL)
        {
        printf ("%-25s 0x%08x %-8s ", nameToPrint, val,
                symTypeNameGet (type));

#define SYM_LOCAL      0x40     /* local */
        if (type & SYM_LOCAL)
            (void) printf (" (local)");

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
        printf ("%s: 0x%x\n", "Symbol Hash Id",(int) pSymTbl->nameHashId);
        printf ("%s: %s\n", "Name Clash Policy", 
                (pSymTbl->sameNameOk) ? "Allowed" : "Disallowed");
        }
    else
        {
        if (pSymTbl == sysSymTbl)
            symEach (pSymTbl, (FUNCPTR) symSysTblPrint, (int) substr);
        else
            symEach (pSymTbl, (FUNCPTR) symPrint, (int) substr);
        }

    return (OK);
    }

void symFindExample
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
