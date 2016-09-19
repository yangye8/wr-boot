/*
 * vivi/lib/heap.c: simple dynamic memory allocation routine
 *
 * Based on bootldr/heap.c
 *
 */

#define HEAP_SIZE               0x3fc00000
#define HEAP_BASE               0x1

extern void printf(char * frmt,...);

#define BLOCKHEAD_SIGNATURE 0x0000F00D

typedef struct blockhead_t {
    long signature;
    char allocated;
    unsigned long size;
    struct blockhead_t *next;
} blockhead;

static blockhead *gHeapBase = 0;

static inline int first_fit_malloc_init(unsigned char *heap, unsigned long size)
{
//    if (gHeapBase != 0) 
//        return -1;

    printf("initialize heap area [0x%x-0x%x]\n", (unsigned int)heap, heap+size);

    gHeapBase = (blockhead *)(heap);
    gHeapBase->allocated=0;
    gHeapBase->signature=BLOCKHEAD_SIGNATURE;
    gHeapBase->next=0;
    gHeapBase->size = size - sizeof(blockhead);

    return 0;
}

int heap_init(void)
{
    return first_fit_malloc_init((unsigned char *)(HEAP_BASE), HEAP_SIZE);  
}

void *fisrt_fit_malloc(unsigned long size) 
{
    blockhead *blockptr = gHeapBase;
    blockhead *newblock;

    size = (size+7)&~7; /* unsigned long align the size */

    printf("malloc(): size = 0x%x blockptr-%x allocated-%d\n", size,blockptr,blockptr->allocated);

    while (blockptr != 0) {
        if (blockptr->allocated == 0) 
        {
        printf("blockptr->size = 0x%x size-%x\n", blockptr->size,size);
            if (blockptr->size >= size) 
            {
                blockptr->allocated=1;
                if ((blockptr->size - size) > sizeof(blockhead))
                {
                    newblock = (blockhead *)((unsigned char *)(blockptr) + sizeof(blockhead) + size);
                    newblock->signature = BLOCKHEAD_SIGNATURE;
                    newblock->next = blockptr->next;
                    newblock->size = blockptr->size - size - sizeof(blockhead);
                    newblock->allocated = 0;
                    blockptr->next = newblock;
                    blockptr->size = size;
                } 
                break;
            }
        }
        blockptr = blockptr->next;
    }

    if (blockptr == 0)
        printf("Error: malloc(), out of storage. size = 0x%x\n", size);

    return (blockptr != 0) ? ((unsigned char *)(blockptr)+sizeof(blockhead)) : 0;
}

void first_fit_free(void *block) {
    blockhead *blockptr;

    if (block == 0) return;

    blockptr = (blockhead *)((unsigned char *)(block) - sizeof(blockhead));

    if (blockptr->signature != BLOCKHEAD_SIGNATURE) return;

    blockptr->allocated=-1;
    return;
}



void * kmalloc(unsigned long size)
{
return fisrt_fit_malloc(size);
}

void kfree(void * block)
{
return first_fit_free(block);
}


/******************************************************************************
*
* xmalloc - allocate unused space for an object
*
* This routine allocates unused space for an object whose size in bytes is
* specified by 'size' parameter and whose value is unspecified. This routine calls malloc,
* and if allocation failed, the routine will hang the system.
*
* RETURNS: the pointer to the allocated block.
*
* ERRNO: ENOMEM indicating "insufficient storage space is available"
*
* \NOMANUAL
*/

void * xmalloc
    (
    unsigned long size
    )
    {
    void * ret = kmalloc(size);
    if (!ret)
        {
        printf("%s: failed\n", __func__);
        }
    return ret;
    }


 /******************************************************************************
*
* calloc - allocate unused space for an array of elements
*
* This routine allocates unused space for an array of 'num' elements each of
* whose size in bytes is 'sz'. The space shall be initialized to all bits 0
*
* RETURNS: the pointer to the allocated block or NULL if allocation fails.
*
* ERRNO: See malloc
*
* \NOMANUAL
*/

void *calloc
    (
    unsigned long num,
    unsigned long sz
    )
    {
    void * p = kmalloc(num * sz);
    if (p)
        {
        memset(p, 0, num * sz);
        }
    return p;
    }
