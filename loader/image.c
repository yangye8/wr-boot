#include <wrboot.h>
#include "h/image.h"
#include "h/elf_image.h"
#include "h/elf.h"
#include <libfdt_util.h>
#include <libfdt.h>
#include <string.h>
#include <stdio.h>

extern char wrs_kernel_text_start[];
extern char wrs_kernel_rom_size[];

#define EPAPR_MAGIC    (0x45504150)

#define BOOT_MAP_SIZE  0x10000000

image_header_t header;

unsigned long load_addr = 0x00100000;       /* Default Load Address */

/* powerpc kernel entry interface */

typedef void (*__func_ppcEntry)(void *, unsigned long r4, unsigned long r5, \
    unsigned long r6,  unsigned long r7, unsigned long r8, unsigned long r9);

/*
 * print sizes as "xxx kB", "xxx.y kB", "xxx MB" or "xxx.y MB" as needed;
 * allow for optional trailing string (like "\n")
 */
void print_size (unsigned long size, const char *s)
{
        unsigned long m, n;
        unsigned long d = 1 << 20;              /* 1 MB */
        char  c = 'M';

        if (size < d) {                 /* print in kB */
                c = 'k';
                d = 1 << 10;
        }

        n = size / d;

        m = (10 * (size - (n * d)) + (d / 2) ) / d;

        printf ("%d", n);
        if (m) {
                printf (".%d", m);
        }
        printf (" %cB%s", c, s);
}

static void print_type (image_header_t *hdr)
{
    char *os, *arch, *type, *comp;

    switch (hdr->ih_os) {
    case IH_OS_INVALID: os = "Invalid OS";      break;
    case IH_OS_NETBSD:  os = "NetBSD";          break;
    case IH_OS_LINUX:   os = "Linux";           break;
    case IH_OS_VXWORKS: os = "VxWorks";         break;
    case IH_OS_QNX:     os = "QNX";         break;
    case IH_OS_PPCBOOT: os = "PPCBoot";         break;
    default:        os = "Unknown OS";      break;
    }

    switch (hdr->ih_arch) {
    case IH_CPU_INVALID:    arch = "Invalid CPU";       break;
    case IH_CPU_ALPHA:  arch = "Alpha";         break;
    case IH_CPU_ARM:    arch = "ARM";           break;
    case IH_CPU_I386:   arch = "Intel x86";     break;
    case IH_CPU_IA64:   arch = "IA64";          break;
    case IH_CPU_MIPS:   arch = "MIPS";          break;
    case IH_CPU_MIPS64: arch = "MIPS 64 Bit";       break;
    case IH_CPU_PPC:    arch = "PowerPC";       break;
    case IH_CPU_S390:   arch = "IBM S390";      break;
    case IH_CPU_SH:     arch = "SuperH";        break;
    case IH_CPU_SPARC:  arch = "SPARC";         break;
    case IH_CPU_SPARC64:    arch = "SPARC 64 Bit";      break;
    default:        arch = "Unknown Architecture";  break;
    }

    switch (hdr->ih_type) {
    case IH_TYPE_INVALID:   type = "Invalid Image";     break;
    case IH_TYPE_STANDALONE:type = "Standalone Program";    break;
    case IH_TYPE_KERNEL:    type = "Kernel Image";      break;
    case IH_TYPE_RAMDISK:   type = "RAMDisk Image";     break;
    case IH_TYPE_MULTI: type = "Multi-File Image";  break;
    case IH_TYPE_FIRMWARE:  type = "Firmware";      break;
    case IH_TYPE_SCRIPT:    type = "Script";        break;
    default:        type = "Unknown Image";     break;
    }

    switch (hdr->ih_comp) {
    case IH_COMP_NONE:  comp = "uncompressed";      break;
    case IH_COMP_GZIP:  comp = "gzip compressed";   break;
    case IH_COMP_BZIP2: comp = "bzip2 compressed";  break;
    default:        comp = "unknown compression";   break;
    }

    printf ("%s %s %s (%s)", arch, os, type, comp);
}

