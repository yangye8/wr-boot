/*
 * Copied from <file:arch/powerpc/kernel/misc_32.S>
 *
 * This file contains miscellaneous low-level functions.
 *    Copyright (C) 1995-1996 Gary Thomas (gdt@linuxppc.org)
 *
 * Largely rewritten by Cort Dougan (cort@cs.nmt.edu)
 * and Paul Mackerras.
 *
 * kexec bits:
 * Copyright (C) 2002-2003 Eric Biederman  <ebiederm@xmission.com>
 * GameCube/ppc32 port Copyright (C) 2004 Albert Herranz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */
#include "toolPpc.h"
#define SPRN_TBRL       268
#define SPRN_TBRU       269
#define SPRN_PVR        0x11F   /* Processor Version Register */

    /* globals */



    .globl getCpuId
    .globl setTimeBase
    .globl getHid0
    .globl setHid0
    .globl vxSdaInit


#if 1
        .globl readb
        .globl readw
        .globl readl
        .globl writeb
        .globl writew
        .globl writel

    .text
    .balign 4

/*******************************************************************************
*
* readb - read 1 byte of data from 'addr'
*
* This routine reads 1 byte of data from 'addr'. This routine guarantees
* that read operation is finished before it returns.
*
* RETURNS: value read
*
* ERRNO: N/A
*
* \NOMANUAL
*/

readb:
    lbzx    r3, r0, r3
    eieio
    blr

/*******************************************************************************
*
* readw - read w bytes of data from 'addr'
*
* This routine reads w bytes of data from 'addr'. This routine guarantees
* that read operation is finished before it returns.
*
* RETURNS: value read
*
* ERRNO: N/A
*
* \NOMANUAL
*/

readw:
    lhzx    r3, r0, r3
    eieio
    blr

/*******************************************************************************
*
* readl - read 4 bytes of data from 'addr'
*
* This routine reads 4 bytes of data from 'addr'. This routine guarantees
* that read operation is finished before it returns.
*
* RETURNS: value read
*
* ERRNO: N/A
*
* \NOMANUAL
*/

readl:
    lwzx    r3, r0, r3
    eieio
    blr

/*******************************************************************************
*
* writeb - write 1 byte of 'val' to 'addr'
*
* This routine writes 1 byte of 'val' to 'addr'. This routine guarantees
* that write operation is finished before it returns.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

writeb:
    stbx    r4, r0, r3
    eieio
    blr

/*******************************************************************************
*
* writew - write 2 bytes of 'val' to 'addr'
*
* This routine writes 1 bytes of 'val' to 'addr'. This routine guarantees
* that write operation is finished before it returns.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

writew:
    sthx    r4, r0, r3
    eieio
    blr

/*******************************************************************************
*
* writel - write 4 bytes of 'val' to 'addr'
*
* This routine writes 4 bytes of 'val' to 'addr'. This routine guarantees
* that write operation is finished before it returns.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/
 
writel:
    stwx    r4, r0, r3
    eieio
    blr
#endif


    .text
    .balign 4

/*******************************************************************************
*
* getCpuId - get CPU physical ID
*
* This routine gets CPU physical ID
*
* UINT32 getCpuId
* (
*   void
* )
*
* RETURNS: CPU physical ID
*
* \NOMANUAL
*/


getCpuId:
    mfspr r3, PIR
    blr

/*******************************************************************************
*
* getHid0 - get HID0
*
* This routine gets HID0
*
* void getHid0
* (
*  void
* )
*
* RETURNS: N/A
*
* \NOMANUAL
*/

getHid0:
    mfspr   r3, HID0
    blr

/*******************************************************************************
*
* setHid0 - set HID0
*
* This routine sets HID0
*
* void setHid0
* (
*  void
* )
*
* RETURNS: N/A
*
* \NOMANUAL
*/

setHid0:
    mtspr   HID0, r3
    blr

/*******************************************************************************
*
* setTimeBase - set Time Base for the initialization phase.
*
* This routine is used to set Time Base for the initialization phase.
*
* void setTimeBase
* (
*   UINT32 tbHI
*   UINT32 tbLO
* )
*
* RETURNS: N/A
*
* \NOMANUAL
*/

setTimeBase:
    mttbl  p0
    mttbu  p1
    blr

/*******************************************************************************
*
* vxSdaInit - initialize Small Data Area (SDA)
*
* This routine initializes Small Data Area (SDA)
*
* void vxSdaSet (void)
*
* RETURNS: N/A
*
* \NOMANUAL
*/

vxSdaInit:
    lis     r2, HI(_SDA2_BASE_)
    ori     r2, r2, LO(_SDA2_BASE_)
    lis     r13, HI(_SDA_BASE_)
    ori r13, r13, LO(_SDA_BASE_)
    blr

    .text

