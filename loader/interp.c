/* Includes */

#include <wrboot.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <symbol.h>
#include <symLib.h>

/* Defines */

#define SYM_BASE_MASK   0x2e

/* Structures */

typedef struct {
    const char * name;       /* command name */
    FUNCPTR      command;    /* command C function */
    } DEMO_CMD;

/* Externals */

SYMTAB_ID       sysSymTbl;              /* system symbol table id */

/* Globals */

/* Forward definitions */

static int   dumpCommandFunc (int argc, char ** argv);
static int   symCommandFunc (int argc, char ** argv);

static int  statementSplit (const char * statement, char ** pCommand,
                char ** pArgument);
static char *   tokenize (char * string, char ** ppString);
int  commandExec (const char * name, const char * args);
static int  symbolGet (const char * symbolName, int ** ppSymbolValue,
               SYM_TYPE * pSymTypeOut);
static int  strToInt32 (const char * valueStr, int * pInt,
                const char * errorStr);

static DEMO_CMD *       commandGet (const char * name);

/* Locals */

static DEMO_CMD demoCmd[] = {
    {"dump", dumpCommandFunc},
    {"sym", symCommandFunc},
    {NULL, NULL}
    };

extern int symFind (SYMTAB_ID symTblId, SYMBOL_DESC *pSymDesc);
extern int sscanf(const char *ibuf, const char *fmt, ...);

/*******************************************************************************
*
* dumpCommandFunc - function for the `dump' command
*
* This function corresponds to the `dump' command. This command display the 
* content of the memory starting at the address <address>, for <num> 16
* bits integer.
*
* If <num> is not specified, the previous value is used instead. If <address>
* is not specified, the last address of memory displayed is used instead.
*
* SYNOPSIS
* dump [-n <num>] [address]
*
* PARAMETERS
* -n : specify the number of 16 bits integer to display
*
* RETURNS: OK, or ERROR if an error occured
*
*/

static int dumpCommandFunc
    (
    int         argc,
    char **     argv
    )
    {
    unsigned char * pByte;      /* byte pointer for filling ascii buffer */
    unsigned short *    tmpShort;   /* temporary short word pointer */
    int     addr = 0;
    int     nunits = 0;
    char    ascii[16 + 1];  /* ascii buffer for displaying */
    int     item;       /* item counter displayed per line */
    int     ix;     /* temporary count */
    static int  dNitems = 0x80; /* default number of item to display */

    /* Check options and get address */

    for (ix = 1; ix < argc; ix++)
    {
    if (strcmp (argv[ix], "-n") == 0)
        {
        if (strToInt32 (argv[ix++], &nunits, argv[0]) != OK)
        return ERROR;
        }
    else if (strToInt32 (argv[ix], &addr, argv[0]) != OK)
        return ERROR;
    }

    /* Execute memory dump */

    ascii[16] = EOS;    /* put an EOS on the string */

    if (nunits < 0)
    {
    printf ("%s: the number of units displayed must be positive.\n", 
          argv[0]);
    return ERROR;
    }

    /* Get last number of item to display */

    if (nunits == 0)
        nunits = dNitems;

    /* round address down to word boundary */

    addr = (int) addr & ~(2 - 1);

    /* print leading spaces on first line */

    bfill (ascii, 16, '.');

    printf ("NOTE: memory values are displayed in hexadecimal.\n");

    printf ("0x%08x:  ", addr & ~0xf);

    for (item = 0; item < (addr & 0xf) / 2; item++)
    {
    printf ("     ");
    bfill (&ascii[item * 2], 4, ' ');
    }

    /* print out all the words */

    while (nunits-- > 0)
    {
    if (item == 8)
        {
        /* end of line:
         *   print out ascii format values and address of next line */

        printf ("  *%16s*\n0x%08x:  ", ascii, addr);
        bfill (ascii, 16, '.'); /* clear out ascii buffer */
        item = 0;               /* reset word count */
        }

    tmpShort = (unsigned short *)addr;
    printf ("%04x", *tmpShort);

        printf (" ");   /* space between words */

    /* set ascii buffer */

    pByte = (unsigned char *) addr;
    for (ix = 0; ix < 2; ix ++)
        {
        if (*pByte == ' ' || (isascii (*pByte) && isprint (*pByte)))
        {
            ascii[item*2 + ix] = *pByte;
        }
        pByte ++;
        }

    addr += 2;
    item++;
    }

    /* print remainder of last line */

    for (; item < 8; item++)
    printf ("     ");

    printf ("  *%16s*\n", ascii);   /* print out ascii format values */

    return OK;
    }

