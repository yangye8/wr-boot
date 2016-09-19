#include <string.h>

void * memmove
    (
    void *   destination,   /* destination of copy */
    const void * source,    /* source of copy */
    unsigned int     size       /* size of memory to copy */
    )
    {
    bcopy ((char *)source, (char *)destination, size);
    return (destination);
    }
