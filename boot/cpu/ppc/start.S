/* head.S - ROM bootcode for p2020 */
#include "toolPpc.h"

#include "../../board/p2020rdb/p2020rdb.h"

#ifndef MMU_STATE_IPROT
#define MMU_STATE_IPROT 0x40000000
#endif

/* special purpose register encoding */

#define XER        1    /* external exception register */
#define LR         8    /* link register (return address) */
#define CTR        9    /* counter register */

#define _PPC_MSR_BIT_EE         16      /* MSR Ext. Intr. Enable bit - EE */
#define _PPC_HID0_TBEN          0x00004000      /* time base enable */
#define BOOT_COLD          0
#define BOOT_WARM_AUTOBOOT 1
#define MMU_ROM_ACCESS (MMU_STATE_CACHEABLE_WRITETHROUGH | MMU_STATE_CACHEABLE | \
                        MMU_STATE_MEM_COHERENCY          | MMU_STATE_GUARDED)

/* Macro for hiadjust and lo */

#define HIADJ(arg)  (arg)@ha
#define HI(arg)     (arg)@h
#define LO(arg)     (arg)@l

#define WRITEADR(reg1,reg2,addr32,val)  \
    lis reg1, HI(addr32);               \
    ori reg1, reg1, LO(addr32);         \
    lis reg2, HI(val);                  \
    ori reg2, reg2, LO(val);            \
    stw reg2, 0(reg1)

#define WRITEOFFSET(regbase,reg2,offset,val) \
    lis reg2, HI(val);                  \
    ori reg2, reg2, LO(val);            \
    stw reg2, offset(regbase);

#define WRITE_BYTE(reg1,reg2,addr32,val)\
    lis reg1, HI(addr32);               \
    ori reg1, reg1, LO(addr32);         \
    lis reg2, 0x00;                     \
    ori reg2, reg2, LO(val);            \
    stb reg2, 0(reg1)

#define WRITEOFFSET_BYTE(regbase,reg2,offset,val) \
        lis   reg2,     HI(val); \
        ori   reg2,     reg2, LO(val); \
        stb   reg2,     offset(regbase)

#define INT_MASK(src, des) rlwinm des, src, 0, _PPC_MSR_BIT_EE+1, _PPC_MSR_BIT_EE-1

    .text 
    .balign  4       /* 4 byte text segment alignment */

#if 0
    .fill 0x100,1,0
#endif
    .global _romInit
romInit:
_romInit:
 //   bl     resetEntry

    /* Useful to have recognizeable string */
//    .data
	.globl version_String
version_string:
    .ascii "WRBOOT 1.0.0"
    .ascii " (",__DATE__, " - ",__TIME__, ")"
    .ascii " Freescale P2020RDB Board", "\0"
    .balign 4

platpllTable:
    .long 0x00000000
    .long 0x04050600
    .long 0x08000a00
    .long 0x0c000000
    .long 0x00000000
    .long 0x00000000

   bl     resetEntry

    .global cold
cold:

    li  r2, BOOT_COLD
    b   start_core

    .global start_code
start_core:

    /* turn off exceptions */

    mfmsr   r3                      /* r3 = msr              */
    INT_MASK (r3, r4)               /* mask EE and CE bit    */
    rlwinm  r4, r4, 0, 20, 18       /* turn off _PPC_MSR_ME  */
    mtmsr   r4                      /* msr = r4              */
    isync

    xor     r0, r0, r0
    addi    r1, r0, -1

    mtspr   DEC, r0
    mtspr   TBL, r0
    mtspr   TBU, r0
    mtspr   TSR, r1
    mtspr   TCR, r0
    mtspr   ESR, r0                 /* clear Exception Syndrome Reg */
    mtspr   XER, r0                 /* clear Fixed-Point Exception Reg */

    xor     r6, r6, r6
    msync
    isync
    mtspr   L1CSR0, r6              /* Invalidate the Data cache */
    li      r6, 0x0000
    msync
    isync
    mtspr   L1CSR0, r6              /* Disable the Data cache */
    li      r6, 0x0002
    msync
    isync
    mtspr   L1CSR1, r6              /* Disable the Instrunction cache */
    li      r6, 0x0002
    msync
    isync
    mtspr   L1CSR1, r6              /* Invalidate the Instruction cache */
    isync
    li      r6, 0x0000
    msync
    isync
    mtspr   L1CSR1, r6              /* Temp disable Instruction cache*/

    isync

    li      r7, 0x0001
    msync
    isync
    mtspr   L1CSR1, r7              /* enable the instruction cache */


    msync
    isync

    xor     r6, r6, r6
    xor     r7, r7, r7
    mullw   r7, r7, r6

