#ifndef _STDLIB_H_
#define _STDLIB_H_

extern void * kmalloc(unsigned long size);

extern void kfree(void * block);

extern unsigned long strtoul(const char *nptr, char **endptr, int base);

extern unsigned long long int strtoull(const char *ptr, char **end, int base);

#endif

