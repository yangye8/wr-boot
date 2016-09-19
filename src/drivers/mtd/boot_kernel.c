/*
 * vivi/lib/boot-kernel.c: copy the kernel image to ram, then execute it 
 *
 */

#include "priv_data.h"
#include "command.h"
#include <types.h>
#include <stdio.h>
#include <wrboot.h>
#include <stdlib.h>
#include <string.h>

#define COMMAND_LINE_SIZE 1024
#define FLASH_UNCACHED_BASE 0x10000000  /* to mapping flash memory */

struct param_struct {
    union {
        struct {
            unsigned long page_size;            /*  0 */
            unsigned long nr_pages;             /*  4 */
            unsigned long ramdisk_size;         /*  8 */
            unsigned long flags;                /* 12 */
#define FLAG_READONLY   1
#define FLAG_RDLOAD     4
#define FLAG_RDPROMPT   8
            unsigned long rootdev;              /* 16 */
            unsigned long video_num_cols;       /* 20 */
            unsigned long video_num_rows;       /* 24 */
            unsigned long video_x;              /* 28 */
            unsigned long video_y;              /* 32 */
            unsigned long memc_control_reg;     /* 36 */
            unsigned char sounddefault;         /* 40 */
            unsigned char adfsdrives;           /* 41 */
            unsigned char bytes_per_char_h;     /* 42 */
            unsigned char bytes_per_char_v;     /* 43 */
            unsigned long pages_in_bank[4];     /* 44 */
            unsigned long pages_in_vram;        /* 60 */
            unsigned long initrd_start;         /* 64 */
            unsigned long initrd_size;          /* 68 */
            unsigned long rd_start;             /* 72 */
            unsigned long system_rev;           /* 76 */
            unsigned long system_serial_low;    /* 80 */
            unsigned long system_serial_high;   /* 84 */
            unsigned long mem_fclk_21285;       /* 88 */
        } s;
        char unused[256];
    } u1;
    union {
        char paths[8][128];
        struct {
            unsigned long magic;
            char n[1024 - sizeof(unsigned long)];
        } s;
    } u2;
    char commandline[COMMAND_LINE_SIZE];
};

#define LINUX_KERNEL_OFFSET 0x8000
#define LINUX_PARAM_OFFSET  0x100
#define LINUX_PAGE_SIZE     SZ_4K
#define LINUX_PAGE_SHIFT    12
#define LINUX_ZIMAGE_MAGIC  0x016f2818

/*-
 * Media Type: A type of storage device that contains the linux kernel
 *
 *  +----------------+-----------------------------------------+
 *  | Value(Integer) |  Type                                   |                   
 *  +----------------+-----------------------------------------+
 *  |       0        |  UNKNOWN                                |-
 *  |       1        |  RAM                                    |
 *  |       2        |  NOR Flash Memory                       |
 *  |       3        |  SMC (NAND Flash Memory) on the S3C2410 |
 *  |       4        |  SMC (NAND Flash Memory) on the S3C2440 |
 *  +----------------+-----------------------------------------+
 */
enum {
MT_UNKNOWN = 0,
MT_RAM,
MT_NOR_FLASH,
MT_SMC_S3C2410,
};


/*
 * call_linux(): execute the linux kernel
 * r0 = must contain a zero or else the kernel loops
 * r1 = architecture type
 * r2 = address to be executed
 */
