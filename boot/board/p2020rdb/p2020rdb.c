/*************************************************************************
    > File Name: p2020rdb.c
    > Author: YangYe
    > Mail: goodwillyang@163.com 
    > Created Time: 2016年08月05日 星期五 11时14分49秒
 ************************************************************************/

#include "p2020rdb.h"
#include <wrboot.h>
#include <stdio.h>
#include <types.h>
static unsigned int   coreFreq;
static unsigned int   core1Freq;
static unsigned int   ddrFreq;
static unsigned int   systemFreq;

/* Clock Ratio Tables */

#define MAX_VALUE_DDR_RATIO     16
static unsigned int ddrRatioTable[MAX_VALUE_DDR_RATIO] = {
    0,  0,  0,  3,  4,  0, 6,  0,
    8,  0,  10, 0,  12, 0, 14, 0
};

#define MAX_VALUE_PLAT_RATIO    32
static unsigned int platRatioTable [MAX_VALUE_PLAT_RATIO][2] = {
    { 0,  0 }, { 0, 0 }, { 0,  0 }, { 3, 0 },
    { 4,  0 }, { 5, 0 }, { 6,  0 }, { 0, 0 },
    { 8,  0 }, { 9, 0 }, { 10, 0 }, { 0, 0 },
    { 12, 0 }, { 0, 0 }, { 0,  0 }, { 0, 0 },
    { 0,  0 }, { 0, 0 }, { 0,  0 }, { 0, 0 },
    { 0,  0 }, { 0, 0 }, { 0,  0 }, { 0, 0 },
    { 0,  0 }, { 0, 0 }, { 0,  0 }, { 0, 0 },
    { 0,  0 }, { 0, 0 }, { 0,  0 }, { 0, 0 }
};

#define MAX_VALUE_E500_RATIO    10
static unsigned int e500RatioTable [MAX_VALUE_PLAT_RATIO][2] = {
    { 0, 0 }, { 0, 0 }, { 1, 0 }, { 3, 1 }, { 2, 0 },
    { 5, 1 }, { 3, 0 }, { 7, 1 }, { 4, 0 }, { 9, 1 }
};

extern unsigned int sysTimeBaseLGet(void);
#if 0
extern u32 readl(void* addr);
extern uint16_t readw(void* addr);
extern uint8_t readb(void* addr);
extern void writel(void* addr, u32 val);
extern void writew(void* addr, uint16_t val);
extern void writeb(void* addr, uint8_t val);
#endif
extern void setTimeBase(u32, u32);
extern void setHid0(u32);
extern u32 getHid0(void);

/*******************************************************************************
*
* sysClkFreqGet - return the clock freq of the system bus
*
* This routine returns the clock freq of the system bus
*
* RETURNS: Clock freq of the system bus
*
* ERRNO: N/A
*
* \NOMANUAL
*/

unsigned int sysClkFreqGet (void)
    {
    unsigned int  sysClkFreq;
    unsigned int  e500Ratio;
    unsigned int  e5001Ratio;
    unsigned int  platRatio;
    unsigned int  ddrRatio;

    //return DEFAULT_SYSCLKFREQ;

    platRatio = M85XX_PORPLLSR_PLAT_RATIO(CCSBAR);
    ddrRatio = M85XX_PORPLLSR_DDR_RATIO(CCSBAR);

    if ((platRatio >= ARRAY_SIZE(platRatioTable))
        || (platRatioTable[platRatio] == 0)
       )
    return(DEFAULT_SYSCLKFREQ); /* A default value better than zero or -1 */
#if 0
    systemFreq = FREQ_100_MHZ;
#else
    systemFreq = FREQ_25_MHZ; 
#endif
    ddrFreq = DDR_CLK_FREQ;

    ddrFreq = ddrFreq * ddrRatioTable[ddrRatio];
    sysClkFreq = ((unsigned int)(systemFreq * platRatioTable[platRatio][0])) >> \
                 ((unsigned int)platRatioTable[platRatio][1]);

    e500Ratio = M85XX_PORPLLSR_E500_RATIO(CCSBAR);
    e5001Ratio = M85XX_PORPLLSR_E500_1_RATIO(CCSBAR);

    coreFreq = ((unsigned int)(sysClkFreq * e500RatioTable[e500Ratio][0])) >> \
               ((unsigned int)e500RatioTable[e500Ratio][1]);
    core1Freq = ((unsigned int)(sysClkFreq * e500RatioTable[e5001Ratio][0]))>> \
                ((unsigned int)e500RatioTable[e5001Ratio][1]);
    printf("Clock: %dM, default:%dM\n",sysClkFreq/1000000,DEFAULT_SYSCLKFREQ/1000000);
    return(sysClkFreq);
    }

