#include "ctype.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <wrboot.h>

static char hex_to_ascii_table[16] = "0123456789ABCDEF";

/*
 * Simple print string
 */

void putnstr(const char *str, size_t n)
{
        if (str == 0)
                return;

        while (n && *str != '\0') {
                putc(*str);
                str++;
                n--;
        }
}

void putstr(const char *str)
{
        putnstr(str, strlen(str));
}

void u32todecimal(char *buf, unsigned long x)
{
        int i = 0;
        int j = 0;
        char localbuf[16];

        if (x != 0) {
                while (x > 0) {
                        unsigned long rem = x % 10;
                        localbuf[i++] = hex_to_ascii_table[rem];
                        x /= 10;
                }
                /* now reverse the characters into buf */
                while (i > 0) {
                        i--;
                        buf[j++] = localbuf[i];
                }
                buf[j] = '\0';
        } else {
                buf[0] = '0';
                buf[1] = '\0';
        }
}

void binarytohex(char *buf, long x, int nbytes)
{
        int i;
        int s = 4*(2*nbytes - 1);
        if (hex_to_ascii_table[0] != '0')
                putstr("hex_to_ascii_table corrupted\r\n");
        for (i = 0; i < 2*nbytes; i++){
                buf[i] = hex_to_ascii_table[(x >> s) & 0xf];
                s -= 4;
        }
        buf[2*nbytes] = 0;
}


/*
 *
 */
unsigned long strtoul2(const char *str, char **endptr, int requestedbase, int *ret)
{
        unsigned long num = 0;
        char c;
        unsigned char digit;
        int base = 10;
        int nchars = 0;
        int leadingZero = 0;

        *ret = 0;

        while ((c = *str) != 0) {
                if (nchars == 0 && c == '0') {
                        leadingZero = 1;
                        goto step;
                } else if (leadingZero && nchars == 1) {
                        if (c == 'x') {
                                base = 16;
                                goto step;
                        } else if (c == 'o') {
                                base = 8;
                                goto step;
                        }
                }
                if (c >= '0' && c <= '9') {
                        digit = c - '0';
                } else if (c >= 'a' && c <= 'z') {
                        digit = c - 'a' + 10;
                } else if (c >= 'A' && c <= 'Z') {
                        digit = c - 'A' + 10;
                } else {
                        *ret = 3;
                        return 0;
                }
                if (digit >= base) {
                        *ret = 4;
                        return 0;
                }
                num *= base;
                num += digit;
step:
                str++;
                nchars++;

        }
        return num;
}

void putstr_hex(const char *str, unsigned long value)
{
        char buf[9];
        binarytohex(buf, value, 4);
        putstr(str);
        putstr(buf);
        putstr("\r\n");
}

int printmem (unsigned int address,unsigned int numBytes)
    {
    int     i;
    unsigned int  endAddr;
    volatile unsigned int  addr;
 
    endAddr = address+numBytes;
    addr=address;
    do
        {
        printf("%08lx: ", addr);
        for (i=0; i<4; i++)
            {
            printf(" %08lx", *((unsigned int *)addr));
            addr += 4;
            }
        printf("\n");
        } while (addr<endAddr);

    return 0;
    }


int md(unsigned char *addr, int size)
{
    unsigned char* i;
    int      size16Aling = (size>>4)<<4;
    int      size4Aling  = (size>>2)<<2;
    int      notAlign    = 0;

    if (size%16)
        notAlign = 1;

    for (i = addr; (unsigned int)i < (unsigned int)addr + size16Aling; i+=16)
        printf("0x%x: %x %x %x %x\r\n",
                i,
                *((unsigned int*)(i)),
                *((unsigned int*)(i+4)),
                *((unsigned int*)(i+8)),
                *((unsigned int*)(i+12)));
    if (notAlign)
        printf( "0x%x: ", i);

    for (; (unsigned int)i < (unsigned int)addr + size4Aling; i+=4)
        printf("%x ", *((unsigned int*)(i)));

    for (; (unsigned int)i < (unsigned int)addr + size; i++)
        printf("%x", *((unsigned char*)(i)));

    if (notAlign)
        printf("\r\n");
    return 0;
}


/*
 * dump hex
 */
