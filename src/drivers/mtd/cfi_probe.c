/* vivi/drivers/mtd/cfi-probe.c: Common Flash Interface probe code.
 *
 * Based on linux/drivers/mtd/chips/cfi_probe.c
 *
 * $Id: cfi_probe.c,v 1.1.1.1 2004/02/04 06:22:25 laputa Exp $
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "map.h"
#include "cfi.h"
#include "gen_probe.h"

#define DEBUG
#ifdef DEBUG
static void print_cfi_ident(struct cfi_ident *);
#endif

static int cfi_probe_chip(struct map_info *map, __u32 base,
                          struct flchip *chips, struct cfi_private *cfi);
static int cfi_chip_setup(struct map_info *map, struct cfi_private *cfi);

struct mtd_info *cfi_probe(struct map_info *map);

/*
 * check for QRY.
 * in: interleave, type, mode
 * ret: table index, < 0 for error
 */

static inline int qry_present(struct map_info *map, __u32 base, 
                              struct cfi_private *cfi)
{
    int osf = cfi->interleave * cfi->device_type;   /* scale factor */

    if (cfi_read(map, base+osf*0x10) == cfi_build_cmd('Q', map, cfi) &&
        cfi_read(map, base+osf*0x11) == cfi_build_cmd('R', map, cfi) &&
        cfi_read(map, base+osf*0x12) == cfi_build_cmd('Y', map, cfi))
        return 1;   /* ok */

    return 0;   /* nothing found */
}


static int cfi_probe_chip(struct map_info *map, __u32 base,
                          struct flchip *chips, struct cfi_private *cfi)
{
    int i;

    if ((base + 0) >= map->size) {
        printf("Probe at base[0x00](0x%08lx) past the end of map(0x%08lx)\n",
            (unsigned long)base, map->size - 1);
//        return 0;
    }
    else
        printf("Probe at base[0x00](0x%08lx) past the end of map(0x%08lx)\n",
          (unsigned long)base, map->size - 1);

    if ((base + 0xff) >= map->size) {
        printf("Probe at base[0x55](0x%08lx) past the end of the map(0x%08lx)\n",
            (unsigned long)base + 0x55, map->size - 1);
//       return 0;
    }
    else
        printf("Probe at base[0x55](0x%08lx) past the end of the map(0x%08lx)\n",
            (unsigned long)base + 0x55, map->size - 1);

    cfi_send_gen_cmd(0xF0, 0, base, map, cfi, cfi->device_type, NULL);
    cfi_send_gen_cmd(0x98, 0x55, base, map, cfi, cfi->device_type, NULL);

    if (!qry_present(map, base, cfi))
        return 0;

    if (!cfi->numchips) {
        /* 
         * This is the first time we're called. Set up the CFI
         * stuff accordingly and return 
         */
        return cfi_chip_setup(map, cfi);

    }

    /* Check each previous chip to see if it's an alias */
    for (i = 0; i < cfi->numchips; i++) {
        /* This chip should be in read mode if it's one
         * we've alread touched. */
        if (qry_present(map, chips[i].start,cfi)) {
            /* Eep. This chip also had the QRY marker.
             * Is it an alias for the new one? */
            cfi_send_gen_cmd(0xF0, 0, chips[i].start, map, cfi, cfi->device_type, NULL);

            /* If the QRY marker goes away, it's an alias */
            if (!qry_present(map, chips[i].start, cfi)) {
                printf("%s: Found an alias at 0x%x for the chip at 0x%lx\n", 
                    map->name, base, chips[i].start);
                return 0;
            }
            /* Yes, it's actually got QRY for data. Most
             * unfortunate. Stick the new chip in read mode
             * too end if it's the same, assume it's an alias. */
            /* FIXME: Use other modes to do a proper check */
            cfi_send_gen_cmd(0xF0, 0, base, map, cfi, cfi->device_type, NULL);
            if (qry_present(map, base, cfi)) {
                printf("%s: Found an alias at 0x%x for the chip at 0x%lx\n",
                    map->name, base, chips[i].start);
                return 0;
            }
        }
    }