/*******************************************************************************
*
* udelay - delay at least the specified amount of time (in microseconds)
*
* This routine will delay for at least the specified amount of time using the
* lower 32 bit "word" of the Time Base register as the timer.
*
* NOTE:  This routine will not relinquish the CPU; it is meant to perform a
* busy loop delay.  The minimum delay that this routine will provide is
* approximately 10 microseconds.  The maximum delay is approximately the
* size of u3232; however, there is no roll-over compensation for the total
* delay time, so it is necessary to back off two times the system tick rate
* from the maximum.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void udelay
    (
    unsigned int    delay        /* length of time in microsec to delay */
    )
    {
    register u32 baselineTickCount;
    register u32 curTickCount;
    register u32 terminalTickCount;
    register int actualRollover = 0;
    register int calcRollover = 0;
    u32 ticksToWait;
    u32 requestedDelay;
    u32 oneUsDelay;

    /* Exit if no delay count */

    if ((requestedDelay = delay) == 0)
        return;

    /*
     * Get the Time Base Lower register tick count, this will be used
     * as the baseline.
     */

    baselineTickCount = sysTimeBaseLGet();

    /*
     * Calculate number of ticks equal to 1 microsecond
     *
     * The Time Base register and the Decrementer count at the same rate:
     * once per 8 System Bus cycles.
     *
     * e.g., 199999999 cycles     1 tick      1 second            25 ticks
     *       ----------------  *  ------   *  --------         ~  --------
     *       second               8 cycles    1000000 microsec    microsec
     */

    /* add to round up before div to provide "at least" */

    oneUsDelay = (((sysClkFreqGet() >> 3) + 1000000) / 1000000);

    /* Convert delay time into ticks */

    ticksToWait = requestedDelay * oneUsDelay;

    /* Compute when to stop */

    terminalTickCount = baselineTickCount + ticksToWait;

    /* Check for expected rollover */

    if (terminalTickCount < baselineTickCount)
        {
        calcRollover = 1;
        }

    do
        {

        /*
         * Get current Time Base Lower register count.
         * The Time Base counts UP from 0 to
         * all F's.
         */

        curTickCount = sysTimeBaseLGet();

        /* Check for actual rollover */

        if (curTickCount < baselineTickCount)
            {
            actualRollover = 1;
            }

        if (((curTickCount >= terminalTickCount)
             && (actualRollover == calcRollover)) ||
            ((curTickCount < terminalTickCount)
             && (actualRollover > calcRollover)))
            {

            /* Delay time met */

            break;
            }
        }
    while (TRUE); /* breaks above when delay time is met */
    }

void msdelay(int delay)
{
    return udelay(1000*delay);
}

void delay(int delay)
{
return msdelay(1000*delay);
}

void sysMiscInit(void)
    {
    u32 v;

    /* fix-up gpio settings */

    v = readl((void *)GPDIR);
    v |=  0x060f0000;
    writel((void *)GPDIR, v);

    v = readl((void *)GPDAT);
    v &= ~0x04000000;
    v |= 0x020f0000;
    writel((void *)GPDAT, v);

    /* enable time base for udelay() */

    setTimeBase(0, 0);
  #define _PPC_HID0_TBEN  0x00004000      /* time base enable */
    setHid0(getHid0() | _PPC_HID0_TBEN);
    }