#if 1
void call_linux(long a0, long a1, long a2)
{
#if 0
    cache_clean_invalidate();
    tlb_invalidate();

__asm__(
    "mov    r0, %0\n"
    "mov    r1, %1\n"
    "mov    r2, %2\n"
    "mov    ip, #0\n"
    "mcr    p15, 0, ip, c13, c0, 0\n"   /* zero PID */
    "mcr    p15, 0, ip, c7, c7, 0\n"    /* invalidate I,D caches */
    "mcr    p15, 0, ip, c7, c10, 4\n"   /* drain write buffer */
    "mcr    p15, 0, ip, c8, c7, 0\n"    /* invalidate I,D TLBs */
    "mrc    p15, 0, ip, c1, c0, 0\n"    /* get control register */
    "bic    ip, ip, #0x0001\n"      /* disable MMU */
    "mcr    p15, 0, ip, c1, c0, 0\n"    /* write control register */
    "mov    pc, r2\n"
    "nop\n"
    "nop\n"
    : /* no outpus */
    : "r" (a0), "r" (a1), "r" (a2)
    );
#endif
}
#else
#error not defined call_linux() for this architecture   
#endif

/*
 * pram_base: base address of linux paramter
 */
static void setup_linux_param(ulong param_base)
{
    struct param_struct *params = (struct param_struct *)param_base; 
    char *linux_cmd;

    printf("Setup linux parameters at 0x%08lx\n", param_base);
    memset(params, 0, sizeof(struct param_struct));

    params->u1.s.page_size = LINUX_PAGE_SIZE;
    params->u1.s.nr_pages = (DRAM_SIZE >> LINUX_PAGE_SHIFT);
#if 0
    params->u1.s.page_size = LINUX_PAGE_SIZE;
    params->u1.s.nr_pages = (dram_size >> LINUX_PAGE_SHIFT);
    params->u1.s.ramdisk_size = 0;
    params->u1.s.rootdev = rootdev;
    params->u1.s.flags = 0;

    /* TODO */
    /* If use ramdisk */
    /*
    params->u1.s.initrd_start = ?;
    params->u1.s.initrd_size = ?;
    params->u1.s.rd_start = ?;
    */

#endif

    /* set linux command line */
    linux_cmd = get_linux_cmd_line();
    if (linux_cmd == NULL) {
        printf("Wrong magic: could not found linux command line\n");
    } else {
        memcpy(params->commandline, linux_cmd, strlen(linux_cmd) + 1);
        printf("linux command line is: \"%s\"\n", linux_cmd);
    }
}

#if defined(CONFIG_S3C2410_NAND_BOOT) || defined(CONFIG_S3C2440_NAND_BOOT)
extern int nand_read_ll(unsigned char*, unsigned long, int);
#endif


/*
 * dst: destination address
 * src: source
 * size: size to copy
 * mt: type of storage device
 */
static inline int copy_kernel_img(ulong dst, const char *src, size_t size, int mt)
{
    int ret = 0;
    switch (mt) {
        case MT_RAM:    
            /* noting to do */
            break;
        case MT_NOR_FLASH:
            memcpy((char *)dst, (src + FLASH_UNCACHED_BASE), size);
            break;
        case MT_SMC_S3C2410: 
#if defined(CONFIG_S3C2410_NAND_BOOT) || defined(CONFIG_S3C2440_NAND_BOOT)
            ret = nand_read_ll((unsigned char *)dst, 
                       (unsigned long)src, (int)size);
#endif
            break;
        case MT_UNKNOWN:
        default:
            printf("Undefined media type.\n");
            return -1;
    }
    return ret;
}

static inline int media_type_is(const char *mt)
{
    if (strncmp("ram", mt, 3) == 0) {
        return MT_RAM;
    } else if (strncmp("nor", mt, 3) == 0) {
        return MT_NOR_FLASH;
    } else if (strncmp("smc", mt, 3) == 0) {
        return MT_SMC_S3C2410;
    } else {
        return MT_UNKNOWN;
    }
}

/*
 * boot_kernel: booting the linux kernel
 *
 * from: address of stored kernel image
 * size: size of kernel image
 * media_type: a type of stoage device
 */
