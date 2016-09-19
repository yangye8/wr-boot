/* fdt_util.c - flatten device tree help functions */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <spin_table.h>
#include "ctype.h"

#include "libfdt_env.h"

#include "fdt.h"
#include "libfdt.h"

#include "libfdt_internal.h"

#include "libfdt_util.h"

extern unsigned int __spin_table[];


/* use the same value in u-boot */

#ifndef _WRS_CONFIG_FDT_PADDING
#define _WRS_CONFIG_FDT_PADDING 0x3000
#endif

/* externs */

extern const void *__fdt_addr;
extern unsigned getCpuId(void);

/*******************************************************************************
*
* fdt_write_val - write 32bit/64bit value
*
* This is a helper rouine to write a value with specified size
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

static void fdt_write_val
    (
    void *buf,
    uint64_t val,
    int size
    )
    {
    if (size == 4)
        {
        uint32_t *tmp = buf;
        *tmp = cpu_to_fdt32((uint32_t)val);
        }
    else
        {
        uint64_t *tmp = buf;
        *tmp = cpu_to_fdt64(val);
        }
    }

/*******************************************************************************
*
* fdt_is_string - determine if a value is a string
*
* This routine determines if a value is a string
*
* RETURNS: 1 if data is string, 0 otherwise
*
* ERRNO: N/A
*
* \NOMANUAL
*/

static int fdt_is_string
    (
    const char * str,
    uint32_t len
    )
    {
    int i;

    if (str[0] == '\0' && len == 1)
        return 1;
    if (str[len - 1] != '\0' && str[len - 1] != '\n')
        return 0;

    for (i = 0; i < len - 1; i++)
        {
        if (str[i] == '\0')
            {
            if (!isprint(str[i + 1]))
                return 0;
            else
                continue;
            }

        if (!isprint(str[i]))
            {
            return 0;
            }
        }

    return 1;
    }

/*******************************************************************************
*
* fdt_fixup_memory_node - fix up memroy node in device tree blobl
*
* This routine fixes up memory node in device tree blob. Note, this routine
* assumes that the 'memory' node already exists.
*
* RETURNS: 0 on OK, or a negative value indicating error
*
* ERRNO: N/A
*
* \NOMANUAL
*/

static int fdt_fixup_memory_node
    (
    void *fdt_addr,
    struct mem_desc *desc,
    int size,
    int memOffset
    )
    {
    int i, len;
    uint32_t address_cell, size_cell;
    const fdt32_t *tmp;
    char buf[sizeof(struct mem_desc) * size];

    tmp = fdt_getprop(fdt_addr, 0, "#address-cells", NULL);
    address_cell = tmp ? fdt32_to_cpu(*tmp) : 2;
    address_cell = address_cell == 2 ? 8 : 4;

    tmp = fdt_getprop(fdt_addr, 0, "#size-cells", NULL);
    size_cell = tmp ? fdt32_to_cpu(*tmp) : 1;
    size_cell = size_cell == 2 ? 8 : 4;

    for (i = 0, len = 0; i < size; i++)
        {
        fdt_write_val(buf + len, desc[i].addr, address_cell);
        len += address_cell;
        fdt_write_val(buf + len, desc[i].size, size_cell);
        len += size_cell;
        }

    return fdt_setprop(fdt_addr, memOffset, "reg", buf, len);
    }

static struct mem_desc pt1mem[] =
    {
        {
        .addr = (uint64_t)0,
        .size = (uint64_t) 0x40000000
        }
    };

void sysMemDescGet
    (
    struct mem_desc * * outp,
    int * size
    )
    {
    *outp = pt1mem;
    *size = 1;
    }

/*******************************************************************************
*
* fdt_fixup_memory - fix up memroy node in device tree blobl
*
* This routine fixes up memory node in device tree blob. If the 'memory' node
* does not exist, this routine will create one and perform the fixup. If the
* 'memory' node already exists, and if input paramter 'overwrite' is not 0,
* then the current content of memory node will be overwritten.
*
* RETURNS: 0 on OK, or a negative value indicating error
*
* ERRNO: N/A
*
* \NOMANUAL
*/