void print_image_hdr (image_header_t *hdr)
{
    printf ("   Image Name:   %.s\n", hdr->ih_name);
    printf ("   Image Type:   "); print_type(hdr); 
    printf ("\n");
    printf ("   Data Size:    %d Bytes = ", be32_to_cpu(hdr->ih_size));
    print_size (be32_to_cpu(hdr->ih_size), "\n");
    printf ("   Load Address: 0x%08x\n", be32_to_cpu(hdr->ih_load));
    printf ("   Entry Point:  0x%08x\n", be32_to_cpu(hdr->ih_ep));

    if (hdr->ih_type == IH_TYPE_MULTI) {
        int i;
        unsigned long len;
        unsigned long *len_ptr = (unsigned long *)((unsigned long)hdr + sizeof(image_header_t));

        printf ("   Contents:\n");
        for (i=0; (len = be32_to_cpu(*len_ptr)); ++i, ++len_ptr) {
            printf ("   Image %d: %8ld Bytes = ", i, len);
            print_size (len, "\n");
        }
    }
}

static int image_info (unsigned long addr)
{
    image_header_t *hdr = &header;

    printf ("\n## Checking Image at 0x%08lx ...\n", addr);

    /* Copy header so we can blank CRC field for re-calculation */

    memmove (&header, (char *)addr, sizeof(image_header_t));

    if (be32_to_cpu(hdr->ih_magic) != IH_MAGIC) {
        printf ("   Bad Magic Number\n");
        return 1;
    }

    /* for multi-file images we need the data part, too */
    print_image_hdr ((image_header_t *)addr);

    printf ("OK\n");
    return 0;
}

/* ======================================================================
 * Determine if a valid ELF image exists at the given memory location.
 * First looks at the ELF header magic field, the makes sure that it is
 * executable and makes sure that it is for a PowerPC.
 * ====================================================================== */
int valid_elf_image (unsigned long addr)
{
        Elf32_Ehdr *ehdr;               /* Elf header structure pointer */

        /* -------------------------------------------------- */

        ehdr = (Elf32_Ehdr *) addr;

        if (IS_ELF (*ehdr))
                printf ("## Elf image at address 0x%x\n", addr);

        if (ehdr->e_type == ET_EXEC)
                        printf ("## a 32-bit elf image at address 0x%x\n",
                        addr);

        if (ehdr->e_machine == EM_PPC)
                printf ("## a PowerPC elf image at address 0x%x\n",
                        addr);
        return 1;
}

/* ======================================================================
 * A very simple elf loader, assumes the image is valid, returns the
 * entry point address.
 * ====================================================================== */
unsigned long load_elf_image (unsigned long addr)
{
        Elf32_Ehdr *ehdr;               /* Elf header structure pointer     */
        Elf32_Shdr *shdr;               /* Section header structure pointer */
        unsigned char *strtab = 0;      /* String table pointer             */
        unsigned char *image;           /* Binary image pointer             */
        int i;                          /* Loop counter                     */

        /* -------------------------------------------------- */

        ehdr = (Elf32_Ehdr *) addr;

        /* Find the section header string table for output info */
        shdr = (Elf32_Shdr *) (addr + ehdr->e_shoff +
                               (ehdr->e_shstrndx * sizeof (Elf32_Shdr)));

        if (shdr->sh_type == SHT_STRTAB)
                strtab = (unsigned char *) (addr + shdr->sh_offset);

        /* Load each appropriate section */
        for (i = 0; i < ehdr->e_shnum; ++i) {
                shdr = (Elf32_Shdr *) (addr + ehdr->e_shoff +
                                       (i * sizeof (Elf32_Shdr)));

                if (!(shdr->sh_flags & SHF_ALLOC)
                   || shdr->sh_addr == 0 || shdr->sh_size == 0) {
                        continue;
                }

                if (strtab) {
                        printf ("%sing %s @ 0x%08lx (%ld bytes)\n",
                                (shdr->sh_type == SHT_NOBITS) ?
                                        "Clear" : "Load",
                                &strtab[shdr->sh_name],
                                (unsigned long) shdr->sh_addr,
                                (long) shdr->sh_size);
                }

                if (shdr->sh_type == SHT_NOBITS) {
                        memset ((void *)shdr->sh_addr, 0, shdr->sh_size);
                } else {
                        image = (unsigned char *) addr + shdr->sh_offset;
                        memcpy ((void *) shdr->sh_addr,
                                (const void *) image,
                                shdr->sh_size);
                }
        }

        return ehdr->e_entry;
}

