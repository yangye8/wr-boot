/* string.h - standalone C string library header file */

#ifndef __INCstringh
#define __INCstringh

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern void bcopy(const void *, void *, size_t);
extern void bzero(void *, size_t);
extern void bfill(register char *buf, size_t nbytes, register int ch);

extern size_t strlen(const char *);
extern char * strcpy(char * __restrict, const char * __restrict);
extern char * strchr (const char *__s, int __c);
extern int  strcmp (const char *__s1, const char *__s2);
extern size_t strlen (const char *__s);
extern int  strncmp (const char *__s1, const char *__s2, size_t __n);
extern char * strstr (const char *__s1, const char *__s2);
extern char *(strchr)(const char *s, int c);
extern char *(strncpy)(char * s1, const char * s2, unsigned int n);
extern char * strrchr (const char * s, int c);
extern char * strdup (const char *str);
extern char * strpbrk (const char * s1,const char * s2);
extern int strspn (const char * s,const char * sep);
extern char * strcat(char *p1, char* p2);

extern void * memset(void *, int, size_t);
extern void * memchr(const void *s, int c, size_t n);
extern void * memcpy(void *, const void *, size_t);
extern void * memmove(void *, const void *, size_t);
extern int    memcmp(const void *, const void *, size_t);

#define bcmp(a, b, c) memcmp(a, b, c)

#ifdef __cplusplus
}
#endif

#endif /* __INCstringh */