#if 0
    lis     sp, HI(_romInit - 0x200000)
    ori     sp, sp, LO(_romInit - 0x200000)
    addi    sp, sp, -FRAMEBASESZ
#else
 /* Setup stack pointer */

    lis     sp, HI(LOCAL_MEM_SIZE - 0x200000)
    ori     sp, sp, LO(LOCAL_MEM_SIZE - 0x200000)
    addi    sp, sp, -FRAMEBASESZ
#endif

    lis     r6, HI(_romInit)
    ori     r6, r6, LO(_romInit)
    lis     r7, HI(bootInit)
    ori     r7, r7, LO(bootInit)
    lis     r8, HI(ROM_TEXT_ADRS)
    ori     r8, r8, LO(ROM_TEXT_ADRS)
    sub     r6, r7, r6              /* routine - entry point */
    add     r6, r6, r8              /* + ROM base */
    mtspr   LR, r6
    mr      r3, r2
    blr

/*
* read the contents the lower half of the Time Base Register (TBL - TBR 268).
*/
.globl sysTimeBaseLGet
sysTimeBaseLGet:
    mfspr  r3, 268
    bclr   20, 0            /* Return to caller */

.globl sda_init
sda_init:
    lis     r2, HI(_SDA2_BASE_)
    ori     r2, r2, LO(_SDA2_BASE_)
    lis     r13, HI(_SDA_BASE_)
    ori r13, r13, LO(_SDA_BASE_)
    blr

#define ENTRY_SIZE      64
    /*
     * setup the entry
     * r10 has the base address of the spin table.
     * spin table is defined as
     * struct {
     *  uint64_t entry_addr;
     *  uint64_t r3;
     *  u32_t rsvd1;
     *  u32_t pir;
     * };
     * we pad this struct to 64 bytes so each entry is in its own cacheline
     */

    .align 6
    .globl __spin_table
__spin_table:
    .space CONFIG_MAX_CPUS*ENTRY_SIZE

__spin_table_end:
    .space 4096 - (__spin_table_end - __spin_table)

/* resetEntry - rom entry point */

    .section .boot, "ax", @progbits
    .global _resetEntry