static int elfImageCheck32
    (
    Elf32_Ehdr *header
    )
    {
    int i;
    char *str;
    char *stringTable;
    Elf32_Shdr *sectionHeader;
    Elf32_Shdr *phdr;
    size_t len = 0, tmp;

    if (header->e_shoff == 0)
        {
        (void)printf("invalid ELF section offset(e_shoff)\n");
        return -1;
        }

    if (header->e_shstrndx == SHN_UNDEF)
        {
        (void)printf("invalid ELF string table section offset(e_shstrndx)\n");
        return -1;
        }

    sectionHeader = (Elf32_Shdr *)__address_add_offset(header, header->e_shoff);

    stringTable = __address_add_offset(header, sectionHeader[header->e_shstrndx].sh_offset);

    for (i = 0; i < header->e_shnum; i++)
        {
        phdr = &sectionHeader[i];

        if (!strcmp(&stringTable[phdr->sh_name], IH_TAG_SECTION_NAME))
            {
            str = __address_add_offset(header, phdr->sh_offset);

            while (len < phdr->sh_size)
                {
                if (strstr(str, IH_VXW7))
                    return 0;
                tmp = strlen(str) + 1;

                str += tmp;
                len += tmp;
                }
            }
        }
#if 0
    return -1;
#else
    return 0;
#endif
    }

int elfMachineCheck
    (
    Elf32_Half machine
    )
    {
    if (machine == EM_PPC || machine == EM_PPC64 || machine == EM_PPC_OLD)
        return 0;
    return -1;
    }

int elf32Load
    (
    unsigned char * imageHeader,
    void ** entry
    )
    {
    int i;
    Elf32_Phdr *phdr;
    Elf32_Ehdr *header = (Elf32_Ehdr *)imageHeader;
    /* map the .so, and locate interesting pieces */
    char *dynstr;
    Elf32_Shdr *dynsec;
    Elf32_Dyn *dynamic;

    valid_elf_image((unsigned long)header);

    describe_elf_hdr(header);

    describe_elf_interpreter(header);

    dynsec = elf_find_section_type(SHT_DYNAMIC, header);

    if (dynsec) {
        dynamic = (Elf32_Dyn*)(be32_to_cpu(dynsec->sh_offset) + (char *)header);
        dynstr = (char *)elf_find_dynamic(DT_STRTAB, dynamic, header, 0);
        list_needed_libraries(dynamic, dynstr);
    }

    if (strncmp ((char *)header->e_ident, (char *)ELFMAG, SELFMAG) != 0)
        {
        (void)printf("not a valid ELF image\n");
        return -1;
        }

    if (header->e_ehsize != sizeof (Elf32_Ehdr))
        {
        (void)printf("ELF32 header size is incorrect.\n");
        return -1;
        }

    if (elfMachineCheck(header->e_machine) != 0)
        {
        (void)printf("invalid arch.\n");
        return -1;
        }

    if ((header->e_type != ET_REL) && (header->e_type != ET_EXEC))
        {
        (void)printf("No relocatable or executable contents in ELF32 file.\n");
        return -1;
        }

    if ((header->e_phoff == 0) ||
        (header->e_phnum == 0) ||
        (header->e_phentsize != sizeof(Elf32_Phdr)))
        {
        (void)printf("ELF32 file header contains errors.\n");
        return -1;
        }

    if (elfImageCheck32(header) != 0)
        {
        (void)printf("Not a valid VxWorks 7 kernel image (make sure COMPAT69 is turned OFF).\n");
        return -1;
        }

    *entry = (void *)(uintptr_t)header->e_entry;

    phdr = (Elf32_Phdr *)((char *)header + header->e_phoff);

    for (i = 0; i < header->e_phnum; i++, phdr++)
        {
        if (phdr->p_type != PT_LOAD)
            continue;

        (void)printf("loading ELF32 section from %x to 0x%x, size = 0x%x, phdr:0x%x\n",
            (char *)header + phdr->p_offset,
            phdr->p_paddr, phdr->p_filesz, phdr);

        memmove((void *)(uintptr_t)phdr->p_paddr, (char *)header + phdr->p_offset, phdr->p_filesz);

        if (phdr->p_filesz < phdr->p_memsz)
                bzero ((char *)(uintptr_t)(phdr->p_paddr + phdr->p_filesz),
                       (size_t)(phdr->p_memsz - phdr->p_filesz));

        /*
          * we don't use cache, so no need to flush, add a comment here in case
          * someday we may have to
          */
        }

    return 0;
    }

