#ifndef _STDIO_H_
#define _STDIO_H_

#include <stdarg.h>

#define ENOMEM      12  /* Out of Memory */
#define EINVAL      22  /* Invalid argument */
#define ENOSPC      28  /* No space left on device */
#define EIO         5   /* Input/output error */
#define EROFS       30  /* Read-only filesystem */
extern void putc (const char c);

extern int printf(const char *fmt, ...);

#define fprintf(fmt, args...)   printf(args)

extern int sprintf(char *buf, const char *fmt, ...);

extern int vsprintf(char *buf, const char *fmt, va_list args);

extern int printk(const char * frmt,...);

extern int kprintf(const char *fmt, ...);

#endif  /* _STDIO_H_ */

