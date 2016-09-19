/* spintable.h - spintable definition for PowerPC */

#ifndef __INCspintableh__
#define __INCspintableh__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct spin_table {
    uint64_t entry;
    uint64_t r3;
    uint32_t rsvd1;
    uint32_t pir;
    uint8_t pad[40];
};

#ifdef __cplusplus
}
#endif

#endif