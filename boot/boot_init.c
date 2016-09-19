#include "ns16550.h"
#include <string.h>
#include <stdio.h>

#define MAX_CMDBUF_SIZE         256

/* extern */

extern void sda_init(void);
extern int boot (unsigned char * imageAddr, unsigned char * dtbAddr);
extern int cmd_parse(const char * inputLine);

extern void serial_init(NS16550_t console, unsigned int);
extern void banner(void);
extern int heap_init(void);
extern void getcmd(char *);
extern void sym_table_init(void);
extern void readid(void);
extern void sysMiscInit(void);
extern unsigned char     binArrayStart [];   /* compressed binary image */
extern unsigned char     binArrayEnd [];     /* end of compressed binary image */
extern char etext [];       /* defined by the loader */
extern char end [];         /* defined by the loader */
extern unsigned char     wrs_kernel_data_start [];  /* defined by the loader */
extern unsigned char     wrs_kernel_data_end [];    /* defined by the loader */
extern char     wrs_kernel_text_start [];  /* defined by the loader */
extern void _romInit(void);

#define CFG_NS16550_CLK        0x299999600UL
#define CCSBAR                  0xF3000000

const static NS16550_t console = (NS16550_t) (CCSBAR + 0x4500);

void mainboot(void)
    {
    char cmd_buf[MAX_CMDBUF_SIZE];
    unsigned long addr;             /* Address of image            */

    /* ns16550 init */

    serial_init(console, (unsigned int)CFG_NS16550_CLK);

     printf("copy bootloader from(ROM)-0x%x to(RAM)-0x%x size-0x%x\n", \
        0xfff00000, wrs_kernel_text_start,end-wrs_kernel_text_start);

    sysMiscInit();

#ifdef ROM_RESIDNET
    /* If ROM resident code, then copy only data segment from ROM to RAM */

    printf("ROM resident, copy data segment from(ROM)-0x%x to(RAM)-0x%x size-0x%x\n", \
        etext, wrs_kernel_data_start,wrs_kernel_data_end-wrs_kernel_data_start);

    memcpy((void*)(wrs_kernel_data_start), 
            (void* )etext,
            (unsigned int)(wrs_kernel_data_end-wrs_kernel_data_start));
#endif
    banner();

    /* dynamic memory heap init */

    heap_init();

    /* shell - main loop */

    sym_table_init();

    /**/

    readid();
    //mtd_dev_init();
    cfi_probe_nor_flash();

fat();

    for (;;)
        {
        memset(cmd_buf, 0, MAX_CMDBUF_SIZE);

        (void)printf("p2020rdb # ");

        /* get inputs */

        getcmd(cmd_buf);

        /* execute the inputs */

        switch (cmd_buf[0])
            {
            case '@':
                boot((unsigned char *)0x2000000, (unsigned char *)0xf000000);
                break;
            case 'g':
                boot((unsigned char *)0x100000, (unsigned char *)0);
                break;
            case 'r':
                addr = 0x100000;
                ((void (*)(void)) addr) ();
                break;
            default:
                break;
            }

        cmd_parse(cmd_buf);
        }
}

void bootInit(void)
    {

    /* 
     * For PPC, the call to sda_init() must be the first operation in romStart.
     * This is because sda_init() sets the r2 and r13 registers to the 
     * SDA and SDA2 base registers. No C code must be placed before this
     * call.
     */

     __asm__ volatile ("");   /* code barrier to prevent compiler moving sda_init() */
    sda_init ();    /* this MUST be the first operation in usrInit() for PPC */
     __asm__ volatile ("");   /* code barrier to prevent compiler moving sda_init() */

    /*  copy bootloader all segment from ROM to RAM */
     void *(*__func_memcpy)(void *, const void *, int) = (void *)memcpy - (void *)_romInit + 0xfff00000;

    __func_memcpy((void*)(wrs_kernel_text_start), (void* )0xfff00000, 0x100000);

    void (*__func_boot)(void) = (void (*)(void)) mainboot;
    __func_boot();

    }
