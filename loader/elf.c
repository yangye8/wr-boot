#include <wrboot.h>
#include <stdint.h>
#include "h/elf.h"
#include <stdio.h>

/* extern */

unsigned short bswap_16(unsigned short a) 
{
return (unsigned short)((a >> 8) & 0xFF) | ((a << 8) & 0xFF00);
}

Elf32_Shdr * elf_find_section_type( uint32_t key, Elf32_Ehdr *ehdr)
{
    int j;
    Elf32_Shdr *shdr = (Elf32_Shdr *)(ehdr->e_shoff + (char *)ehdr);
    for (j = ehdr->e_shnum; --j>=0; ++shdr) {
        if (key==be32_to_cpu(shdr->sh_type)) {
            return shdr;
        }
    }
    return NULL;
}

/*

 00000000:  7f 45 4c 46 01 02 01 00  00 00 00 00 00 00 00 00  .ELF............
 00000010:  00 02 00 14 00 00 00 01  00 10 00 00 00 00 00 34  ...............4
 00000020:  00 13 6e a4 80 00 00 00  00 34 00 20 00 01 00 28  ..n......4. ...(
 00000030:  00 0c 00 09 00 00 00 01  00 00 00 80 00 10 00 00  ................
 00000040:  00 10 00 00 00 13 6d a0  00 15 88 e0 00 00 00 07  ......m.........
 00000050:  00 00 00 40 00 00 00 00  00 00 00 00 00 00 00 00  ...@............
 00000060:  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
 00000070:  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................

Program Headers:
  Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align
  LOAD           0x000080 0x00100000 0x00100000 0x136da0 0x1588e0 RWE 0x40

typedef struct
{
  Elf32_Word    p_type;         Segment type *          00000001
  Elf32_Off     p_offset;       Segment file offset *   00000080
  Elf32_Addr    p_vaddr;        Segment virtual address *  00100000
  Elf32_Addr    p_paddr;        Segment physical address * 00100000
  Elf32_Word    p_filesz;       Segment size in file *   00136da0
  Elf32_Word    p_memsz;        Segment size in memory *  001588e0
  Elf32_Word    p_flags;        Segment flags *       00000007
  Elf32_Word    p_align;        Segment alignment *   00000040
} Elf32_Phdr;
 

Legal values for p_type (segment type).

#define PT_NULL     0       Program header table entry unused *
#define PT_LOAD     1       Loadable program segment *
#define PT_DYNAMIC  2       Dynamic linking information *
#define PT_INTERP   3       Program interpreter *
#define PT_NOTE     4       Auxiliary information *
#define PT_SHLIB    5       Reserved *
#define PT_PHDR     6       Entry for header table itself *
#define PT_TLS      7       Thread-local storage segment *
#define PT_NUM      8       Number of defined types *
#define PT_LOOS     0x60000000      Start of OS-specific *
#define PT_GNU_EH_FRAME 0x6474e550  GCC .eh_frame_hdr segment *
#define PT_GNU_STACK    0x6474e551  Indicates stack executability *
#define PT_GNU_RELRO    0x6474e552  Read-only after relocation *
#define PT_LOSUNW   0x6ffffffa
#define PT_SUNWBSS  0x6ffffffa       Sun Specific segment *
#define PT_SUNWSTACK    0x6ffffffb   Stack segment *
#define PT_HISUNW   0x6fffffff
#define PT_HIOS     0x6fffffff End of OS-specific *
#define PT_LOPROC   0x70000000 Start of processor-specific *
#define PT_HIPROC   0x7fffffff End of processor-specific *
*/

