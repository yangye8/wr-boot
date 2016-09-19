/* ffsLib.c - find first bit set library */

/*
DESCRIPTION
This library contains routines to find the first bit set in 32 bit and 64 bit
fields.  It is utilized by bit mapped priority queues and hashing functions.
*
* INCLUDE FILES: ffsLib.h
*/

#include <wrboot.h>

unsigned char ffsMsbTbl [256] =			/* lookup table for ffsMsb() */
    {
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    };

unsigned char ffsLsbTbl [256] =                 /* lookup table for ffsLsb() */
    {
    0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    };

/******************************************************************************
*
* ffs32Msb - find most significant bit set
*
* This routine finds the most significant bit set in the 32 bit argument
* passed to it and returns the index of that bit.  Bits are numbered starting
* at 1 from the least significant bit.  A return value of zero indicates that
* the value passed is zero.
*
* RETURNS: index of most significant bit set, or zero
* ERRNO: N/A
*/

int ffs32Msb
    (
    unsigned int i        /* value in which to find first set bit */
    )
    {
    union
	{ 
	unsigned int dword;
	unsigned short word[2];
	unsigned char byte[4];
	} i_u;

    i_u.dword = i;

#if (_BYTE_ORDER == _BIG_ENDIAN)

    if (i_u.word[0])	/* Most significant 16-bit */
	{
	if (i_u.byte[0])
	    return (ffsMsbTbl[i_u.byte[0]] + 24 + 1);
	else
	    return (ffsMsbTbl[i_u.byte[1]] + 16 + 1);
	}
    else		/* Least significant 16-bit */
	{
	if (i_u.byte[2])
	    return (ffsMsbTbl[i_u.byte[2]] + 8 + 1);
	else
	    return (ffsMsbTbl[i_u.byte[3]] + (i ? 1 : 0));
	}

#else /* _BYTE_ORDER == _LITTLE_ENDIAN */

    if (i_u.word[1])	/* Most significant 16-bit */
	{
	if (i_u.byte[3])
	    return (ffsMsbTbl[i_u.byte[3]] + 24 + 1);
	else
	    return (ffsMsbTbl[i_u.byte[2]] + 16 + 1);
	}
    else		/* Least significant 16-bit */
	{
	if (i_u.byte[1])
	    return (ffsMsbTbl[i_u.byte[1]] + 8 +1);
	else
	    return (ffsMsbTbl[i_u.byte[0]] + (i ? 1 : 0));
	}

#endif /* _BYTE_ORDER */
    }

/******************************************************************************
*
* ffs32Lsb - find least significant bit set
*
* This routine finds the least significant bit set in the 32 bit argument
* passed to it and returns the index of that bit.  Bits are numbered starting
* at 1 from the least significant bit.  A return value of zero indicates that
* the value passed is zero.
*
* RETURNS: index of least significant bit set, or zero
* ERRNO: N/A
*/

int ffs32Lsb
    (
    unsigned int i        /* value in which to find first set bit */
    )
    {
    union
	{ 
	unsigned int dword;
	unsigned short word[2];
	unsigned char byte[4];
	} i_u;

    i_u.dword = i;

#if (_BYTE_ORDER == _BIG_ENDIAN)

    if (i_u.word[1])	/* Least significant 16-bit */
	{
	if (i_u.byte[3])
	    return (ffsLsbTbl[i_u.byte[3]] + 1);
	else
	    return (ffsLsbTbl[i_u.byte[2]] + 8 + 1);
	}
    else		/* Most significant 16-bit */
	{
	if (i_u.byte[1])
	    return (ffsLsbTbl[i_u.byte[1]] + 16 + 1);
	else
	    return (ffsLsbTbl[i_u.byte[0]] + (i ? 24+1 : 0));
	}

#else /* _BYTE_ORDER == _LITTLE_ENDIAN */

    if (i_u.word[0])	/* Least significant 16-bit */
	{
	if (i_u.byte[0])
	    return (ffsLsbTbl[i_u.byte[0]] + 1);
	else
	    return (ffsLsbTbl[i_u.byte[1]] + 8 + 1);
	}
    else		/* Most significant 16-bit */
	{
	if (i_u.byte[2])
	    return (ffsLsbTbl[i_u.byte[2]] + 16 + 1);
	else
	    return (ffsLsbTbl[i_u.byte[3]] + (i ? 24+1 : 0));
	}

#endif /* _BYTE_ORDER */
    }

int ffsMsb (unsigned int i)
    {
    return ffs32Msb (i);
    }

int ffsLsb (unsigned int i)
    {
    return ffs32Lsb (i);
    }