resetEntry:
_resetEntry:
    xor     r6, r6, r6
    msync
    isync
    mtspr   L1CSR0, r6      /* Invalidate the Data cache */
    li  r6, 0x0000
    msync
    isync
    mtspr   L1CSR0, r6      /* Disable the Data cache */
    li  r6, 0x0002
    msync
    isync
    mtspr   L1CSR1, r6      /* Disable the Instrunction cache */
    li  r6, 0x0002
    msync
    isync
    mtspr   L1CSR1, r6      /* Invalidate the Instruction cache */
    isync
    li  r6, 0x0000
    msync
    isync
    mtspr   L1CSR1, r6      /* temp disabled Enable the Instruction cache*/

    isync
    li  r7, 0x0000
    msync
    isync
    mtspr   L1CSR1, r7      /* enable the instruction cache */

    msync
    isync

    /* Clear SRR0, CSRR0, MCSRR0, SRR1, CSRR1 , MCSRR1, IVPR */

    xor     r0,r0,r0

    mtspr   SRR0, r0
    mtspr   SRR1, r0
    mtspr   CSRR0, r0
    mtspr   CSRR1, r0
    mtspr   MCSRR0, r0
    mtspr   MCSRR1, r0
    mtspr   ESR, r0
    mtspr   MCSR, r0
    mtspr   DEAR, r0
    mtspr   DBCR0, r0
    mtspr   DBCR1, r0
    mtspr   DBCR2, r0
    mtspr   IAC1, r0
    mtspr   IAC2, r0
    mtspr   DAC1, r0
    mtspr   DAC2, r0

    mfspr   r1, DBSR
    mtspr   DBSR, r1

    mtspr   PID0, r0
    mtspr   PID1, r0
    mtspr   PID2, r0
    mtspr   TCR, r0
    mtspr   1013, r0

    mtspr   MAS4, r0
    mtspr   MAS6, r0
    isync

    lis     r1,0xfff0
    ori     r1,r1,0x0000
    mtspr   IVPR, r1

    /* Set up vector offsets */

    addi    r3, r0, 0x100
    mtspr   IVOR0, r3
    addi    r3, r0, 0x200
    mtspr   IVOR1, r3
    addi    r3, r0, 0x300
    mtspr   IVOR2, r3
    addi    r3, r0, 0x400
    mtspr   IVOR3, r3
    addi    r3, r0, 0x500
    mtspr   IVOR4, r3
    addi    r3, r0, 0x600
    mtspr   IVOR5, r3
    addi    r3, r0, 0x700
    mtspr   IVOR6, r3
    /* skipping IVOR7 0x800: no FPU on e500 */
    addi    r3, r0, 0x900
    mtspr IVOR8, r3
    /* skipping IVOR9 0xa00: no aux processor on e500 */
    addi    r3, r0, 0xb00
    mtspr   IVOR10, r3
    addi    r3, r0, 0xc00
    mtspr   IVOR11, r3
    addi    r3, r0, 0xd00
    mtspr   IVOR12, r3
    addi    r3, r0, 0xe00
    mtspr   IVOR13, r3
    addi    r3, r0, 0xf00
    mtspr   IVOR14, r3
    addi    r3, r0, 0x1000
    mtspr   IVOR15, r3
    addi    r3, r0, 0x1400       /* perf mon is e500 specific */
    mtspr   IVOR35, r3

    li      r2, 0x06         /* L2TLB0_FI | L2TLB1_FI */
    mtspr   MMUCSR0, r2
    isync

    li      r3, 4
    li      r4, 0
    tlbivax r4, r3
    nop

    /*
     * Write TLB entry for initial program memory page
     *
     * - Specify EPN, RPN, and TSIZE as appropriate for system
     * - Set valid bit
     * - Specify TID=0
     * - Specify TS=0 or else MSR[IS,DS] must be set to correspond to TS=1
     * - Specify storage attributes (W, I, M, G, E, U0 - U3) as appropriate
     * - Enable supervisor mode fetch, read, and write access (SX, SR, SW)
     */


    /*
     * TLB1 #0. Flash space: 0xFFFFF000 -> 0xFFFFFFFF (4kB),
     * after reset, the last 4KB is mapped to flash, we
     * do not change this here
     */

    /* TLB1 #0. Flash space: 0xFF000000 -> 0xFFFFFFFF (16MB) */

    addis   r4, 0, 0x1000               /* TLB1 entry#0 */
    ori     r4, r4, 0x0000
    mtspr   MAS0, r4
    addis   r5, 0, HI(MMU_STATE_VALID|MMU_STATE_IPROT)
    ori     r5, r5, _MMU_TLB_SZ_16M     /* TS = 0, TSIZE = 16 MByte */
    mtspr   MAS1, r5
    addis   r6, 0, HI(FLASH_BASE_ADRS)  /* EPN */
    ori     r6, r6, MMU_ROM_ACCESS
    mtspr   MAS2, r6
    addis   r7, 0, HI(FLASH_BASE_ADRS)  /* RPN */
    ori     r7, r7, 0x0015              /* Supervisor XWR */
    mtspr   MAS3, r7
    tlbwe
    tlbsync

    /*
     * TLB1 #1. Memory space: 0x0000000 -> 0x3FFFFFFF (1GB)
     * Attributes: UX/UW/UR/SX/SW/SR
     */

    addis   4, 0, 0x1001               /* TLB1 entry#1 */
    ori     4, 4, 0x0000
    mtspr   MAS0, 4
    addis   r5, 0, HI(MMU_STATE_VALID|MMU_STATE_IPROT)
    ori     r5, r5, _MMU_TLB_SZ_1G      /* TS = 0, TSIZE = 1G Byte */
    mtspr   MAS1, r5
    addis   r6, 0, HI(0)   /* EPN: 0 */
    ori     r6, r6, 0x000a              /* WIMGE = 01010 */
    mtspr   MAS2, r6
    addis   7, 0, HI(0)   /* RPN: 0 */
    ori     7, 7, 0x003f              /* Supervisor XWR*/
    mtspr   MAS3, 7
    tlbwe
    tlbsync

    /* TLB1 #2: CCSR space 0xFFE00000 -> 0xFFEFFFFF (1MB) */

    addis   4, 0,  0x1002              /* TLBSEL = TLB1(CAM) , ESEL = 3*/
    ori     4, 4, 0x0000
    mtspr   MAS0,   4
    addis   r5, 0,  HI(MMU_STATE_VALID|MMU_STATE_IPROT)
    ori     r5, r5, _MMU_TLB_SZ_1M      /* TS = 0, TSIZE = 1 MByte page size*/
    mtspr   MAS1,   r5
    addis   r6, 0,  HI(CCSBAR)          /* EPN = CCSBAR */
    ori     r6, r6, 0x000a              /* WIMGE = 01010 */
    mtspr   MAS2,   r6
    addis   7, 0,  HI(CCSBAR)          /* RPN = CCSBAR */
    ori     7, 7, 0x003f              /* Supervisor XWR*/
    mtspr   MAS3,   7
    isync
    msync
    tlbwe
    tlbsync

    /* TLB1 #3: cpld space 0xF4000000 -> 0xF40FFFFF (1MB) */

    addis   4, 0,  0x1003              /* TLBSEL = TLB1(CAM) , ESEL = 3*/
    ori     4, 4, 0x0000
    mtspr   MAS0,   4
    addis   r5, 0,  HI(MMU_STATE_VALID)
    ori     r5, r5, _MMU_TLB_SZ_1M      /* TS = 0, TSIZE = 1 MByte page size*/
    mtspr   MAS1,   r5
    addis   r6, 0,  HI(CPLD_BASE)          /* EPN = CCSBAR */
    ori     r6, r6, 0x000a              /* WIMGE = 01010 */
    mtspr   MAS2,   r6
    addis   7, 0,  HI(CPLD_BASE)          /* RPN = CCSBAR */
    ori     7, 7, 0x003f              /* Supervisor XWR*/
    mtspr   MAS3,   7
    isync
    msync
    tlbwe
    tlbsync

    /* TLB1 #6: VSC7385 space 0xF100000 -> 0xF1FFFFF (1MB) */

    addis   r4, 0,  0x1006              /* TLBSEL = TLB1(CAM) , ESEL = 3*/
    ori     r4, r4, 0x0000
    mtspr   MAS0,   r4
    addis   r5, 0,  HI(MMU_STATE_VALID|MMU_STATE_IPROT)
    ori     r5, r5, _MMU_TLB_SZ_1M      /* TS = 0, TSIZE = 1 MByte page size*/
    mtspr   MAS1,   r5
    addis   r6, 0,  HI(VSC7385_BASE)          /* EPN = CCSBAR */
    ori     r6, r6, 0x000a              /* WIMGE = 01010 */
    mtspr   MAS2,   r6
    addis   r7, 0,  HI(VSC7385_BASE)          /* RPN = CCSBAR */
    ori     r7, r7, 0x003f              /* Supervisor XWR*/
    mtspr   MAS3,   r7
    isync
    msync
    tlbwe
    tlbsync

    mfspr  r6, HID1
    andi.  r6, r6, 1
    cmpwi  r6, 1
    bne    doCcsrbar