int boot_kernel(ulong from, size_t size, int media_type)
{
    int ret;
    ulong boot_mem_base;    /* base address of bootable memory ¸Â³ª? */
    ulong to;
    ulong mach_type;

    boot_mem_base = get_param_value("boot_mem_base", &ret);
    if (ret) {
        printf("Can't get base address of bootable memory\n");
        printf("Get default DRAM address. (0x%08lx\n", DRAM_BASE);
        boot_mem_base = DRAM_BASE;
    }

    /* copy kerne image */
    to = boot_mem_base + LINUX_KERNEL_OFFSET;
    printf("Copy linux kernel from 0x%08lx to 0x%08lx, size = 0x%08lx ... ",
        from, to, size);
    ret = copy_kernel_img(to, (char *)from, size, media_type);
    if (ret) {
        printf("failed\n");
        return -1;
    } else {
        printf("done\n");
    }

    if (*(ulong *)(to + 9*4) != LINUX_ZIMAGE_MAGIC) {
        printf("Warning: this binary is not compressed linux kernel imag\n");
        printf("zImage magic = 0x%08lx\n", *(ulong *)(to + 9*4));
    } else {
        printf("zImage magic = 0x%08lx\n", *(ulong *)(to + 9*4));
    }

    /* Setup linux parameters and linux command line */
    setup_linux_param(boot_mem_base + LINUX_PARAM_OFFSET);

    /* Get machine type */
    mach_type = get_param_value("mach_type", &ret);
    printf("MACH_TYPE = %d\n", mach_type);

    /* Go Go Go */
    printf("NOW, Booting Linux......\n");
    call_linux(0, mach_type, to);

    return 0;   
}
    

/*
 * User Commands
 */

static inline void display_help(void)
{
    printf("invalid 'params' command: too few(many) arguments\n");
    printf("Usage:\n");
    printf("  boot <media_type> -- booting kernel \n");
    printf("    value of media_type (location of kernel image\n");
    printf("       1 = RAM\n");
    printf("       2 = NOR Flash Memory\n");
    printf("       3 = SMC (On S3C2410)\n");
    printf("  boot <media_type> <mtd_part> -- boot from specific mtd partition\n");
    printf("  boot <media_type> <addr> <size>\n");
    printf("  boot help -- help about 'boot' command\n");
}

/*
 * default values:
 *   kernel mtd partition = "kernel"
 *   base adress of bootable memory = DRAM_BASE
 *   media type = 
 *
 * avalable commands
 *
 * boot
 * boot <media_type>
 * boot <media_type> <mtd_part_name>
 * boot <media_type> <base address of stored kernel image> <kernel_size>
 * boot help
 *
 * Anyway, I need three values. this:
 *  media type, address of kernel image, size of kernel image,
 */

void command_boot(int argc, const char **argv)
{
    int media_type = 0;
    ulong from = 0;
    size_t size = 0;
    mtd_partition_t *kernel_part;
    int ret;

    switch (argc) {
        case 1:
            media_type = get_param_value("media_type", &ret);
            if (ret) {
                printf("Can't get default 'media_type'\n");
                return;
            }
            kernel_part = get_mtd_partition("kernel");
            if (kernel_part == NULL) {
                printf("Can't find default 'kernel' partition\n");
                return;
            }
            from = kernel_part->offset;
            size = kernel_part->size;
            break;
        case 2:
            if (strncmp("help", argv[1], 4) == 0) {
                display_help();
                return;
            }
            media_type = media_type_is(argv[1]);
            kernel_part = get_mtd_partition("kernel");
            from = kernel_part->offset;
            size = kernel_part->size;
            break;
        case 3:
            media_type = media_type_is(argv[1]);
            kernel_part = get_mtd_partition(argv[2]);
            from = kernel_part->offset;
            size = kernel_part->size;
            break;
        case 4:
            media_type = media_type_is(argv[1]);
            from = strtoul(argv[2], NULL, 0);
            size = strtoul(argv[3], NULL, 0);
            break;
        default:
            display_help();
            break;
    }

    boot_kernel(from, size, media_type);
}

user_command_t boot_cmd = {
    "boot",
    command_boot,
    NULL,
    "boot [{cmds}] \t\t\t-- Booting linux kernel"
};
