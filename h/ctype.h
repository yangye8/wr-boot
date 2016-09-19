#ifndef _LINUX_CTYPE_H
#define _LINUX_CTYPE_H

/*
 * NOTE! This ctype does not handle EOF like the standard C
 * library is required to.
 */

#define _U	0x01	/* upper */
#define _L	0x02	/* lower */
#define _D	0x04	/* digit */
#define _C	0x08	/* cntrl */
#define _P	0x10	/* punct */
#define _S	0x20	/* white space (space/lf/tab) */
#define _X	0x40	/* hex digit */
#define _SP	0x80	/* hard space (0x20) */

extern const unsigned char _ctype[];

#define __ismask(x) (_ctype[(int)(unsigned char)(x)])

#define isalnum(c)	((__ismask(c)&(_U|_L|_D)) != 0)
#define isalpha(c)	((__ismask(c)&(_U|_L)) != 0)
#define iscntrl(c)	((__ismask(c)&(_C)) != 0)
#define isdigit(c)	((__ismask(c)&(_D)) != 0)
#define isgraph(c)	((__ismask(c)&(_P|_U|_L|_D)) != 0)
#define islower(c)	((__ismask(c)&(_L)) != 0)
#define isprint(c)	((__ismask(c)&(_P|_U|_L|_D|_SP)) != 0)
#define ispunct(c)	((__ismask(c)&(_P)) != 0)
/* Note: isspace() must return false for %NUL-terminator */
#define isspace(c)	((__ismask(c)&(_S)) != 0)
#define isupper(c)	((__ismask(c)&(_U)) != 0)
#define isxdigit(c)	((__ismask(c)&(_D|_X)) != 0)

#define isascii(c) (((unsigned char)(c))<=0x7f)
#define toascii(c) (((unsigned char)(c))&0x7f)

static inline unsigned char __tolower(unsigned char c)
{
	if (isupper(c))
		c -= 'A'-'a';
	return c;
}

static inline unsigned char __toupper(unsigned char c)
{
	if (islower(c))
		c -= 'a'-'A';
	return c;
}

#define tolower(c) __tolower(c)
#define toupper(c) __toupper(c)

/*
 * Fast implementation of tolower() for internal usage. Do not use in your
 * code.
 */
static inline char _tolower(const char c)
{
	return c | 0x20;
}

/* Fast check for octal digit */
static inline int isodigit(const char c)
{
	return c >= '0' && c <= '7';
}


#ifndef __DECONST
#define __DECONST(type, var)    ((type)(unsigned int )(const void *)(var))
#endif

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
#ifndef _KERNEL
typedef	unsigned short	ushort;		/* Sys V compatibility */
typedef	unsigned int	uint;		/* Sys V compatibility */
#endif

typedef unsigned int size_t;

/*
 * Basic types upon which most other types are built.
 */
typedef	signed char		__int8_t;
typedef	unsigned char		__uint8_t;
typedef	short			__int16_t;
typedef	unsigned short		__uint16_t;

typedef	int			__int32_t;
typedef __int32_t  ssize_t;

typedef	unsigned int		__uint32_t;
#ifdef __LP64__
typedef	long			__int64_t;
typedef	unsigned long		__uint64_t;
#else
#ifndef lint
__extension__
#endif

/* LONGLONG */

typedef	long long		__int64_t;
typedef __int64_t      loff_t;


#ifndef lint
__extension__
#endif

/* LONGLONG */

typedef	unsigned long long	__uint64_t;
#endif

typedef __uint8_t	u_int8_t;	/* unsigned integrals (deprecated) */
typedef u_int8_t  __u8;

typedef __uint16_t	u_int16_t;
typedef u_int16_t   __u16;

typedef __uint32_t	u_int32_t;
typedef u_int32_t  __u32;

typedef __uint64_t	u_int64_t;
typedef u_int64_t  __u64;

typedef	__uint64_t	u_quad_t;	/* quads (deprecated) */
typedef	__int64_t	quad_t;
typedef	quad_t *	qaddr_t;

typedef	char *		caddr_t;	/* core address */
typedef	const char *	c_caddr_t;	/* core address, pointer to const */

#endif