waitForStartSet:

    lis r4,HI(__spin_table)
    ori r4, r4, LO(__spin_table)
    
    slwi r5,r6,8
    add r4, r4, r5
    
    li r3, 0
    stw r3, 0(r4)
    stw r3, 4(r4)
    
    stw r3, 8(r4)
    stw r3, 12(r4)
    
    stw r3, 16(r4)
    
    mfspr r3, PIR
    stw r3, 20(r4)
    
    li r3, 1
    stw r3, 4(r4)
    msync

1:
    lwz r5, 4(r4)
    andi. r5, r5, 1
    bne 1b

    xor r3, r3, r3
    mttbl  r3
    mttbu  r3
    ori     r3, r3, LO(_PPC_HID0_TBEN)       /* enable Timebase */
    mtspr   HID0, r3

    lwz r3, 4(r4)
    mtctr r3
    lwz r5, 20(r4)
    mtspr PIR, r5

    lwz r3, 12(r4)    
    xor r4,r4,r4
    xor r5,r5,r5
    xor r6,r6,r6

    lis r7,HI(LOCAL_MEM_SIZE)
    ori r7, r7, LO(LOCAL_MEM_SIZE)
    
    xor r8,r8,r8
    xor r9,r9,r9
    
    bctr

doCcsrbar:

    /* Setup the memory mapped register address */

    isync
    sync

    lis     r6, HI(CCSBAR)
    ori     r6, r6, LO(CCSBAR)
    isync

    srwi    r5, r6, 12
    lis     r7, HI(CCSBAR_RESET)
    ori     r7, r7, LO(CCSBAR_RESET)
    lwz     r4, 0(r7)

    isync
    sync
    mbar 

    stw     r5, 0(r7)

    sync
    isync
    mbar

    /* Perform a load of flash address */

    lis     r5, 0xffff
    ori     r5, r5,0xfffc
    lwz     r4, 0(r5)
    isync

    /* read new CCSRBAR value */