Elf32_Phdr * elf_find_phdr_type( uint32_t type, Elf32_Ehdr *ehdr)
{
    int j;
    Elf32_Phdr *phdr = (Elf32_Phdr *)(ehdr->e_phoff + (char *)ehdr);
    printf(" phdr:0x%x\n Program header table file offset(ehdr->e_phoff):0x%x\n Program header table entry count(ehdr->e_phnum):0x%x\r\n",phdr,ehdr->e_phoff,ehdr->e_phnum);
    for (j = ehdr->e_phnum; --j>=0; ++phdr) {
        if (type==be32_to_cpu(phdr->p_type)) {
            return phdr;
        }
    }
    return NULL;
}

/* Returns value if return_val==1, ptr otherwise */ 
void * elf_find_dynamic( long long key, Elf32_Dyn *dynp, 
    Elf32_Ehdr *ehdr, int return_val)
{
    Elf32_Phdr *pt_text = elf_find_phdr_type(PT_LOAD, ehdr);
    Elf32_Addr tx_reloc = be32_to_cpu(pt_text->p_vaddr) - be32_to_cpu(pt_text->p_offset);
    for (; DT_NULL!=be32_to_cpu(dynp->d_tag); ++dynp) {
        if (key == be32_to_cpu(dynp->d_tag)) {
            if (return_val == 1)
                return (void *)be32_to_cpu(dynp->d_un.d_val);
            else
                return (void *)(be32_to_cpu(dynp->d_un.d_val) - tx_reloc + (char *)ehdr );
        }
    }
    return NULL;
}

/*

 00000000:  7f 45 4c 46 01 02 01 00  00 00 00 00 00 00 00 00  .ELF............
 00000010:  00 02 00 14 00 00 00 01  00 10 00 00 00 00 00 34  ...............4
 00000020:  00 13 6e a4 80 00 00 00  00 34 00 20 00 01 00 28  ..n......4. ...(
 00000030:  00 0c 00 09 00 00 00 01  00 00 00 80 00 10 00 00  ................
 00000040:  00 10 00 00 00 13 6d a0  00 15 88 e0 00 00 00 07  ......m.........
 00000050:  00 00 00 40 00 00 00 00  00 00 00 00 00 00 00 00  ...@............
 00000060:  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
 00000070:  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................

 The ELF file header.  This appears at the start of every ELF file.

#define EI_NIDENT (16)

7f 45 4c 46 01 02 01 00

#define EI_CLASS    4       File class byte index *
#define ELFCLASSNONE    0   Invalid class *
#define ELFCLASS32  1       32-bit objects *
#define ELFCLASS64  2       64-bit objects *
#define ELFCLASSNUM 3

#define EI_DATA     5       Data encoding byte index *
#define ELFDATANONE 0       Invalid data encoding *
#define ELFDATA2LSB 1       2's complement, little endian *
#define ELFDATA2MSB 2       2's complement, big endian *
#define ELFDATANUM  3

#define EI_VERSION  6        File version byte index 
#define EI_OSABI   7            OS ABI identification *
#define ELFOSABI_NONE       0   UNIX System V ABI *
#define ELFOSABI_SYSV       0   Alias.  *

typedef struct
{
  unsigned char e_ident[EI_NIDENT];  Magic number and other info
  Elf32_Half    e_type;         Object file type *    0002
  Elf32_Half    e_machine;      Architecture *        0014
  Elf32_Word    e_version;      Object file version * 00000001
  Elf32_Addr    e_entry;        Entry point virtual address *  00100000
  Elf32_Off e_phoff;            Program header table file offset * 00000034
  Elf32_Off e_shoff;            Section header table file offset * 00136ea4
  Elf32_Word    e_flags;        Processor-specific flags *     80000000
  Elf32_Half    e_ehsize;       ELF header size in bytes *     0034
  Elf32_Half    e_phentsize;    Program header table entry size * 0020
  Elf32_Half    e_phnum;        Program header table entry count *    0001
  Elf32_Half    e_shentsize;    Section header table entry size * 0028
  Elf32_Half    e_shnum;        Section header table entry count *    000c
  Elf32_Half    e_shstrndx;     Section header string table index *   0009
} Elf32_Ehdr;
#endif    sizeof(Elf32_Ehdr) = 0x34

*/