/* usdelay (on non-601 processors) needs to know the period of the
 * timebase in nanoseconds.  This used to be hardcoded to be 60ns
 * (period of 66MHz/4).  Now a variable is used that is initialized to
 * 60 for backward compatibility, but it can be overridden as necessary
 * with code something like this:
 *    extern unsigned long timebase_period_ns;
 *    timebase_period_ns = 1000000000 / bd->bi_tbfreq;
 */
    .data
    .globl timebase_period_ns
timebase_period_ns:
    .long   60 
    .text
/*
 * Delay for a number of microseconds
 */
    .globl  usdelay
usdelay:
    mfspr   r4,SPRN_PVR
    srwi    r4,r4,16
    cmpwi   0,r4,1      /* 601 ? */
    bne .Lusdelay_not_601
00: li  r0,86   /* Instructions / microsecond? */
    mtctr   r0
10: addi    r0,r0,0 /* NOP */
    bdnz    10b
    subic.  r3,r3,1
    bne 00b
    blr

.Lusdelay_not_601:
    mulli   r4,r3,1000  /* nanoseconds */
    /*  Change r4 to be the number of ticks using:
     *  (nanoseconds + (timebase_period_ns - 1 )) / timebase_period_ns
     *  timebase_period_ns defaults to 60 (16.6MHz) */
    mflr    r5
    bl  0f
0:  mflr    r6
    mtlr    r5
    lis r5,0b@ha
    addi    r5,r5,0b@l
    subf    r5,r5,r6    /* In case we're relocated */
    addis   r5,r5,timebase_period_ns@ha
    lwz r5,timebase_period_ns@l(r5)
    add r4,r4,r5
    addi    r4,r4,-1
    divw    r4,r4,r5    /* BUS ticks */
#ifdef CONFIG_8xx
1:  mftbu   r5
    mftb    r6
    mftbu   r7
#else
1:  mfspr   r5, SPRN_TBRU
    mfspr   r6, SPRN_TBRL
    mfspr   r7, SPRN_TBRU
#endif
    cmpw    0,r5,r7
    bne 1b      /* Get [synced] base time */
    addc    r9,r6,r4    /* Compute end time */
    addze   r8,r5
#ifdef CONFIG_8xx
2:  mftbu   r5
#else
2:  mfspr   r5, SPRN_TBRU
#endif
    cmpw    0,r5,r8
    blt 2b
    bgt 3f
#ifdef CONFIG_8xx
    mftb    r6
#else
    mfspr   r6, SPRN_TBRL
#endif
    cmpw    0,r6,r9
    blt 2b
3:  blr

  .text
    .globl  strcpy
strcpy:
    addi    r5,r3,-1
    addi    r4,r4,-1
1:  lbzu    r0,1(r4)
    cmpwi   0,r0,0
    stbu    r0,1(r5)
    bne 1b
    blr

    .globl  strncpy
strncpy:
    cmpwi   0,r5,0
    beqlr
    mtctr   r5
    addi    r6,r3,-1
    addi    r4,r4,-1
1:  lbzu    r0,1(r4)
    cmpwi   0,r0,0
    stbu    r0,1(r6)
    bdnzf   2,1b        /* dec ctr, branch if ctr != 0 && !cr0.eq */
    blr

    .globl  strcat
strcat:
    addi    r5,r3,-1
    addi    r4,r4,-1
1:  lbzu    r0,1(r5)
    cmpwi   0,r0,0
    bne 1b
    addi    r5,r5,-1
1:  lbzu    r0,1(r4)
    cmpwi   0,r0,0
    stbu    r0,1(r5)
    bne 1b
    blr

    .globl  strchr
strchr:
    addi    r3,r3,-1
1:  lbzu    r0,1(r3)
    cmpw    0,r0,r4
    beqlr
    cmpwi   0,r0,0
    bne 1b
    li  r3,0
    blr

    .globl  strcmp
strcmp:
    addi    r5,r3,-1
    addi    r4,r4,-1
1:  lbzu    r3,1(r5)
    cmpwi   1,r3,0
    lbzu    r0,1(r4)
    subf.   r3,r0,r3
    beqlr   1
    beq 1b
    blr

    .globl  strncmp
strncmp:
    mtctr   r5
    addi    r5,r3,-1
    addi    r4,r4,-1
1:  lbzu    r3,1(r5)
    cmpwi   1,r3,0
    lbzu    r0,1(r4)
    subf.   r3,r0,r3
    beqlr   1
    bdnzt   eq,1b
    blr

    .globl  strlen
strlen:
    addi    r4,r3,-1
