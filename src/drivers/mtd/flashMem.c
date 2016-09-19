/* flashMem.c - flash memory device driver */

/* includes */

#include <wrboot.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define FLASH_SECTOR_SIZE  0x20000

#define _WRS_CONFIG_FLASH_BASE_ADRS 0xff000000

#undef static
#define static  

/*******************************************************************************
*
* sdelay - rough delay
*
* This routine performs a rough sdelay
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

static void sdelay(int x)
    {
    volatile int y = x;
    while (y--);
    }

/*******************************************************************************
*
* s29glWrite - write data to flash memory using single word program algorithm 
*
* This routine writes data to flash memory using single word program algorithm 
*
* RETURNS: 0 on success or -1 otherwise
*
* ERRNO: N/A
*
* \NOMANUAL
*/

static int s29glWrite
    (
    uint16_t *sector,
    const void *data,
    size_t len
    )
    {
    int i;
    const uint16_t *d = data;
    uint16_t *addr = (uint16_t *)_WRS_CONFIG_FLASH_BASE_ADRS;

    if ((uintptr_t)data & 0x1)
        {
        (void)printf("s29gl_write: data@%p is not aligned to 16bit boundary\n");
        return -1;
        }

    for (i = 0; i < len / sizeof(uint16_t); i++, sector++)
        {
        writew(addr + 0x555, 0x00AA); /* write unlock cycle 1 */
        writew(addr + 0x2AA, 0x0055); /* write unlock cycle 2 */
        writew(addr + 0x555, 0x00A0); /* write program setup command */
        writew(sector, d[i]);         /* write data to be programmed */

        sdelay(1000);

        while (((readw(sector) & 0x80) >> 7) != ((d[i] & 0x80) >> 7))
             ;

        if (i % 512 == 0)
            (void)printf(".");
        }
    (void)printf("\n");

    if (len & 0x1)
        {
        const uint8_t *u8 = data;
        uint16_t dd = u8[len - 1] & 0xff;

        writew(addr + 0x555, 0x00AA); /* write unlock cycle 1 */
        writew(addr + 0x2AA, 0x0055); /* write unlock cycle 2 */
        writew(addr + 0x555, 0x00A0); /* write program setup command */
        writew(sector, dd);         /* write data to be programmed */

        sdelay(1000);

        while (((readw(sector) & 0x80) >> 7) != ((dd & 0x80) >> 7))
             ;
        }

    writew(addr + 0x555, 0x00AA); /* write unlock cycle 1 */
    writew(addr + 0x2AA, 0x0055); /* write unlock cycle 2 */
    writew(addr + 0x555, 0x00F0); /* read reset command */

    return 0;
    }

/*******************************************************************************
*
* s29glSoftwareReset - perform software reset
*
* This routine performs software reset
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

static void s29glSoftwareReset(void)
    {
    uint16_t *addr = (uint16_t *)_WRS_CONFIG_FLASH_BASE_ADRS;
    writew(addr, 0x00F0); /* write unlock cycle 1 */
    }

/*******************************************************************************
*
* s29glSectorErase - erase one sector
*
* This routine erases one sector, whose address is specifid by 'sector'
*
* RETURNS: 0 on success or -1 otherwise
*
* ERRNO: N/A
*
* \NOMANUAL
*/

static int s29glSectorErase
    (
    uint16_t *sector
    )
    {
    uint16_t  stat;
    uint16_t *addr = (uint16_t *)_WRS_CONFIG_FLASH_BASE_ADRS;

    writew(addr + 0x555, 0x00AA); /* write unlock cycle 1 */
    writew(addr + 0x2AA, 0x0055); /* write unlock cycle 2 */
    writew(addr + 0x555, 0x0080); /* write setup command */
    writew(addr + 0x555, 0x00AA); /* write additional unlock cycle 1 */
    writew(addr + 0x2AA, 0x0055); /* write additional unlock cycle 2 */
    writew(sector, 0x0030);       /* write sector erase command */

    for (;;)
        {
        stat = readw(sector);
        if (stat & 0x80)
            break;
        if (stat & 0x20)
            {
            (void)printf("timeout\n");
            s29glSoftwareReset();
            return -1;
            }
        }

    writew(addr + 0x555, 0x00F0); /* read array command */
    return 0;
    }

/*******************************************************************************
*
* s29glReadid - read device id
*
* This routine reads the flash device id 
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void readid1(void)
    {
    uint16_t manuf_id = 0;
    uint16_t *addr = (uint16_t *)0xff000000;

    /* Auto Select Entry */
printf("addr:0x%x\n",addr);

writew((addr + 0x555), 0x00AA); /* write unlock cycle 1 */


writew(addr + 0x2AA, 0x0055); /* write unlock cycle 2 */

writew(addr + 0x555, 0x0090); /* write autoselect command */

sdelay(10000);

/* multiple reads can be performed after entry */

    manuf_id = readw(addr + 0x000); /* read manuf. id */
(void)printf("manufacturer id = 0x%x\n", manuf_id);

	manuf_id = readw(addr + 0x001); /* read manuf. id */
(void)printf("device id 1 = 0x%x\n", manuf_id);

    manuf_id = readw(addr + 0x00E); /* read manuf. id */