void describe_elf_hdr(Elf32_Ehdr* ehdr)
{
    char *tmp, *tmp1;

    switch (ehdr->e_type) {
        case ET_NONE:   tmp = "None"; tmp1 = "NONE"; break;
        case ET_REL:    tmp = "Relocatable File"; tmp1 = "REL"; break;
        case ET_EXEC:   tmp = "Executable file"; tmp1 = "EXEC"; break;
        case ET_DYN:    tmp = "Shared object file"; tmp1 = "DYN"; break;
        case ET_CORE:   tmp = "Core file"; tmp1 = "CORE"; break;
        default:
                        tmp = tmp1 = "Unknown";
    }
    printf( "Type(0x%x):\t\t%s (%s)\n", ehdr->e_type, tmp1, tmp);

    switch (ehdr->e_machine) {
        case EM_NONE:       tmp="No machine"; break;
        case EM_M32:        tmp="AT&T WE 32100"; break;
        case EM_SPARC:      tmp="SUN SPARC"; break;
        case EM_386:        tmp="Intel 80386"; break;
        case EM_68K:        tmp="Motorola m68k family"; break;
        case EM_88K:        tmp="Motorola m88k family"; break;
        case EM_486:        tmp="Intel 80486"; break;
        case EM_860:        tmp="Intel 80860"; break;
        case EM_MIPS:       tmp="MIPS R3000 big-endian"; break;
        case EM_S370:       tmp="IBM System/370"; break;
        case EM_MIPS_RS3_LE:    tmp="MIPS R3000 little-endian"; break;
        case EM_PARISC:     tmp="HPPA"; break;
        /*case EM_PPC_OLD:  tmp="Power PC (old)"; break;  conflicts with EM_VPP500 */
        case EM_SPARC32PLUS:    tmp="Sun's v8plus"; break;
        case EM_960:        tmp="Intel 80960"; break;
        case EM_PPC:        tmp="PowerPC"; break;
        case EM_PPC64:      tmp="PowerPC 64-bit"; break;
        case EM_V800:       tmp="NEC V800 series"; break;
        case EM_FR20:       tmp="Fujitsu FR20"; break;
        case EM_RH32:       tmp="TRW RH-32"; break;
        case EM_MCORE:      tmp="MCORE"; break;
        case EM_ARM:        tmp="ARM"; break;
        case EM_FAKE_ALPHA: tmp="Digital Alpha"; break;
        case EM_SH:         tmp="Renesas SH"; break;
        case EM_SPARCV9:    tmp="SPARC v9 64-bit"; break;
        case EM_TRICORE:    tmp="Siemens Tricore"; break;
        case EM_ARC:        tmp="Argonaut RISC Core"; break;
        case EM_H8_300:     tmp="Renesas H8/300"; break;
        case EM_H8_300H:    tmp="Renesas H8/300H"; break;
        case EM_H8S:        tmp="Renesas H8S"; break;
        case EM_H8_500:     tmp="Renesas H8/500"; break;
        case EM_IA_64:      tmp="Intel Merced"; break;
        case EM_MIPS_X:     tmp="Stanford MIPS-X"; break;
        case EM_COLDFIRE:   tmp="Motorola Coldfire"; break;
        case EM_68HC12:     tmp="Motorola M68HC12"; break;
        case EM_ALPHA:      tmp="Alpha"; break;
        case EM_CYGNUS_D10V:
        case EM_D10V:       tmp="Mitsubishi D10V"; break;
        case EM_CYGNUS_D30V:
        case EM_D30V:       tmp="Mitsubishi D30V"; break;
        case EM_CYGNUS_M32R:
        case EM_M32R:       tmp="Renesas M32R (formerly Mitsubishi M32r)"; break;
        case EM_CYGNUS_V850:
        case EM_V850:       tmp="NEC v850"; break;
        case EM_CYGNUS_MN10300:
        case EM_MN10300:    tmp="Matsushita MN10300"; break;
        case EM_CYGNUS_MN10200:
        case EM_MN10200:    tmp="Matsushita MN10200"; break;
        case EM_CYGNUS_FR30:
        case EM_FR30:       tmp="Fujitsu FR30"; break;
        case EM_CYGNUS_FRV:
        case EM_PJ_OLD:     
        case EM_PJ:         tmp="picoJava"; break;
        case EM_MMA:        tmp="Fujitsu MMA Multimedia Accelerator"; break;
        case EM_PCP:        tmp="Siemens PCP"; break;
        case EM_NCPU:       tmp="Sony nCPU embeeded RISC"; break;
        case EM_NDR1:       tmp="Denso NDR1 microprocessor"; break;
        case EM_STARCORE:   tmp="Motorola Start*Core processor"; break;
        case EM_ME16:       tmp="Toyota ME16 processor"; break;
        case EM_ST100:      tmp="STMicroelectronic ST100 processor"; break;
        case EM_TINYJ:      tmp="Advanced Logic Corp. Tinyj emb.fam"; break;
        case EM_FX66:       tmp="Siemens FX66 microcontroller"; break;
        case EM_ST9PLUS:    tmp="STMicroelectronics ST9+ 8/16 mc"; break;
        case EM_ST7:        tmp="STmicroelectronics ST7 8 bit mc"; break;
        case EM_68HC16:     tmp="Motorola MC68HC16 microcontroller"; break;
        case EM_68HC11:     tmp="Motorola MC68HC11 microcontroller"; break;
        case EM_68HC08:     tmp="Motorola MC68HC08 microcontroller"; break;
        case EM_68HC05:     tmp="Motorola MC68HC05 microcontroller"; break;
        case EM_SVX:        tmp="Silicon Graphics SVx"; break;
        case EM_ST19:       tmp="STMicroelectronics ST19 8 bit mc"; break;
        case EM_VAX:        tmp="Digital VAX"; break;
        case EM_AVR_OLD:
        case EM_AVR:        tmp="Atmel AVR 8-bit microcontroller"; break;
        case EM_CRIS:       tmp="Axis Communications 32-bit embedded processor"; break;
        case EM_JAVELIN:    tmp="Infineon Technologies 32-bit embedded processor"; break;
        case EM_FIREPATH:   tmp="Element 14 64-bit DSP Processor"; break;
        case EM_ZSP:        tmp="LSI Logic 16-bit DSP Processor"; break;
        case EM_MMIX:       tmp="Donald Knuth's educational 64-bit processor"; break;
        case EM_HUANY:      tmp="Harvard University machine-independent object files"; break;
        case EM_PRISM:      tmp="SiTera Prism"; break;
        case EM_X86_64:     tmp="AMD x86-64 architecture"; break;
        case EM_S390_OLD:
        case EM_S390:       tmp="IBM S390"; break;
        case EM_XSTORMY16:  tmp="Sanyo Xstormy16 CPU core"; break;
        case EM_OPENRISC:
        case EM_OR32:       tmp="OpenRISC"; break;
        case EM_CRX:        tmp="National Semiconductor CRX microprocessor"; break;
        case EM_DLX:        tmp="OpenDLX"; break;
        case EM_IP2K_OLD:
        case EM_IP2K:       tmp="Ubicom IP2xxx 8-bit microcontrollers"; break;
        case EM_IQ2000:     tmp="Vitesse IQ2000"; break;
        case EM_XTENSA_OLD:
        case EM_XTENSA:     tmp="Tensilica Xtensa Processor"; break;
        case EM_M32C:       tmp="Renesas M32c"; break;
        case EM_MT:         tmp="Morpho Techologies MT processor"; break;
        case EM_BLACKFIN:   tmp="Analog Devices Blackfin"; break;
        case EM_NIOS32:     tmp="Altera Nios 32"; break;
        case EM_ALTERA_NIOS2:   tmp="Altera Nios II"; break;
        case EM_VPP500:     tmp="Fujitsu VPP500"; break;
        case EM_PDSP:       tmp="Sony DSP Processor"; break;
        default:            tmp="unknown";
    }
    printf( "Machine(0x%x):\t\t%s\n", ehdr->e_machine,tmp); 

    switch (ehdr->e_ident[EI_CLASS]) {
        case ELFCLASSNONE: tmp = "Invalid class";  break;
        case ELFCLASS32:   tmp = "ELF32"; break;
        case ELFCLASS64:   tmp = "ELF64"; break;
        default:           tmp = "Unknown";
    }
    printf( "Class(0x%x):\t\t%s\n", ehdr->e_ident[EI_CLASS], tmp);

    switch (ehdr->e_ident[EI_DATA]) {
        case ELFDATANONE:  tmp = "Invalid data encoding"; break;
        case ELFDATA2LSB:  tmp = "2's complement, little endian"; break;
        case ELFDATA2MSB:  tmp = "2's complement, big endian"; break;
        default:           tmp = "Unknown";
    }
    printf( "Data(0x%x):\t\t%s\n", ehdr->e_ident[EI_DATA], tmp);

    printf( "Version:\t\t%d %s\n", ehdr->e_ident[EI_VERSION],
            (ehdr->e_ident[EI_VERSION]==EV_CURRENT)? 
            "(current)" : "(unknown: %lx)");

    switch (ehdr->e_ident[EI_OSABI]) {
        case ELFOSABI_SYSV:       tmp ="UNIX - System V"; break;
        case ELFOSABI_HPUX:       tmp ="UNIX - HP-UX"; break;
        case ELFOSABI_NETBSD:     tmp ="UNIX - NetBSD"; break;
        case ELFOSABI_LINUX:      tmp ="UNIX - Linux"; break;
        case ELFOSABI_HURD:       tmp ="GNU/Hurd"; break;
        case ELFOSABI_SOLARIS:    tmp ="UNIX - Solaris"; break;
        case ELFOSABI_AIX:        tmp ="UNIX - AIX"; break;
        case ELFOSABI_IRIX:       tmp ="UNIX - IRIX"; break;
        case ELFOSABI_FREEBSD:    tmp ="UNIX - FreeBSD"; break;
        case ELFOSABI_TRU64:      tmp ="UNIX - TRU64"; break;
        case ELFOSABI_MODESTO:    tmp ="Novell - Modesto"; break;
        case ELFOSABI_OPENBSD:    tmp ="UNIX - OpenBSD"; break;
        case ELFOSABI_STANDALONE: tmp ="Standalone App"; break;
        case ELFOSABI_ARM:        tmp ="ARM"; break;
        default:                  tmp = "Unknown";
    }
    printf( "OS/ABI(0x%x):\t\t%s\n", ehdr->e_ident[EI_OSABI], tmp);

    printf( "ABI Version:\t\t%d\n", ehdr->e_ident[EI_ABIVERSION]);
}

void list_needed_libraries(Elf32_Dyn* dynamic, char *strtab)
{
    Elf32_Dyn  *dyns;

    printf("Dependancies:\n");
    for (dyns=dynamic; be32_to_cpu(dyns->d_tag)!=DT_NULL; ++dyns) {
        if (dyns->d_tag == DT_NEEDED) {
            printf("\t%s\n", (char*)strtab + be32_to_cpu(dyns->d_un.d_val));
        }
    }
}

void describe_elf_interpreter(Elf32_Ehdr* ehdr)
{
    Elf32_Phdr *phdr;
    phdr = elf_find_phdr_type(PT_LOAD, ehdr);
    if (phdr) {
        printf("Interpreter:program header offset\t @%x\n", (char*)ehdr + be32_to_cpu(phdr->p_offset));
    }
}