int fdt_fixup_memory
    (
    void *fdt_addr,
    int overwrite 
    )
    {
    int offset, ret;
    struct mem_desc * desc = NULL;
    int size = 0;

   // sysMemDescGet(&desc, &size);
    size = 1;

    if (!desc || size <= 0)
        {
        (void)printf("warning -  mem desc not available\n");
        return 0;
        }

    offset = fdt_path_offset(fdt_addr, "/memory");
    if (offset == -FDT_ERR_NOTFOUND)
        {
        offset = fdt_add_subnode(fdt_addr, 0, "memory");
        if (offset < 0)
            {
            (void)printf("error creating memory node - %s\n",
                    fdt_strerror(offset));
            return offset;
            }

        ret = fdt_setprop(fdt_addr, offset, "device_type",
                "memory", strlen("memory") + 1);
        if (ret < 0)
            {
            (void)printf("error setting deice type - %s\n",
                    fdt_strerror(ret));
            return ret;
            }
        return fdt_fixup_memory_node(fdt_addr, desc, size, offset);
        }
    else if (overwrite)
        return fdt_fixup_memory_node(fdt_addr, desc, size, offset);

    return 0;
    }

/*******************************************************************************
*
* fdt_fixup_bootargs - fix up bootargs in device tree blob
*
* This routine fixes up bootargs in device tree blob. If "/chosen" node is not
* present, this routine will create the "/chosen" node.
*
* RETURNS: 0 on OK, or a negative value indicating error
*
* ERRNO: N/A
*
* \NOMANUAL
*/

int fdt_fixup_bootargs
    (
    void * fdt_addr,
    const char * bootargs
    )
    {
    int offset;

    offset = fdt_path_offset(fdt_addr, "/chosen");
    if (offset < 0)
        {
        offset = fdt_add_subnode(fdt_addr, 0, "chosen");
        if (offset < 0)
            {
            (void)printf("error creating chosen - %s\n",
                    fdt_strerror(offset));
            return offset;
            }
        }

    if (!bootargs || *bootargs == '\0')
        return 0;

    offset = fdt_setprop(fdt_addr, offset, "bootargs",
            bootargs, strlen(bootargs) + 1);
    if (offset < 0)
        {
        (void)printf("error setup bootline - %s\n",
                fdt_strerror(offset));
        return offset;
        }

    return 0;
    }

/*******************************************************************************
*
* fdt_fixup_cpu - fix up cpu node in fdt device tree
*
* This routine fixes up cpu node in fdt device tree
*
* RETURNS: 0 on OK, or a negative value indicating error
*
* ERRNO: N/A
*
* \NOMANUAL
*/

int fdt_fixup_cpu
    (
    void * fdt_addr
    )
    {
    int ret;
    int offset, depth = 0, len;
    const void *val;
    unsigned int cpuid;
    unsigned int myid = getCpuId();
    unsigned long long releaseAddr;

    offset = fdt_path_offset(fdt_addr, "/cpus");
    if (offset < 0)
        {
        fdt_strerror(offset);
        return offset;
        }

    while ((offset >= 0) && (depth >= 0))
        {
        offset = fdt_next_node(fdt_addr, offset, &depth);
        if (depth != 1)
            continue;
        val = fdt_getprop(fdt_addr, offset, "reg", &len);
        if (!val)
            continue;
        cpuid = fdt32_to_cpu(*(unsigned int *)val);
       if (cpuid == myid)
            {
            ret = fdt_setprop_string(fdt_addr, offset, "status", "okay");
            if (ret != 0)
                goto error;
            continue;
            }
        else
            {
            releaseAddr = cpu_to_fdt64((unsigned long long)(uintptr_t)__spin_table + cpuid * 2);
            ret = fdt_setprop_string(fdt_addr, offset, "status", "disabled");
            if (ret != 0)
                goto error;

            ret = fdt_setprop(fdt_addr, offset, "cpu-release-addr", &releaseAddr,
                sizeof(releaseAddr));
            if (ret != 0)
                goto error;

            ret = fdt_setprop_string(fdt_addr, offset, "enable-method",
                "spin-table");
            if (ret != 0)
                goto error;
            }
        }

    ret = fdt_add_mem_rsv(fdt_addr, (unsigned long long)(uintptr_t)__spin_table,
        sizeof(struct spin_table) * 2);
    if (ret != 0)
        goto error;

    return 0;

error:
    return ret;
    }