(void)printf("device id 2 = 0x%x\n", manuf_id);

    manuf_id = readw(addr + 0x00F); /* read manuf. id */
(void)printf("device id 3 = 0x%x\n", manuf_id);

    /* Autoselect exit */

    writew(addr + 0x000, 0x00F0); /* exit autoselect (write reset command) */
	}

void readid(void)
    {
    uint16_t manuf_id = 0;
printf("%s:%s:%d\n",__FILE__,__func__,__LINE__);
    uint16_t *addr = (uint16_t *)0xff000000;

    /* Auto Select Entry */
/*
 * 这里一执行就挂住的原因是说明flash已经进入编程模式了，但是因为现在指令都在flash上，进入编程模式以后下一条指令无法获取了，所以系统会跑飞,唯一的办法就是把flash上的指令全部copy到DDR
 * */
*(volatile unsigned short *)(addr + 0x555) = 0xaa;

*(volatile unsigned short *)(addr + 0x2AA) = 0x55;

*(volatile unsigned short *)(addr + 0x555) = 0x90;

    sdelay(10000);
printf("0xaa => (u16 *)0xff000555\n");
printf("0x55 => (u16 *)0xff0002aa\n");
printf("0x90 => (u16 *)0xff000555\n");

    /* multiple reads can be performed after entry */

manuf_id = *(volatile unsigned short *)(addr + 0x000);
(void)printf("manufacturer id = 0x%x\n", manuf_id);
manuf_id = *(volatile unsigned short *)(addr + 0x001);
(void)printf("device id 1 = 0x%x\n", manuf_id);
manuf_id = *(volatile unsigned short *)(addr + 0x00E);
(void)printf("device id 2 = 0x%x\n", manuf_id);
manuf_id = *(volatile unsigned short *)(addr + 0x00F);
(void)printf("device id 3 = 0x%x\n", manuf_id);

    /* Autoselect exit */

    //writew(addr + 0x000, 0x00F0); /* exit autoselect (write reset command) */
*(volatile unsigned short *)(addr + 0x000) = 0xf0;

        }


/*******************************************************************************
*
* flashBlockRead - read data from flash
*
* This routine reads data from flash
*
* RETURNS: 0 on OK, -1 otherwise
*
* ERRNO: N/A
*
* \NOMANUAL
*/
static int flashBlockRead
    (
    struct blkdev *dev,
    void *buf,
    uint32_t sector,
    uint32_t sectnum
    )
    {
    memcpy(buf, (char *)sector, sectnum * FLASH_SECTOR_SIZE);
    return 0;
    }

/*******************************************************************************
*
* flashBlockWrite - write data to flash
*
* This routine writes data to flash
*
* RETURNS: 0 on OK, -1 otherwise
*
* ERRNO: N/A
*
* \NOMANUAL
*/

static int flashBlockWrite
    (
    struct blkdev *dev,
    const void *buf,
    uint32_t sector,
    uint32_t sectnum
    )
    {
    int ret = 0;

    ret = s29glSectorErase((uint16_t *)sector);
    if (ret != 0)
        goto error;

    ret = s29glWrite((uint16_t *)sector,
            buf, sectnum * FLASH_SECTOR_SIZE);
    if (ret != 0)
        goto error;
    return 0;

error:
    return ret;
    }

/*******************************************************************************
*
* flashBlkDevInit - register and initialize flash device
*
* This routine register and initialize flash device
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

#if 0
static void flashBlkDevInit(void)
    {
    int i;
    static struct partition p2020flash_parts[]  =
        {
            {
            .name = "VSC7385",
            .start = 0x0,
            .nrblk = 1024 * 1024 * 1
            },

            {
            .name = "uVxWorks",
            .start = 0x100000,
            .nrblk = 1024 * 1024 * 4
            },

            {
            .name = "p2020rdb.dtb",
            .start = 0x500000,
            .nrblk = 16 * 1024
            },

            {
            .name = "nvram",
            .start = 0x540000,
            .nrblk = FLASH_SECTOR_SIZE
            },

            {
            .name = "vxboot.bin",
            .start = 0x600000,
            .nrblk = 1024 * 1024 * 2
            },
        };

    static struct blkdev_ops flash_ops =
        {
        .read = flashBlockRead,
        .write = flashBlockWrite
        };
    static struct blkdev flash_blkdev =
        {
        .name = "flash0",
        .ops = &flash_ops,
        .blksize = FLASH_SECTOR_SIZE,
        .part = p2020flash_parts,
        .partnum = ARRAY_SIZE(p2020flash_parts)
        };

    for (i = 0; i < ARRAY_SIZE(p2020flash_parts); i++)
        {
        /* we limit flash to only 8M to avoid destroying u-boot */
        p2020flash_parts[i].start += (FLASH_ADRS + SZ_8M);
        p2020flash_parts[i].nrblk =
            ROUND_UP(p2020flash_parts[i].nrblk, FLASH_SECTOR_SIZE) / FLASH_SECTOR_SIZE;
        }

    (void)blkdevRegister(&flash_blkdev);
    }
INIT_BEFORE_FS(flashBlkDevInit)
#endif