ccsrbarWait:
    lis     r5, HI(CCSBAR)
    ori     r5, r5, LO(CCSBAR)
    lwz     r4, 0(r5)
    srwi    r5, r5, 12
    cmpw    r4, r5 
    bne     ccsrbarWait
    isync

    /* Local access window 0 (Nor Flash) initialize */

    WRITEADR(r6, 7, M85XX_LAWBAR1(CCSBAR), 0x000ff000)
    WRITEADR(r6, 7, M85XX_LAWAR1(CCSBAR), 0x80400018)

    WRITEADR(r6, 7, M85XX_BR3(CCSBAR), 0xff001001)
    WRITEADR(r6, 7, M85XX_OR3(CCSBAR), 0xff000ff7)

    isync
    sync
    mbar

    /* VSC7385 interface initialize */

    WRITEADR(r6, 7, M85XX_LAWBAR5(CCSBAR), 0x000f1000)
    WRITEADR(r6, 7, M85XX_LAWAR5(CCSBAR), 0x80400010)

    WRITEADR(r6, 7, M85XX_BR2(CCSBAR), 0xf1000801)
    WRITEADR(r6, 7, M85XX_OR2(CCSBAR), 0xfffe09ff)

    isync
    sync
    mbar

    isync
    sync
    mbar

    /* Memory mapped region base address */

    WRITEADR(r6, 7, M85XX_LAWBAR11(CCSBAR), 0x00000000)
    WRITEADR(r6, 7, M85XX_LAWAR11(CCSBAR), 0x80f0001d)

    isync

    lis     7, HI(M85XX_PORPLLSR(CCSBAR))
    ori     7, 7, LO(M85XX_PORPLLSR(CCSBAR))
    lwz     7, 0(7)
    andi.   7, 7, 0x3e
    srwi    7, 7, 1

    /* Get multiplier from table */

    lis     r8, HI(0xFFF00000)
    ori     r8, r8, LO(platpllTable)
    add     r8, r8, 7
    lbz     r8, 0(r8)

    cmpwi   r8,0                /* Test for unsupported freq */
    beq     checkStop           /* Jump to 0 */

    /* DDR Controller Initialization */

    lis r6, HI(DDRBA)
    ori r6, r6, LO(DDRBA)       /* r6 = DDR base */

#   ifdef INCLUDE_DDR_ECC
    WRITEOFFSET(r6, 7, (DDR_SDRAM_CFG),        0x63000000)
#   else  /* INCLUDE_DDR_ECC */
    WRITEOFFSET(r6, 7, (DDR_SDRAM_CFG),        0x43000000)
