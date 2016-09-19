#include <stdint.h>
#undef bcopy            /* in case of LIBSA_USE_MEMCPY */

/*
 * This is designed to be small, not fast.
 */
void
bcopy(const void *s1, void *s2, size_t n)
{
    const char *f = s1; 
    char *t = s2; 

    if (f < t) {
        f += n;
        t += n;
        while (n-- > 0)
            *--t = *--f;
    } else
        while (n-- > 0)
            *t++ = *f++;
}