static inline uint8_t getOSType
    (
    unsigned char *imageHeader
    )
    {
    struct uimage_header *header = (struct uimage_header *)imageHeader;

    return header->ih_os;
    }

static int uimageLoad
    (
    unsigned char * imageHeader,
    void ** entry
    )
    {
    size_t size;
    unsigned char *loadAddr;
    unsigned char *srcAddr;
    struct uimage_header * header = (struct uimage_header *)imageHeader;

    image_info((unsigned long)header);

    if (header->ih_os != IH_OS_VXWORKS && header->ih_os != IH_OS_LINUX)
        {
        (void)printf("unsupported target OS: %u\n", header->ih_os);
        return -1;
        }

    *entry = (void *)(uintptr_t)be32_to_cpu(header->ih_ep);

    loadAddr = (unsigned char *)(uintptr_t)be32_to_cpu(header->ih_load);

    srcAddr = imageHeader + sizeof(struct uimage_header);

    size = be32_to_cpu(header->ih_size);

    if (loadAddr != srcAddr)
        {
        if (header->ih_comp == IH_COMP_NONE)
            {
            (void)printf("copying image to 0x%x from 0x%x, size = 0x%x bytes\n",
                loadAddr, srcAddr, size);

            memmove(loadAddr, srcAddr, size);
            }
        else
            {
            (void)printf("unsupported image compression type\n");

            return -1;
            }
        }
    else
        {
        if (header->ih_comp == IH_COMP_NONE)
            (void)printf("image is XIP, start image directly\n");
        else
            {
            (void)printf("warning - srcAddr == dstAddr\n");
            return -1;
            }
        }

    return 0;
    }


static int getImageType
    (
    unsigned char *imageHeader
    )
    {
    uint32_t magic;

    const unsigned char *eident;

    struct uimage_header *header = (struct uimage_header *)imageHeader;

    magic = be32_to_cpu(header->ih_magic);

    printf("magic - 0x%x, header: 0x%x \r\n",magic, header);

    if (magic == IH_MAGIC)
        return IMAGE_TYPE_UIMAGE;
    else
        {
        eident = (unsigned char *)imageHeader;
        if (!strncmp((char *)eident, (char *)ELFMAG, SELFMAG))
            {
            if (eident[EI_CLASS] == ELFCLASS32)
                return IMAGE_TYPE_ELF32;

            if (eident[EI_CLASS] == ELFCLASS64)
                return IMAGE_TYPE_ELF64;
            }
        else
            return IMAGE_TYPE_BINARY;
        }

    return IMAGE_TYPE_UNKNOWN;
    }

