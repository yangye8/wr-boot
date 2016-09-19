/* libfdt_util.h - flatten device tree header files */
#include <stdint.h>

#ifndef _LIBFDT_UTIL_H
#define _LIBFDT_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

int fdt_fixup_cpu(void * fdt_addr);
int fdt_fixup_bootargs(void * fdt_addr, const char * bootargs);
int fdt_fixup_memory(void *fdt_addr,int overwrite  );
void fdt_init(void *fdt_addr);
int fdt_print(const void *fdt_addr);

struct mem_desc
    {
    uint64_t addr;
    uint64_t size;
    };


#ifdef __cplusplus
}
#endif

#endif /* _LIBFDT_UTIL_H */