/*******************************************************************************
*
* cmd_parse - call the DEMO interpreter parser
*
* This function calls the DEMO interpreter parser.
*
* It breaks the input line <inputLine> into understandable tokens and call
* the corresponding DEMO command with its arguments.
*
* RETURNS: OK or ERROR
*
*/

 int cmd_parse
    (
    const char *    inputLine   /* shell input line */
    )
    {
    const char *    statement;
    char *      command;
    char *      argument;
    char *      argument1;
    char *      argument2;
    int     status = OK;
    unsigned int a [12];
    FUNCPTR pFuncPtr;

    SYM_TYPE    symType;
    int *       pSymVal = NULL;

    /* Strip any heading blank characters */

    memset(a,0,12);

    statement = inputLine;
    while (isspace ((int)statement[0]))
    statement++;

    /* Skip a commented statement */

    if (strlen (statement) == 0)
    return OK;

    /* Split statement into a command name and an argument string */

    if (statementSplit (statement, &command, &argument) != OK)
    return ERROR;

    if (*argument!='\0')
    if (statementSplit (argument, &argument1, &argument2) != OK)
    return ERROR;

    printf("String: arg0%s, arg1-%s\n",argument1, argument2);

    if (*argument1 !='\0')
        a[0] = strtoul(argument1,0,0);

    if (*argument2 !='\0')
        a[1] = strtoul(argument2,0,0);

    /* Execute the command */

    if (symbolGet (command, &pSymVal, &symType) != OK)
        {
        printf ("unknown symbol name '%s'.\n", command);
        }

    printf("\ncommand %s\n",command);
    printf("    args: 0x%x 0x%x\n",a[0],a[1]);

    printf ( \
        "    address = 0x%x\n"              \
        "    value: (byte)      = 0x%x\n"   \
        "           (short)     = 0x%hx\n"  \
        "           (word)      = 0x%x\n"   \
        "           (long word) = 0x%llx\n", \
        (int)pSymVal,*(char *)pSymVal, *(short *)pSymVal, *(int *)pSymVal,
        *(long long *)pSymVal);

 //   status = commandExec (command, argument);

    pFuncPtr = (FUNCPTR)pSymVal;

#if 0
    ((void (*)(void)) pFunc) ();
#else
    (* pFuncPtr) (a[0], a[1], a[2], a[3], a[4], a[5], a[6],
                                a[7], a[8], a[9], a[10], a[11]);
#endif

    /* Check error */

    if (status == ERROR)
    {
    printf ("DEMO: cannot execute \"%s", command);
    if (argument[0] != EOS)
        printf (" %s", argument);
    printf ("\".\n");
    }

    kfree (command);
    kfree (argument);

    return status;
    }

/*******************************************************************************
*
* statementSplit - split a command statement
*
* This routine splits the command statement string <statement> into a command
* string and an argument string. It checks command name against the command
* database.
*
* The command name string pointer and the argument string pointer are
* returned into <pCommand> and <pArgument>. It is up to the caller to
* kfree these buffers using a kfree() call.
*
* RETURNS: OK, or ERROR if error occured.
*/

static int statementSplit
    (
    const char *    statement,
    char **     pCommand,
    char **     pArgument
    )
    {
    char *  cmdName;
    char *  cmdArgs;
    char *  pLast;
    char *  statementCpy;

    statementCpy = strdup (statement);
    if (statementCpy == NULL)
    return ERROR;   /* errno set */

    cmdName = tokenize (statementCpy, &pLast);

printf("cmdName:%s\n",cmdName);

    if (cmdName == NULL)
    {
    /* None command */

    kfree (statementCpy);
    return ERROR;
    }

    /* Check if the command exists */
#if 0
    if (commandGet (cmdName) == NULL)
    {
    /* This is not an available command */

    printf ("DEMO: command '%s' not found.\n", cmdName);
    kfree (statementCpy);
    return ERROR;
    }
#endif

    /* Get the argument string */

    if (pLast != NULL)
    cmdArgs = pLast;
    else
    cmdArgs = "";   /* none arguments */

    /* Return the full command name and the argument string */

    *pCommand = strdup (cmdName);
    *pArgument = strdup (cmdArgs);

printf("*pCommand:%s\n",*pCommand);
printf("*pArgument:%s\n",*pArgument);

    kfree (statementCpy);

    if (*pCommand == NULL || *pArgument == NULL)
    {
    kfree (*pArgument);
    kfree (*pCommand);
    return ERROR;
    }

    return OK;
    }