    /* OK, if we got to here, then none of the previous chips appear to
     * be aliases for the current one. */
    if (cfi->numchips == MAX_CFI_CHIPS) {
        printf("%s: Too many flash chips detected. Increase MAX_CFI_CHIPS from %d.\n", map->name, MAX_CFI_CHIPS);
        /* Doesn't matter about resetting it to Read Mode - we're not going to talk to anyway */
        return -1;
    }
    chips[cfi->numchips].start = base;
    cfi->numchips++;

    /* Put it back into Read Mode */
    cfi_send_gen_cmd(0xF0, 0, base, map, cfi, cfi->device_type, NULL);

    printf("%s: Found %d x%d devices at 0x%x in %d-bit mode\n",
        map->name, cfi->interleave, cfi->device_type*8, base,
        map->buswidth*8);

    return 1;
}

static int cfi_chip_setup(struct map_info *map, struct cfi_private *cfi)
{
    int ofs_factor = cfi->interleave * cfi->device_type;
    __u32 base = 0;
    int num_erase_regions = cfi_read_query(map, base + (0x10 + 28)*ofs_factor);
    int i;

#ifdef DEBUG
    printf("Number of erase regions: %d\n", num_erase_regions);
#endif
    if (!num_erase_regions)
        return 0;

    cfi->cfiq = kmalloc(sizeof(struct cfi_ident) + num_erase_regions * 4);

    if (!cfi->cfiq) {
        printf("%s: kmalloc failed for CFI ident structure\n", map->name);
        return 0;
    }
    memset(cfi->cfiq, 0, sizeof(struct cfi_ident));

    /* Read the CFI info structure */
    for (i = 0; i < (sizeof(struct cfi_ident) + num_erase_regions * 4); i++) {
        ((unsigned char *)cfi->cfiq)[i] = cfi_read_query(map, base + (0x10 + i)*ofs_factor);
    }

#ifdef DEBUG
    /* Dump the information therin */
    print_cfi_ident(cfi->cfiq);
#endif

    for (i = 0; i < cfi->cfiq->NumEraseRegions; i++) {
        cfi->cfiq->EraseRegionInfo[i] = cfi->cfiq->EraseRegionInfo[i];

#ifdef DEBUG
        printf("  erase Region #%d: BlockSize 0x%4.4x bytes, %d blocks\n",
            i, (cfi->cfiq->EraseRegionInfo[i] >> 8) & ~0xff,
            (cfi->cfiq->EraseRegionInfo[i] & 0xffff) + 1);
#endif
    }
    
    /* Put it back into Read Mode */    
    cfi_send_gen_cmd(0xF0, 0, base, map, cfi, cfi->device_type, NULL);

    return 1;
}

#ifdef DEBUG
static char *vendorname(__u16 vendor) 
{
        switch (vendor) {
        case P_ID_NONE:
                return "None"; 
         
        case P_ID_INTEL_EXT:
                return "Intel/Sharp Extended";
         
        case P_ID_AMD_STD:
                return "AMD/Fujitsu Standard";
         
        case P_ID_INTEL_STD:
                return "Intel/Sharp Standard";
         
        case P_ID_AMD_EXT:
                return "AMD/Fujitsu Extended";
         
        case P_ID_MITSUBISHI_STD:
                return "Mitsubishi Standard";
         
        case P_ID_MITSUBISHI_EXT:
                return "Mitsubishi Extended";
         
        case P_ID_RESERVED:
                return "Not Allowed / Reserved for Future Use";
         
        default:
                return "Unknown";
        }
}

