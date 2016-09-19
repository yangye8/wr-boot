#include <stdint.h>
int memcmp
    (
    const void * s1,		/* array 1 */
    const void * s2,		/* array 2 */
    size_t n   		/* size of memory to compare */
    )
    {
    const unsigned char *p1;
    const unsigned char *p2;

    /* size of memory is zero */

    if (n == 0)
	return (0);

    /* compare array 2 into array 1 */

    p1 = s1;
    p2 = s2;

    while (*p1++ == *p2++)
	{
	if (--n == 0)
	    return (0);
        }

    return ((*--p1) - (*--p2));
    }