/*******************************************************************************
*
* tokenize - tokenize a string
*
* This routine splits the string <string> into a sequence of token, each of
* which is delimited by one or more blank character.
*
* The function considers the string <string> to consist of a sequence of 
* zero or more text tokens separated by spans of one or more blank
* characters. The first call (with pointer <string>  specified) returns
* a pointer to the first character of the first token, and will have written a
* null character into <string> immediately following the returned token.
* The function keeps track of its position writting it to the placeholder 
* pointed by <ppString>. Subsequent calls (which  must be made with the first
* argument being a NULL pointer) will work through the string <string>
* immediately following that token. In this way, subsequent calls will work
* through the string <string> until no tokens remain. When no token remains in
* <string>, a null pointer is returned.
*
* RETURNS: a pointer on the next token, or NULL if no more token is available.
*
* ERRNO: N/A
*
* SEE ALSO
*/

static char * tokenize
    (
    char *  string,     /* string to break into tokens */
    char ** ppString    /* pointer to serve as string index */
    )
    {
    char *      pStr;
    char *      pStrToken;
    static const char * tokenSep = " \t\v\r\n\f";

    if (string != NULL)
    pStr = string;
    else
    {
    pStr = *ppString;
    if (pStr == NULL)
        return NULL;    /* no more token */
    }

    /* Remove token separator characters at the begining of the string */

    pStr += strspn (pStr, tokenSep);

    if (*pStr == EOS)
    {
    /* No more token available after that */

    *ppString = NULL;
    return NULL;
    }

    /* Get the end of the token */

    pStrToken = pStr;           /* start of the next token */
    pStr = strpbrk (pStr, tokenSep);    /* end of the token */

    if (pStr != NULL)
    {
    /* Found a token separator character: return the token */

    *pStr = EOS;
    *ppString = ++pStr;
    }
    else
    *ppString = NULL;

    if (strlen (pStrToken) == 0)
    return NULL;

    return pStrToken;
    }

/*******************************************************************************
*
* commandGet - get a command
*
* This routine returns the command entry for the command named <name>. If the
* command cannot be found, NULL is returned. 
*
* RETURNS: pointer on the command node, or NULL if the command cannot be found.
*
* ERRNO: N/A
*
*/

static DEMO_CMD * commandGet
    (
    const char *    name        /* command name looked for */
    )
    {
    int ix;

    /* Search the command name */

    ix = 0;
    while (demoCmd[ix].name != NULL)
    {
    if (strcmp (demoCmd[ix].name, name) == 0)
        break;

    ix++;
    }

    if (demoCmd[ix].name != NULL)
    return &demoCmd[ix];

    return NULL;
    }

/*******************************************************************************
*
* commandExec - execute a command
*
* This routine executes the command <name> with the argument
* string <args>. The argument string <args> is parsed to split it into 
* an array of strings. These strings are separated by blank characters. The
* array and the number of strings is passed to the command function 
* (as argc/argv parameters).
*
* RETURNS: OK, or ERROR if the command cannot be executed.
*
* ERRNO: N/A
*/

int commandExec
    (
    const char *    name,   /* command name to execute */
    const char *    args    /* arguments of the command */
    )
    {
    DEMO_CMD *  pCmd;
    int status;
    char ** argv;
    char *  pTmpArgs;
    char *  pStr;
    char *  pLast;
    int     argc;
    int     sizeArgv;
    int     ix;

    /* Get the shell command information */

    pCmd = commandGet (name);
 
 printf("pCmd %x\n",pCmd);

    if (pCmd == NULL || pCmd->command == NULL)
    return ERROR;

    /* Create the argc/argv parameters. One argument for the command name */

    sizeArgv = strlen (name) + 1 + sizeof (char *);
    argc = 1;

    /* Count the arguments */

    pTmpArgs = strdup (args);
    if (pTmpArgs == NULL)
    return ERROR;

    pStr = tokenize (pTmpArgs, &pLast);

    while (pStr != NULL)
    {
    /* Each entry of argv is a pointer on a string */

    sizeArgv += (strlen (pStr) + 1 + sizeof (char *));
    argc++;
    pStr = tokenize (NULL, &pLast);
    }

    kfree (pTmpArgs);

    /* Reserve memory for argv array */

    argv = (char **) kmalloc (sizeArgv);
    if (argv == NULL)
    return ERROR;

    /* Copy the arguments */

    argv[0] = (char *)&argv[argc];
    strcpy (argv[0], name);

    pTmpArgs = strdup (args);
    if (pTmpArgs == NULL)
    {
    kfree (argv);
    return ERROR;
    }

    pStr = tokenize (pTmpArgs, &pLast);
    ix = 1;
    while (pStr != NULL)
    {
    argv[ix] = argv[ix - 1] + strlen (argv[ix - 1]) + 1;
    strcpy (argv[ix], pStr);
    pStr = tokenize (NULL, &pLast);
    ix++;
    }

    kfree (pTmpArgs);

    /* Call the function associated to the command */

    status = (pCmd->command) (argc, argv);

    kfree (argv);

    return status;
    }