#define BL_ISPRINT(ch)      (((ch) >= ' ') && ((ch) < 128))
int hexdump(unsigned char *data, size_t num)
{
    int i;
    long oldNum;
    char buf[90];
    char *bufp;
    int line_resid;

    while (num) {
        bufp = buf;
        binarytohex(bufp, (unsigned long)data, 4);
        bufp += 8;
        *bufp++ = ':';
        *bufp++ = ' ';

        oldNum = num;

        for (i = 0; i < 16 && num; i++, num--) {
            binarytohex(bufp, (unsigned long)data[i], 1);
            bufp += 2;
            *bufp++ = (i == 7) ? '-' : ' ';
        }

        line_resid = (16 - i) * 3;
        if (line_resid) {
            memset(bufp, ' ', line_resid);
            bufp += line_resid;
        }

        memcpy(bufp, "| ", 2);
        bufp += 2;

        for (i = 0; i < 16 && oldNum; i++, oldNum--)
            *bufp++ = BL_ISPRINT(data[i]) ? data[i] : '.';

        line_resid = 16 - i;
        if (line_resid) {
            memset(bufp, ' ', 16 - i);
            bufp += 16 - i;
        }

        *bufp++ = '\r';
        *bufp++ = '\n';
        *bufp++ = '\0';
        putstr(buf);
        data += 16;
    }
    return 0;
}

unsigned int bsd_sum_memory(unsigned long img_src, size_t img_size) 
{
    unsigned long checksum = 0;   /* The checksum mod 2^16. */
    unsigned char *pch;       /* Each character read. */
    size_t i;

    pch = (unsigned char *)img_src;
    for (i = 1; i <= img_size; i++) {
        /* Do a right rotate */
        if (checksum & 01)
            checksum = (checksum >> 1) + 0x8000; 
        else    
            checksum >>= 1;
        checksum += *pch;      /* add the value to the checksum */
        checksum &= 0xffff;  /* Keep it within bounds. */
        pch++;  
    }
    return(checksum & 0xffff);
}

void progress_bar(unsigned long cur, unsigned long max)
{
        int percent, full_percent, count = 0, i;

        full_percent = (cur * 100) / max;
        percent = full_percent - (full_percent % 5);


    /* |====================|(   %) */

    if (cur) {
        for (i = 0; i < 28; i++) {
            printf("\b");
        }
    }

    printf("|");
    count = percent / 5;
    for (i = 0;  i < count; i++) 
        printf("=");
    for (i = 0; i < (20 - count); i++)
        printf(" ");
    printf("|(");
    printf("%3ld", full_percent);
    printf("%)");
}


unsigned long mem_compare(const char *to, const char *from, size_t len, int echo)
{
    unsigned long ofs = 0;

    if (echo) progress_bar(ofs, len + ofs);
    while (len >= sizeof(unsigned long)) {
        if (*(unsigned long *)(to + ofs) != *(unsigned long *)(from + ofs))
            return ofs;

        len -= sizeof(unsigned long);
        ofs += sizeof(unsigned long);
    }
    if (echo) progress_bar(ofs, len + ofs);

    if (len > 0) {
        if (*(to + ofs) != *(from + ofs));
            return ofs;
        len -= sizeof(char);
        ofs += sizeof(char);
    }
    if (echo) progress_bar(ofs, len + ofs);

    return ofs;
}

size_t mem_copy(void *to, const void *from, size_t len, int echo)
{
    ulong ofs = 0;

    while (len >= sizeof(unsigned long)) {
        *(unsigned long *)(to + ofs) = *(unsigned long *)(from + ofs);
        len -= sizeof(unsigned long);
        ofs += sizeof(unsigned long);
    }

    if (len > 0) {
        (*(char *)(to + ofs) = *(char *)(from + ofs));
        len -= sizeof(char);
        ofs += sizeof(char);
    }

    return ofs;
}

void search_value(unsigned long *start, unsigned long *end, 
                         unsigned long value)
{
    volatile unsigned long *addr;
    while ((end - start) != 0) {
        addr = start;
        if (*addr == value) 
            printf("address = 0x%08lx\n", start);
        start++;
    }
}


/*
 * return a pointer to a string containing the size
 *"as xxx KiB", "xxx.y KiB", "xxx MiB", "xxx.y MiB",
 * xxx GiB, xxx.y GiB, etc as needed;
 */
char *size_human_readable(unsigned long long size)
    {
    static char buf[30];
    unsigned long m = 0, n;
    unsigned long long f;
    static const char names[] = {'E', 'P', 'T', 'G', 'M', 'K'};
    unsigned long d = 10 * ARRAY_SIZE(names);
    char c = 0;
    unsigned int i;
    char *ptr = buf;
    for (i = 0; i < ARRAY_SIZE(names); i++, d -= 10) {
        if (size >> d) {
            c = names[i];
            break;
        }
    }

    if (!c) {
        sprintf(buf, "%llu Bytes", size);
        return buf;
    }

    n = size >> d;
    f = size & ((1ULL << d) - 1);

    /* If there's a remainder, deal with it */
    if (f) {
        m = (10ULL * f + (1ULL << (d - 1))) >> d;

        if (m >= 10) {
            m -= 10;
            n += 1;
        }
    }

    ptr += sprintf(buf, "%lu", n);
    if (m) {
        ptr += sprintf(ptr, ".%lu", m);
    }
    sprintf(ptr, " %ciB", c);

    return buf;
    }
