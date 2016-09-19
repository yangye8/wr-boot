/* image.h - u-boot image header layer */

#ifndef __INCimageh__
#define __INCimageh__

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

typedef uint64_t __le64;
typedef uint32_t __le32;
typedef uint16_t __le16;
typedef uint64_t __be64;
typedef uint32_t __be32;
typedef uint16_t __be16;


#define IH_MAGIC    0x27051956
#define IH_NMLEN    32

#define IH_VXW7     "tag VXW7"
#define IH_VX6X     "tag COMPAT69"
#define IH_TAG_SECTION_NAME ".wrs_build_vars"

/*
 * Compression Types
 */
#define IH_COMP_NONE        0
#define IH_COMP_GZIP        1

/*
 * OS types
 */

#define IH_OS_VXWORKS       14
#define IH_OS_LINUX         5

struct loader_param
    {
    void *kernel;
    void *config;
    void *rsvd;
    void *entry;
    int flags;
    };


struct uimage_header
    {
    __be32        ih_magic;
    __be32        ih_hcrc;
    __be32        ih_time;
    __be32        ih_size;
    __be32        ih_load;
    __be32        ih_ep;
    __be32        ih_dcrc;
    uint8_t       ih_os;
    uint8_t       ih_arch;
    uint8_t       ih_type;
    uint8_t       ih_comp;
    uint8_t       ih_name[IH_NMLEN];
    };

#define IMAGE_TYPE_UNKNOWN  0x0
#define IMAGE_TYPE_UIMAGE   0x1
#define IMAGE_TYPE_ELF32    0x2
#define IMAGE_TYPE_ELF64    0x4
#define IMAGE_TYPE_BINARY   0x8

#ifdef __cplusplus
}
#endif

#endif
