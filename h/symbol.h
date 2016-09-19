/* symbol.h - symbol structure header */

#ifndef __INCsymbolh
#define __INCsymbolh

#include <sllLib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* symbol types */

typedef char *      SYM_VALUE;
typedef unsigned char   SYM_TYPE;
typedef unsigned short      SYM_GROUP;
typedef unsigned long       SYM_REF;

typedef struct symbol           /* SYMBOL - entry in symbol table */
    {
    SL_NODE     nameHNode;      /* hash node (must come first) */
    char *  name;       /* pointer to symbol name */
    SYM_VALUE   value;      /* symbol value */
    SYM_REF symRef;     /* Id of module, or predefined SYMREF. */
    SYM_GROUP   group;          /* symbol group */
    SYM_TYPE    type;           /* symbol type */
    } SYMBOL;

typedef SYMBOL * SYMBOL_ID;

/* 
 * NOTE: Do not use these symbol type macro definitions directly.  Instead 
 * use the SYM_IS_<TYPE> masks, defined below, to test whether a symbol 
 * is global, absolute, etc.  This will prevent backwards and/or forwards
 * compatibility issues in the case that it becomes necessary to change 
 * the values of these macros or method of storing the symbol type. 
 */

#define SYM_UNDF        0x0     /* undefined (lowest 8 bits only) */  
#define SYM_GLOBAL      0x1     /* global (external) */
#define SYM_ABS         0x2     /* absolute */
#define SYM_TEXT        0x4     /* text */
#define SYM_DATA        0x8     /* data */             
#define SYM_BSS        0x10     /* bss */              
#define SYM_COMM       0x20     /* common symbol */    
#define SYM_LOCAL      0x40     /* local */            
#define SYM_THUMB      0x80 /* Thumb function */

#define SYM_IS_UNDF(symType)   (!(symType))
#define SYM_IS_GLOBAL(symType) ((symType) & SYM_GLOBAL) 
#define SYM_IS_LOCAL(symType)  ((symType) & SYM_LOCAL) 
#define SYM_IS_TEXT(symType)   ((symType) & SYM_TEXT) 
#define SYM_IS_DATA(symType)   ((symType) & SYM_DATA) 
#define SYM_IS_BSS(symType)    ((symType) & SYM_BSS) 
#define SYM_IS_ABS(symType)    ((symType) & SYM_ABS) 
#define SYM_IS_COMMON(symType) ((symType) & SYM_COMM) 

/* symbol masks */ 

#define SYM_MASK_ALL    0xff            /* all bits of symbol type valid */
#define SYM_MASK_NONE   0x00        /* no bits of symbol type valid */
#define SYM_MASK_EXACT  0x1ff       /* match symbol pointer exactly */

#define SYM_MASK_ANY_TYPE    SYM_MASK_NONE  /* ignore type in searches */
#define SYM_MASK_EXACT_TYPE  SYM_MASK_ALL   /* match type exactly in searches */

#ifdef __cplusplus
}
#endif

#endif /* __INCsymbolh */