/* fixes up device tree for VxWorks 7 kernel, e.g. bootline,
* memory node and spin table (for PowerPC).
*/

static int fixupDTB
    (
    void *fdt_addr
    )
    {
    int ret;
    const char *bootargs ="gei(0,0)host:vxWorks h=192.168.0.2 e=192.168.0.3 u=target pw=vxTarget";

    if (!fdt_addr)
        return 0;

    ret = fdt_check_header(fdt_addr);

    if (ret != 0)
        {
        (void)printf("%s\n", fdt_strerror(ret));
        return -1;
        }

    fdt_init(fdt_addr);

    //bootargs = getenv("bootargs");
    if (bootargs && bootargs[0] != '\0')
        {
        ret = fdt_fixup_bootargs(fdt_addr, bootargs);
        if (ret != 0)
            {
            (void)printf("error fixing bootargs: %s\n", fdt_strerror(ret));
            return ret;
            }
        }

    ret = fdt_fixup_cpu(fdt_addr);
    if (ret != 0)
        {
        (void)printf("error fixing cpu node: %s\n", fdt_strerror(ret));
        return ret;
        }

    ret = fdt_fixup_memory(fdt_addr, 0);
    if (ret != 0)
        {
        (void)printf("error fixing memory node: %s\n", fdt_strerror(ret));
        return ret;
        }

    fdt_print(fdt_addr);


    return 0;
    }

/*******************************************************************************
*
* boot - memory boot a kernel image
*
* This routine is the handler of boot shell command. Usage ('vxbl@' is shell prompt):
*
*     vxbl@ boot <kernel image address>  - <device tree blob address>
*
* Note: the second parameter provided to boot command is always ignored. The
* 'kernel image address' and 'device tree blob address' must be memory(physical) address.
*
* RETURNS: 0 on OK, -1 otherwise
*
* ERRNO: This function never returns
*
* \NOMANUAL
*/

int boot
    (
    unsigned char * imageAddr,
    unsigned char * dtbAddr
    )
    {
    int type;
    /* 64 bytes is enough for both UIMAGE and ELF32/64 identification */
    unsigned char buf[64];
    void * entry;

    struct loader_param param ={0};

    memcpy(&buf, imageAddr, 64);

    type = getImageType(buf);

    switch (type)
        {
        case IMAGE_TYPE_ELF32:
            {
            if (elf32Load(imageAddr, &entry) < 0)
                return -1;

            param.flags = IH_OS_VXWORKS;

            break;
            }

        case IMAGE_TYPE_UIMAGE:
            {
            if (uimageLoad(imageAddr, &entry) < 0)
                return -1;

            param.flags = getOSType(buf);

            break;
            }

        case IMAGE_TYPE_BINARY:
            entry = imageAddr;
            break;
        default:
            (void)printf("unknown image type(uImage or ELF not detected)\n");
            return -1;
        }

    param.kernel = imageAddr;
    param.entry  = entry;
    param.rsvd = 0;
    param.config = dtbAddr;

    fixupDTB(param.config);

    (void)printf("starting kernel @0x%x dtb0x@%x...\n",
        param.entry, param.config);

    __func_ppcEntry ppcentry = (__func_ppcEntry)param.entry;

    if ((param.flags == IH_OS_VXWORKS) || (param.flags == IH_OS_LINUX) || (type == IMAGE_TYPE_BINARY) )
    {
#if 0
    addr = param.entry;
    ((void (*)(void)) addr) ();
#else
   ppcentry(param.config, 0, 0, EPAPR_MAGIC, BOOT_MAP_SIZE, 0, 0);
#endif  
    }
    else
        {
        (void)printf("unsupported OS by arch: %u\n", param.flags);
        return -1;
        }

    (void)printf("error - boot failed\n");
    return -1;
    }
