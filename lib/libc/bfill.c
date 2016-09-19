#include <ctype.h>

#undef bfill /* so bfill gets built for those who don't include header files */
/*******************************************************************************
*
* bfill - fill a buffer with a specified character
*
* This routine fills the first <nbytes> characters of a buffer with the
* character <ch>.  Filling is done in the most efficient way possible,
* which may be long-word, or even multiple-long-word stores, on some
* architectures.  In general, the fill will be significantly registerer if
* the buffer is long-word aligned.  (For filling that is restricted to
* byte stores, see the manual entry for bfillBytes().)
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* SEE ALSO: bfillBytes()
*/

void bfill
    (
    register char *buf,           /* pointer to buffer              */
    size_t nbytes,            /* number of bytes to fill        */
    register int ch               /* char with which to fill buffer */
    )
    {
    register long *pBuf;
    char *bufend = buf + nbytes;
    register char *buftmp;
    register long val;

    if (nbytes < 2 * sizeof(long) - 1)
    goto byte_fill;

    /*
     * Start on necessary alignment. This loop advances buf
     * at most sizeof(long) - 1 bytes.
     */

    while ((long)buf & (sizeof(long) - 1))
        {
    *buf++ = (char) ch;
        }

    /* write sizeof(long) bytes at a time */
    val = (unsigned char)ch;  /* memset() wants the cast */
    val |= (val << 8);
    val |= (val << 16);

    /* Find the last long word boundary at or before bufend */
    buftmp = (char *)((long)bufend & ~(sizeof(long) - 1));

    pBuf = (long *)buf;

    /* fill 4 bytes (8 bytes for LP64) at a time; don't exceed buf endpoint */

    do
    {
    /* Assert: buftmp and pBuf are sizeof(long) aligned */
    /* Assert: buftmp - (char*)pBuf >= sizeof (long) */
    *pBuf++ = val;
    }
    while ((char *)pBuf != buftmp);

    buf = (char *)pBuf;

    /* fill remaining bytes one at a time */

byte_fill:
    while (buf != bufend)
        {
    *buf++ = (char) ch;
        }
    }