1:  lbzu    r0,1(r4)
    cmpwi   0,r0,0
    bne 1b
    subf    r3,r3,r4
    blr

    .globl  memset
memset:
    rlwimi  r4,r4,8,16,23
    rlwimi  r4,r4,16,0,15
    addi    r6,r3,-4
    cmplwi  0,r5,4
    blt 7f
    stwu    r4,4(r6)
    beqlr
    andi.   r0,r6,3
    add r5,r0,r5
    subf    r6,r0,r6
    rlwinm  r0,r5,32-2,2,31
    mtctr   r0
    bdz 6f
1:  stwu    r4,4(r6)
    bdnz    1b
6:  andi.   r5,r5,3
7:  cmpwi   0,r5,0
    beqlr
    mtctr   r5
    addi    r6,r6,3
8:  stbu    r4,1(r6)
    bdnz    8b
    blr

    .globl  bcopy
bcopy:
    mr  r6,r3
    mr  r3,r4
    mr  r4,r6
    b   memcpy

    .globl  memmove
memmove:
    cmplw   0,r3,r4
    bgt backwards_memcpy
    /* fall through */

    .globl  memcpy
memcpy:
    rlwinm. r7,r5,32-3,3,31     /* r7 = r5 >> 3 */
    addi    r6,r3,-4
    addi    r4,r4,-4
    beq 3f          /* if less than 8 bytes to do */
    andi.   r0,r6,3         /* get dest word aligned */
    mtctr   r7
    bne 5f
    andi.   r0,r4,3         /* check src word aligned too */
    bne 3f
1:  lwz r7,4(r4)
    lwzu    r8,8(r4)
    stw r7,4(r6)
    stwu    r8,8(r6)
    bdnz    1b
    andi.   r5,r5,7
2:  cmplwi  0,r5,4
    blt 3f
    lwzu    r0,4(r4)
    addi    r5,r5,-4
    stwu    r0,4(r6)
3:  cmpwi   0,r5,0
    beqlr
    mtctr   r5
    addi    r4,r4,3
    addi    r6,r6,3
4:  lbzu    r0,1(r4)
    stbu    r0,1(r6)
    bdnz    4b
    blr
5:  subfic  r0,r0,4
    cmpw    cr1,r0,r5
    add r7,r0,r4
    andi.   r7,r7,3         /* will source be word-aligned too? */
    ble cr1,3b
    bne 3b          /* do byte-by-byte if not */
    mtctr   r0
6:  lbz r7,4(r4)
    addi    r4,r4,1
    stb r7,4(r6)
    addi    r6,r6,1
    bdnz    6b
    subf    r5,r0,r5
    rlwinm. r7,r5,32-3,3,31
    beq 2b
    mtctr   r7
    b   1b

    .globl  backwards_memcpy
backwards_memcpy:
    rlwinm. r7,r5,32-3,3,31     /* r7 = r5 >> 3 */
    add r6,r3,r5
    add r4,r4,r5
    beq 3f
    andi.   r0,r6,3
    mtctr   r7
    bne 5f
    andi.   r0,r4,3
    bne 3f
1:  lwz r7,-4(r4)
    lwzu    r8,-8(r4)
    stw r7,-4(r6)
    stwu    r8,-8(r6)
    bdnz    1b
    andi.   r5,r5,7
2:  cmplwi  0,r5,4
    blt 3f
    lwzu    r0,-4(r4)
    subi    r5,r5,4
    stwu    r0,-4(r6)
3:  cmpwi   0,r5,0
    beqlr
    mtctr   r5
4:  lbzu    r0,-1(r4)
    stbu    r0,-1(r6)
    bdnz    4b
    blr
5:  cmpw    cr1,r0,r5
    subf    r7,r0,r4
    andi.   r7,r7,3
    ble cr1,3b
    bne 3b
    mtctr   r0
6:  lbzu    r7,-1(r4)
    stbu    r7,-1(r6)
    bdnz    6b
    subf    r5,r0,r5
    rlwinm. r7,r5,32-3,3,31
    beq 2b
    mtctr   r7
    b   1b

    .globl  memchr
memchr:
    cmpwi   0,r5,0
    blelr
    mtctr   r5
    addi    r3,r3,-1
1:  lbzu    r0,1(r3)
    cmpw    r0,r4
    beqlr
    bdnz    1b
    li  r3,0
    blr

    .globl  memcmp
memcmp:
    cmpwi   0,r5,0
    ble 2f
    mtctr   r5
    addi    r6,r3,-1
    addi    r4,r4,-1
1:  lbzu    r3,1(r6)
    lbzu    r0,1(r4)
    subf.   r3,r0,r3
    bdnzt   2,1b
    blr
2:  li  r3,0
    blr