/*******************************************************************************
*
* fdt_print_prop - print property values
*
* This routine prints property values
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

static void fdt_print_prop
    (
    const void * data,
    uint32_t len
    )
    {
    int i;
    const char *s;

    if (fdt_is_string(data, len))
        {
        s = data;
        (void)printf("\"");
        for (i = 0; i < len; i++)
            {
            if (s[i] == '\0')
                {
                (void)printf ("\"");
                if (i != (len -1))
                    {
                    (void)printf (" ");
                    (void)printf ("\"");
                    }
                }
            else
                (void)printf("%c", s[i]);
            }
        (void)printf("\n");
        }
    else
        {
        uint32_t *d = (uint32_t *)data;
        int num = len >> 2;
        (void)printf("<");
        for (i = 0; i < num - 1; i++)
            {
            (void)printf("0x%08x ", fdt32_to_cpu(d[i]));
            }
        (void)printf("0x%08x>\n", fdt32_to_cpu(d[i]));
        }
    }

/*******************************************************************************
*
* fdt_print_space - print spaces
*
* This routine prints spaces
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

static void fdt_print_space
    (
    int num
    )
    {
    while (num--)
        {
        (void)printf(" ");
        (void)printf(" ");
        (void)printf(" ");
        }
    }

/*******************************************************************************
*
* fdt_print - print device tree information
*
* This routine prints device tree information
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

int fdt_print
    (
    const void * addr
    )
    {
    int ret = 0, next;
    uint32_t tag, off;
    int len; int spaces = 0;
    const char *name;
    const struct fdt_property *prop;
    const void * fdt_addr = addr;
    
    if (addr == NULL)
        goto error;
    
    ret = fdt_check_header(fdt_addr);
    if (ret)
        goto error;

    ret = fdt_path_offset(fdt_addr, "/");
    if (ret)
        goto error;

    for (;;)
        {
        tag = fdt_next_tag(fdt_addr, ret, &next);
        switch (tag)
            {
            case FDT_BEGIN_NODE:
                {
                spaces++;
                fdt_print_space(spaces);
                name = fdt_get_name(fdt_addr, ret, &len);
                if (name)
                    (void)printf("%s {\n", *name == '\0' ? "root @" : name);
                else if (len == 0)
                    (void)printf("/ {\n");
                else
                    (void)printf("NULL\n");
                break;
                }
            case FDT_PROP:
                {
                fdt_print_space(spaces);
                prop = fdt_offset_ptr(fdt_addr, ret, sizeof(*prop));
                if (!prop)
                    {
                    ret = -FDT_ERR_BADSTRUCTURE;
                    goto error;
                    }

                len = fdt32_to_cpu(prop->len);
                off = fdt32_to_cpu(prop->nameoff);

                if (len >= 0)
                    {
                    (void)printf("%s = ", fdt_string(fdt_addr, off));
                    fdt_print_prop(prop->data, len);
                    }

                break;
                }
            case FDT_END_NODE:
                fdt_print_space(spaces);
                spaces--;
                (void)printf("}\n");
                break;
            case FDT_END:

                fdt_print_space(spaces);

                (void)printf("\n}\n");
                return 0;
            default:
                (void)printf("unknown error!!!\n");
                return -1;
            }

        ret = next;
        }

    return 0;

error:
    (void)printf("%s\n", fdt_strerror(ret));
    return ret;
    }

/*******************************************************************************
*
* fdt_init - initialize fdt library
*
* This routine initializes fdt library
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void fdt_init
    (
    void *fdt_addr
    )
    {
    /* there must be at least FDT_PADDING bytes after the dtb */

    fdt_set_totalsize(fdt_addr, fdt_totalsize(fdt_addr) + _WRS_CONFIG_FDT_PADDING);
    }