static void print_cfi_ident(struct cfi_ident *cfip)
{
#if 0
        if (cfip->qry[0] != 'Q' || cfip->qry[1] != 'R' || cfip->qry[2] != 'Y') {
                printf("Invalid CFI ident structure.\n");
                return;
        }       
#endif          
        printf("Primary Vendor Command Set: %4.4X (%s)\n", cfip->P_ID, vendorname(cfip->P_ID));
        if (cfip->P_ADR)
                printf("Primary Algorithm Table at %4.4X\n", cfip->P_ADR);
        else
                printf("No Primary Algorithm Table\n");
        
        printf("Alternative Vendor Command Set: %4.4X (%s)\n", cfip->A_ID, vendorname(cfip->A_ID));
        if (cfip->A_ADR)
                printf("Alternate Algorithm Table at %4.4X\n", cfip->A_ADR);
        else
                printf("No Alternate Algorithm Table\n");


        printf("Vcc Minimum: %x.%x V\n", cfip->VccMin >> 4, cfip->VccMin & 0xf);
        printf("Vcc Maximum: %x.%x V\n", cfip->VccMax >> 4, cfip->VccMax & 0xf);
        if (cfip->VppMin) {
                printf("Vpp Minimum: %x.%x V\n", cfip->VppMin >> 4, cfip->VppMin & 0xf);
                printf("Vpp Maximum: %x.%x V\n", cfip->VppMax >> 4, cfip->VppMax & 0xf);
        }
        else
                printf("No Vpp line\n");

        printf("Typical byte/word write timeout: %d 탎\n", 1<<cfip->WordWriteTimeoutTyp);
        printf("Maximum byte/word write timeout: %d 탎\n", (1<<cfip->WordWriteTimeoutMax) * (1<<cfip->WordWriteTimeoutTyp));

        if (cfip->BufWriteTimeoutTyp || cfip->BufWriteTimeoutMax) {
                printf("Typical full buffer write timeout: %d 탎\n", 1<<cfip->BufWriteTimeoutTyp);
                printf("Maximum full buffer write timeout: %d 탎\n", (1<<cfip->BufWriteTimeoutMax) * (1<<cfip->BufWriteTimeoutTyp));
        }
        else
                printf("Full buffer write not supported\n");

        printf("Typical block erase timeout: %d 탎\n", 1<<cfip->BlockEraseTimeoutTyp);
        printf("Maximum block erase timeout: %d 탎\n", (1<<cfip->BlockEraseTimeoutMax) * (1<<cfip->BlockEraseTimeoutTyp));
        if (cfip->ChipEraseTimeoutTyp || cfip->ChipEraseTimeoutMax) {
                printf("Typical chip erase timeout: %d 탎\n", 1<<cfip->ChipEraseTimeoutTyp);
                printf("Maximum chip erase timeout: %d 탎\n", (1<<cfip->ChipEraseTimeoutMax) * (1<<cfip->ChipEraseTimeoutTyp));
        }
        else
                printf("Chip erase not supported\n");

        printf("Device size: 0x%X bytes (%d MiB)\n", 1 << cfip->DevSize, 1<< (cfip->DevSize - 20));
        printf("Flash Device Interface description: 0x%4.4X\n", cfip->InterfaceDesc);
        switch(cfip->InterfaceDesc) {
        case 0:
                printf("  - x8-only asynchronous interface\n");
                break;

        case 1:
                printf("  - x16-only asynchronous interface\n");
                break;

        case 2:
                printf("  - supports x8 and x16 via BYTE# with asynchronous interface\n");
                break;

        case 3:
                printf("  - x32-only asynchronous interface\n");
                break;

        case 65535:
                printf("  - Not Allowed / Reserved\n");
                break;

        default:
                printf("  - Unknown\n");
                break;
        }

        printf("Max. bytes in buffer write: 0x%x\n", 1<< cfip->MaxBufWriteSize);
        printf("Number of Erase Block Regions: %d\n", cfip->NumEraseRegions);

}
#endif /* DEBUG */

static struct chip_probe cfi_chip_probe = {
    name: "CFI",
    probe_chip: cfi_probe_chip
};

struct mtd_info *cfi_probe(struct map_info *map)
{
    /*
     * Just use the generic probe stuff to call our CFI-specific
     * chip_probe routine in all the possible permutations, etc.
     */
    return mtd_do_chip_probe(map, &cfi_chip_probe);
}
