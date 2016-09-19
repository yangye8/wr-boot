/* stdint.h - ANSI C / POSIX.1 integer type definitions */

#ifndef __INCstdinth
#define __INCstdinth

/* includes */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLY__

/* Exact-width integer types */

/*typedef char int8_t;*/
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned long ulong;

/* Integer types capable of holding object pointers */

#ifndef _WRS_CONFIG_LP64
typedef uint32_t size_t;
typedef int intptr_t;
typedef unsigned int uintptr_t;
#else
typedef uint64_t size_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
#endif /* _WRS_CONFIG_LP64 */

/* Limits of Specified-Width Integer Types */

#ifndef INT8_MIN

#define INT8_MIN    (-0x7F - 1)
#define INT8_MAX    0x7F
#define UINT8_MAX   0xFF

#define INT16_MIN   (-0x7FFF - 1)
#define INT16_MAX   0x7FFF
#define UINT16_MAX  0xFFFF

#define INT32_MIN   (-0x7FFFFFFF - 1)
#define INT32_MAX   0x7FFFFFFF
#define UINT32_MAX  0xFFFFFFFFU

#define INT64_MIN   (-0x7FFFFFFFFFFFFFFFLL - 1)
#define INT64_MAX   0x7FFFFFFFFFFFFFFFLL
#define UINT64_MAX  0xFFFFFFFFFFFFFFFFULL

#ifndef _WRS_CONFIG_LP64
#define INTPTR_MIN  INT32_MIN
#define INTPTR_MAX  INT32_MAX
#define UINTPTR_MAX UINT32_MAX
#else
#define INTPTR_MIN  INT64_MIN
#define INTPTR_MAX  INT64_MAX
#define UINTPTR_MAX UINT64_MAX
#endif /* _WRS_CONFIG_LP64 */

/* Limits of Other Integer Types */

#ifndef _WRS_CONFIG_LP64
#define PTRDIFF_MIN INT32_MIN
#define PTRDIFF_MAX INT32_MAX
#else
#define PTRDIFF_MIN INT64_MIN
#define PTRDIFF_MAX INT64_MAX
#endif /* _WRS_CONFIG_LP64 */

#ifndef _WRS_CONFIG_LP64
#define SIG_ATOMIC_MIN  INT32_MIN
#define SIG_ATOMIC_MAX  INT32_MAX
#else
#define SIG_ATOMIC_MIN  INT64_MIN
#define SIG_ATOMIC_MAX  UINT64_MAX
#endif /* _WRS_CONFIG_LP64 */

#ifndef _WRS_CONFIG_LP64
#define SIZE_MAX    UINT32_MAX
#else
#define SIZE_MAX    UINT64_MAX
#endif /* _WRS_CONFIG_LP64 */

#endif /* INT8_MIN */



static __inline int
imax(int a, int b)
{
	return (a > b ? a : b);
}

#ifndef NULL

#if !defined(__cplusplus)
#define	NULL	((void *)0)
#else
#if __cplusplus >= 201103L
#define	NULL	nullptr
#elif defined(__GNUG__) && defined(__GNUC__) && __GNUC__ >= 4
#define	NULL	__null
#else
#if defined(__LP64__)
#define	NULL	(0L)
#else
#define	NULL	0
#endif	/* __LP64__ */
#endif	/* __GNUG__ */
#endif	/* !__cplusplus */

#endif

#endif  /* __ASSEMBLY__ */

#ifdef __cplusplus
}
#endif

#endif /* __INCstdinth */