#   endif  /* INCLUDE_DDR_ECC */

    WRITEOFFSET(r6, 7, (CS0_BNDS),             0x0000003F)
    WRITEOFFSET(r6, 7, (CS0_CONFIG),           0x80014202)
    WRITEOFFSET(r6, 7, (CS0_CONFIG_2),         0x00000000)
    WRITEOFFSET(r6, 7, (TIMING_CFG_3),         0x00030000)
    WRITEOFFSET(r6, 7, (TIMING_CFG_0),         0x55770802)
    WRITEOFFSET(r6, 7, (TIMING_CFG_1),         0x5f599543)
    WRITEOFFSET(r6, 7, (TIMING_CFG_2),         0x0fa074d1)
    WRITEOFFSET(r6, 7, (TIMING_CFG_4),         0x00000000)
    WRITEOFFSET(r6, 7, (TIMING_CFG_5),         0x00000000)
    WRITEOFFSET(r6, 7, (DDR_SDRAM_CFG_2),      0x24401000)
    WRITEOFFSET(r6, 7, (DDR_SDRAM_MODE_CFG),   0x00040852)
    WRITEOFFSET(r6, 7, (DDR_SDRAM_MODE_CFG_2), 0x00000000)
    WRITEOFFSET(r6, 7, (DDR_SDRAM_MD_CNTL),    0x00000000)
    WRITEOFFSET(r6, 7, (DDR_SDRAM_INTERVAL),   0x0a280100)
    WRITEOFFSET(r6, 7, (DDR_DATA_INIT),        0xdeadbeef)
    WRITEOFFSET(r6, 7, (DDR_SDRAM_CLK_CTRL),   0x03000000)
    WRITEOFFSET(r6, 7, (DDR_DDR_ZQ_CNTL),      0x00000000)
    WRITEOFFSET(r6, 7, (DDR_DDR_WRLVL_CNTL),   0x00000000)
    WRITEOFFSET(r6, 7, (ERR_INT_EN),           0x00000000)
    WRITEOFFSET(r6, 7, (ERR_SBE),              0x00000000)
    WRITEOFFSET(r6, 7, (DDR_DDRCDR_1),         0x00000000)
    WRITEOFFSET(r6, 7, (DDR_DDRCDR_2),         0x00000000)

    /* wait before enable */

    addi    r5, 0, 0x0fff
    mtspr   CTR,r5
ddrLoop_1:
    bc      16, 0, ddrLoop_1

#   ifdef INCLUDE_DDR_ECC
    WRITEOFFSET(r6, 7, (DDR_SDRAM_CFG),        0xe3000000)
#   else  /* INCLUDE_DDR_ECC */
    WRITEOFFSET(r6, 7, (DDR_SDRAM_CFG),        0xc3000000)
#   endif  /* INCLUDE_DDR_ECC */

    /* wait for DRAM data initialization */

    addi    r5, 0, 0x0fff
    mtspr   CTR,r5
ddrLoop_2:
    bc      16, 0, ddrLoop_2

    /* Memory mapped region base address */

    isync

    lis     7, HI(M85XX_PORPLLSR(CCSBAR))
    ori     7, 7, LO(M85XX_PORPLLSR(CCSBAR))
    lwz     7, 0(7)
    andi.   7, 7, 0x3e
    srwi    7, 7, 1

    /* Get multiplier from table */

    lis     r8, HI(ROM_TEXT_ADRS)
    ori     r8, r8, LO(platpllTable)
    add     r8, r8, 7
    lbz     r8, 0(r8)

    cmpwi   r8, 0 /* Test for unsupported freq */

    beq     checkStop  /* Jump to 0 */

    /* Now that memory is stable we reset TLB entries for standard operation */

    /*
     * TLB1 #1.  Main SDRAM - Not Cached
     *           LOCAL_MEM_LOCAL_ADRS -> LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE
     * Attributes: UX/UW/UR/SX/SW/SR
     *
     */

    addis   4, 0, 0x1001               /* TLB1 entry#1 */
    ori     4, 4, 0x0000
    mtspr   MAS0, 4
    addis   r5, 0, HI(MMU_STATE_VALID|MMU_STATE_IPROT)
    ori     r5, r5, _MMU_TLB_SZ_1G      /* TS = 0, TSIZE = 1 GByte */
    mtspr   MAS1, r5
    addis   r6, 0, HI(0) /* EPN */
    ori     r6, r6, 0x0000              /* WIMGE = 00000 */
    mtspr   MAS2, r6
    addis   7, 0, HI(0) /* RPN */
    ori     7, 7, 0x003f              /* Supervisor XWR*/
    mtspr   MAS3, 7
    tlbwe
    tlbsync

    /* TLB1 #2: CCSR space 0xFFE00000 -> 0xFFEFFFFF (1MB) */

    addis   4, 0,  0x1002              /* TLBSEL = TLB1(CAM) , ESEL = 3*/
    ori     4, 4, 0x0000
    mtspr   MAS0,   4
    addis   r5, 0,  HI(MMU_STATE_VALID|MMU_STATE_IPROT)
    ori     r5, r5, _MMU_TLB_SZ_1M      /* TS = 0, TSIZE = 1 MByte page size*/
    mtspr   MAS1,   r5
    addis   r6, 0,  HI(CCSBAR)          /* EPN = CCSBAR */
    ori     r6, r6, 0x000a              /* WIMGE = 01010 */
    mtspr   MAS2,   r6
    addis   7, 0,  HI(CCSBAR)          /* RPN = CCSBAR */
    ori     7, 7, 0x003f              /* Supervisor XWR*/
    mtspr   MAS3,   7
    isync
    msync
    tlbwe
    tlbsync

     /* Initialize the DUART Controller */

