#ifndef PPC_BOOT_H
#define PPC_BOOT_H

#ifdef __cplusplus
extern "C" {
#endif

#define OK      0
#define ERROR   (-1)

#undef  TRUE
#define TRUE    1
#undef  FALSE
#define FALSE   0

#define EOS     '\0'    /* C string terminator */

#ifndef NULL
#define NULL    0
#endif

#define ROUND_UP(x, align)  (((size_t) (x) + (align - 1)) & ~(align - 1))

#define NELEMENTS(array)    (sizeof (array) / sizeof ((array) [0]))

#define __address_add_offset(addr, size) ((void *)((char *)(addr) + (size)))
#define __address_del_offset(addr, size) ((void *)((char *)(addr) - (size)))

#define EOS '\0'

#define be32_to_cpu(x)        (x)

#define ULONG_MAX       4294967295LU

/* handy sizes */
#define SZ_1K                           0x00000400
#define SZ_4K                           0x00001000
#define SZ_8K                           0x00002000
#define SZ_16K                          0x00004000
#define SZ_32K                          0x00008000
#define SZ_64K                          0x00010000
#define SZ_128K                         0x00020000
#define SZ_256K                         0x00040000
#define SZ_512K                         0x00080000

#define SZ_1M                           0x00100000
#define SZ_2M                           0x00200000
#define SZ_4M                           0x00400000
#define SZ_8M                           0x00800000
#define SZ_16M                          0x01000000
#define SZ_32M                          0x02000000
#define SZ_64M                          0x04000000
#define SZ_128M                         0x08000000
#define SZ_256M                         0x10000000
#define SZ_512M                         0x20000000

#define SZ_1G                           0x40000000
#define SZ_2G                           0x80000000

#define CHAR_FROM_CONST(x) (char *)(x)
#define VOID_FROM_CONST(x) (void *)(x)

#ifndef max
#define max(x, y)       (((x) < (y)) ? (y) : (x))
#endif

#ifndef min
#define min(x, y)       (((x) < (y)) ? (x) : (y))
#endif

#if !(defined(ARRAY_SIZE))
#define ARRAY_SIZE(arr)   (sizeof(arr) / sizeof((arr)[0]))
#endif /* !defined(ARRAY_SIZE) */

#define FAST    register
#define IMPORT  extern
#define LOCAL   static

enum handleContextType
    {
    handleContextTypeNone   = 0,
    handleContextTypeUser,
    handleContextTypeOms,
    handleContextTypeCOM,
    handleContextTypeI2O
    };

enum handleType
    {
    handleTypeUnknown   = -1,   /* bad handle               */
    handleTypeStart = 100,  /* starting value for handle types  */
    handleTypeHashTbl   = 100,  /* hashLib HASH_TBL         */
    handleTypeSymTbl,       /* symLib SYMTAB            */
    handleTypeFile,     /* stdioLib FILE            */
    handleTypeRbuff,        /* rbuffLib RBUFF           */
    handleTypeModuleDesc,   /* moduleLib MODULE_DESC        */
    handleTypeClass,        /* classLib OBJ_CLASS           */
    handleTypeModuleListDesc,   /* moduleLib MODLIST_DESC       */
    handleTypeModuleHdl,    /* malLib MODULE_HDL            */
    handleTypeSectionDesc,  /* moduleLib SECTION_DESC       */
    handleTypeEnvironment,  /* envLib ENV_TBL           */
    handleTypeCbioHdl,      /* cbioLib CBIO_DEV handle      */
    handleTypeHamChannel,   /* hamLib (Foundation HA) HAM_ID    */

   /* see comments in posix section on windNumObjClass */
    handleNumType   = handleTypeHamChannel - handleTypeStart + 1
    };


typedef struct
    {
    unsigned long     magic;          /* 0x00/0x00: magic. Used in HANDLE_VERIFY() */
    unsigned int    safeCnt;          /* 0x04/0x08: safe count */
    unsigned short    attributes;     /* 0x08/0x0c: attribute bit set */
    char      type;           /* 0x0a/0x0e: enum windObjClassType */
    unsigned char     contextType;    /* 0x0b/0x0f: enum handleContextType */
    void *    context;        /* 0x0c/0x10: WRS defined context */
    } HANDLE;

typedef HANDLE * HANDLE_ID;             /* handle id */


/* macro declarations */

#define HANDLE_VERIFY(handle,handleType)                \
    (                                   \
        (                               \
        (((HANDLE_ID) (handle))->magic == (unsigned long)(handle))      \
    &&                              \
        (((HANDLE_ID) (handle))->type == (char)(handleType))    \
    )                               \
    ?                                   \
    OK                              \
    :                                   \
    ERROR                               \
    )

/* externs */

extern int handleInit (HANDLE_ID handleId, enum handleType type);

#define wr_read8(x)    *(volatile unsigned char *)(x)
#define wr_read16(x)    *(volatile unsigned short *)(x)
#define wr_read32(x)    *(volatile unsigned long *)(x)

#define wr_write8(x,v) *(volatile unsigned char *)(x) = (v)
#define wr_write16(x,v) *(volatile unsigned short *)(x) = (v)
#define wr_write32(x,v) *(volatile unsigned long *)(x) = (v)

__inline void  write(long *to, unsigned long value)
{
    volatile long *addr = to;
    *addr = value;
}

__inline unsigned long read(long *from)
{
    volatile long *addr = from;
    return *addr;
}

#ifndef container_of
#define container_of(p, stype, field) ((stype *)(((uint8_t *)(p)) - offsetof(stype, field)))
#endif

#ifdef __cplusplus
}
#endif

#endif