/*******************************************************************************
*
* symCommandFunc - function for the `sym' command
*
* This function corresponds to the `sym' command. It display the address, the
* type and the value of the symbol named <symbol>.
*
* SYNOPSIS
* sym <symbol> ...
*
* RETURNS: OK, or ERROR if an error occured
*
*/

static int symCommandFunc
    (
    int         argc,
    char **     argv
    )
    {
    int         ix;

    /* At leat one argument is mandatory */

    if (argc == 1)
    {
    printf ("%s: a function address must be specified.\n", argv[0]);
    return ERROR;
    }

    for (ix = 1; ix < argc; ix++)
    {
    SYM_TYPE    symType;
    int *       pSymVal;
    char *      typeStr;

    if (symbolGet (argv[ix], &pSymVal, &symType) != OK)
        {
        printf ("%s: unknown symbol name '%s'.\n", argv[0], argv[ix]);
        continue;
        }

    switch (symType & SYM_BASE_MASK)
        {
        case SYM_TEXT:
        typeStr = "TEXT";
        break;
        case SYM_DATA:
        case SYM_BSS:
        case SYM_ABS:
        case SYM_COMM:
        default:
        typeStr = "DATA";
        }

    printf ("%s: \n"\
        "    address = 0x%x\n"
        "    type: %s\n"\
        "    value: (byte)      = 0x%x\n"\
        "           (short)     = 0x%hx\n"\
        "           (word)      = 0x%x\n"
        "           (long word) = 0x%llx\n",
        argv[ix], (int)pSymVal, typeStr, 
        *(char *)pSymVal, *(short *)pSymVal, *(int *)pSymVal,
        *(long long *)pSymVal);
    }

    return OK;
    }

/*******************************************************************************
*
* symbolGet - get the value of a symbol
*
* This routine gets the address of the symbol value <symbolName>. The 
* address of the value is put in  <ppSymbolValue>. The type of the symbol (as 
* defined by the type SHELL_SYMBOL_TYPE) is returned in <pSymTypeOut>.
*
* RETURNS: OK, or ERROR if the symbol cannot be found.
*
*/

static int symbolGet
    (
    const char *    symbolName,     /* symbol name to look for */
    int **      ppSymbolValue,  /* returned symbol value address */
    SYM_TYPE *      pSymTypeOut     /* symbol type */
    )
    {
    SYMBOL_DESC symbolDesc; /* symFind() descriptor */
    int status;
    char *  tmpSymbolName;

    /* Search for a C symbol name */

    /*
     * Search the symbol table for prepend and not prepend symbol name:
     * compiler for some architectures append the '_' to the symbol name.
     */

    memset (&symbolDesc, 0, sizeof (SYMBOL_DESC));
    symbolDesc.mask = SYM_FIND_BY_NAME;
    symbolDesc.name = (char *)symbolName;

    status = symFind (sysSymTbl, &symbolDesc);

    if (status != OK)
        {
        /* Create a symbol name prepend with '_' */

        tmpSymbolName = kmalloc (1 + strlen (symbolName) + 1);
        if (tmpSymbolName == NULL)
            return ERROR;

        sprintf (tmpSymbolName, "_%s", symbolName);

        memset (&symbolDesc, 0, sizeof (SYMBOL_DESC));

        symbolDesc.mask = SYM_FIND_BY_NAME;

        symbolDesc.name = tmpSymbolName;

        status = symFind (sysSymTbl, &symbolDesc);

        kfree (tmpSymbolName);
        }
    else
        printf("symFind OK!\n");

    if (status == OK)
    {
    *ppSymbolValue  = (int *)symbolDesc.value;
    *pSymTypeOut    = symbolDesc.type;
    }

    return (status);
    }

/*******************************************************************************
*
* strToInt32 - convert a string to a 32 bits integers
*
* This routine converts the string <valueStr> to a 32 bits integer <pInt>.
*
* RETURNS: Ok, or ERROR
*
*/

static int strToInt32
    (
    const char *    valueStr,
    int *       pInt,
    const char *    errorStr    /* or NULL */
    )
    {

    /* Evaluate the string */

  //  if (sscanf (valueStr, "%i", pInt) != 1)
    {
    if (errorStr != NULL)
        printf ("%s: '%s' is not a valid integer value.\n", errorStr,
              valueStr);
    return ERROR;
    }
    pInt = 0;
    return OK;
    }