#if 0
    lis r6, HI(UART_ADDR + CCSBAR)
    ori r6, r6, LO(UART_ADDR + CCSBAR)  /* r6 = DUART0  Controller base */            
    WRITEOFFSET_BYTE(r6,r7,(UIER), 0x0)              /* ier */
    WRITEOFFSET_BYTE(r6,r7,(ULCR), (UART_LCR_BKSE|UART_LCR_8N1))    /* lcr */   
    WRITEOFFSET_BYTE(r6,r7,(UDLB), 0)                    /* dll */
    WRITEOFFSET_BYTE(r6,r7,(UDMB), 0)       /* dlm */
    WRITEOFFSET_BYTE(r6,r7,(ULCR), UART_LCR_8N1)    /* lcr */
    WRITEOFFSET_BYTE(r6,r7,(UMCR), UART_MCRVAL)    /* mcr */
    WRITEOFFSET_BYTE(r6,r7,(UFCR), UART_FCRVAL)   /* fcr */        
    WRITEOFFSET_BYTE(r6,r7,(ULCR), (UART_LCR_BKSE |UART_LCR_8N1))    /* lcr */
    WRITEOFFSET_BYTE(r6,r7,(UDLB), (VAL & 0xff))     /* dll */
    WRITEOFFSET_BYTE(r6,r7,(UDMB), (VAL >> 8) & 0xff )   /* dlm */                 
    WRITEOFFSET_BYTE(r6,r7,(ULCR), UART_LCR_8N1)     /* lcr */     
    
    WRITE_BYTE(r4, r5, (CCSBAR+UART_ADDR), 0x75) /* output 'u'  */
    WRITE_BYTE(r4, r5, (CCSBAR+UART_ADDR), 0x61) /* output 'a'  */
    WRITE_BYTE(r4, r5, (CCSBAR+UART_ADDR), 0x72) /* output 'r'  */
    WRITE_BYTE(r4, r5, (CCSBAR+UART_ADDR), 0x74) /* output 't'  */
    WRITE_BYTE(r4, r5, (CCSBAR+UART_ADDR), 0xa)  /* output '\r' */
    WRITE_BYTE(r4, r5, (CCSBAR+UART_ADDR), 0xd)  /* output '\n' */        
#endif


    /* jump out of the last 4KB (resetEntry) */

    lis     r6, HI(_romInit)
    ori     r6, r6, LO(_romInit)
    lis     7, HI(cold)
    ori     7, 7, LO(cold)
    lis     r8, HI(ROM_TEXT_ADRS)
    ori     r8, r8, LO(ROM_TEXT_ADRS)
    sub     r6, 7, r6              /* routine - entry point */
    add     r6, r6, r8              /* + ROM base */
    mtspr   LR, r6
    mr      r3, r2
    blr

checkStop:
    ba 0x0

cacheTextUpdate:
    blr

    .section .reset, "ax"
resetVector:
    b   resetEntry
