#include "stdarg.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define MAXNBUF (sizeof( long ) * 8 + 1)

extern void puts (const char *s);

char const hex2ascii_data[] = "0123456789abcdefghijklmnopqrstuvwxyz";

#define hex2ascii(hex)  (hex2ascii_data[hex])

#define toupper(c)      ((c) - 0x20 * (((c) >= 'a') && ((c) <= 'z')))

char *convert(unsigned int num, int base)
    {
    static char buff[33];
    char *ptr;
    
    ptr=&buff[sizeof(buff)-1];
    
    *ptr='\0';
    
    do
        {
        *--ptr="0123456789abcdef"[num%base];
        num/=base;
        }while(num!=0);
    
    return(ptr);
    } 

void printf2(char * frmt,...)
{
    char *p; 
    int i;
    unsigned u;
    char *s; 
    va_list argp;
    
    va_start(argp, frmt);

    p=frmt;
    
    for(p=frmt; *p!='\0';p++)
        {
        if(*p!='%')
            {
            putc(*p);
            continue;
            }

        p++;

        switch(*p)
            {
            case 'c' : 
                i=va_arg(argp,int);
                putc(i);
                break;
            case 'd' : 
                i=va_arg(argp,int);

            if(i<0)
                {
                i=-i;
                putc('-');
                }
                
                puts(convert(i,10));
                break;

            case 'o': 
                i=va_arg(argp,unsigned int); 
                puts(convert(i,8));
                break;

            case 's': 
                s=va_arg(argp,char *); 
                puts(s); 
                break;
            case 'u': 
                u=va_arg(argp, unsigned int); 
                puts(convert(u,10));
                break;
            case 'x': 
                u=va_arg(argp, unsigned int); 
                puts(convert(u,16));
                break;
            case '%': 
                putc('%');
                break;
            }
        }

    va_end(argp);
}

/*
 * Put a NUL-terminated ASCII number (base <= 36) in a buffer in reverse
 * order; return an optional length and a pointer to the last character
 * written in the buffer (i.e., the first character of the string).
 * The buffer pointed to by `nbuf' must have length >= MAXNBUF.
 */
static char * ksprintn(char *nbuf, unsigned  long   num, int base, int *lenp, int upper)
{
    char *p, c;

    p = nbuf;
    *p = '\0';
    do {
        c = hex2ascii(num % base);
        *++p = upper ? toupper(c) : c;
    } while (num /= base);
    if (lenp)
        *lenp = p - nbuf;
    return (p);
}

int kvprintf(char const *fmt, void (*func)(int, void*), void *arg, int radix, va_list ap)
{
#define PCHAR(c) {int cc=(c); if (func) (*func)(cc,arg); else *d++ = cc; retval++; }
    char nbuf[MAXNBUF];
    char *d;
    const char *p, *percent, *q;
    unsigned char *up;
    int ch, n;
    unsigned  long  num;
    int base, lflag, qflag, tmp, width, ladjust, sharpflag, neg, sign, dot;
    int cflag, hflag, jflag, tflag, zflag;
    int dwidth, upper;
    char padc;
    int stop = 0, retval = 0;

    num = 0;
    if (!func)
        d = (char *) arg;
    else
        d = 0;

    if (fmt == 0)
        fmt = "(fmt null)\n";

    if (radix < 2 || radix > 36)
        radix = 10;

    for (;;) {
        padc = ' ';
        width = 0;
        while ((ch = (unsigned char)*fmt++) != '%' || stop) {
            if (ch == '\0')
                return (retval);
            PCHAR(ch);
        }
        percent = fmt - 1;
        qflag = 0; lflag = 0; ladjust = 0; sharpflag = 0; neg = 0;
        sign = 0; dot = 0; dwidth = 0; upper = 0;
        cflag = 0; hflag = 0; jflag = 0; tflag = 0; zflag = 0;
reswitch:   switch (ch = (unsigned char)*fmt++) {
        case '.':
            dot = 1;
            goto reswitch;
        case '#':
            sharpflag = 1;
            goto reswitch;
        case '+':
            sign = 1;
            goto reswitch;
        case '-':
            ladjust = 1;
            goto reswitch;
        case '%':
            PCHAR(ch);
            break;
        case '*':
            if (!dot) {
                width = va_arg(ap, int);
                if (width < 0) {
                    ladjust = !ladjust;
                    width = -width;
                }
            } else {
                dwidth = va_arg(ap, int);
            }
            goto reswitch;
        case '0':
            if (!dot) {
                padc = '0';
                goto reswitch;
            }
        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
                for (n = 0;; ++fmt) {
                    n = n * 10 + ch - '0';
                    ch = *fmt;
                    if (ch < '0' || ch > '9')
                        break;
                }
            if (dot)
                dwidth = n;
            else
                width = n;
            goto reswitch;
        case 'b':
            num = (unsigned int)va_arg(ap, int);
            p = va_arg(ap, char *);
            for (q = ksprintn(nbuf, num, *p++, 0, 0); *q;)
                PCHAR(*q--);

            if (num == 0)
                break;

            for (tmp = 0; *p;) {
                n = *p++;
                if (num & (1 << (n - 1))) {
                    PCHAR(tmp ? ',' : '<');
                    for (; (n = *p) > ' '; ++p)
                        PCHAR(n);
                    tmp = 1;
                } else
                    for (; *p > ' '; ++p)
                        continue;
            }
            if (tmp)
                PCHAR('>');
            break;
        case 'c':
            width -= 1;

            if (!ladjust && width > 0)
                while (width--)
                    PCHAR(padc);
            PCHAR(va_arg(ap, int));
            if (ladjust && width > 0)
                while (width--)
                    PCHAR(padc);
            break;
        case 'D':
            up = va_arg(ap, unsigned char *);
            p = va_arg(ap, char *);
            if (!width)
                width = 16;
            while(width--) {
                PCHAR(hex2ascii(*up >> 4));
                PCHAR(hex2ascii(*up & 0x0f));
                up++;
                if (width)
                    for (q=p;*q;q++)
                        PCHAR(*q);
            }
            break;
        case 'd':
        case 'i':
            base = 10;
            sign = 1;
            goto handle_sign;
        case 'h':
            if (hflag) {
                hflag = 0;
                cflag = 1;
            } else
                hflag = 1;
            goto reswitch;
        case 'j':
            jflag = 1;
            goto reswitch;
        case 'l':
            if (lflag) {
                lflag = 0;
                qflag = 1;
            } else
                lflag = 1;
            goto reswitch;
        case 'n':
            if (jflag)
                *(va_arg(ap,  long  *)) = retval;
            else if (qflag)
                *(va_arg(ap,  long  *)) = retval;
            else if (lflag)
                *(va_arg(ap, long *)) = retval;
            else if (zflag)
                *(va_arg(ap, size_t *)) = retval;
            else if (hflag)
                *(va_arg(ap, short *)) = retval;
            else if (cflag)
                *(va_arg(ap, char *)) = retval;
            else
                *(va_arg(ap, int *)) = retval;
            break;
        case 'o':
            base = 8;
            goto handle_nosign;
        case 'p':
            base = 16;
            sharpflag = (width == 0);
            sign = 0;
            num = (uintptr_t)va_arg(ap, void *);
            goto number;
        case 'q':
            qflag = 1;
            goto reswitch;
        case 'r':
            base = radix;
            if (sign)
                goto handle_sign;
            goto handle_nosign;
        case 's':
            p = va_arg(ap, char *);
            if (p == 0)
                p = "(null)";
            if (!dot)
                n = strlen (p);
            else
                for (n = 0; n < dwidth && p[n]; n++)
                    continue;

            width -= n;

            if (!ladjust && width > 0)
                while (width--)
                    PCHAR(padc);
            while (n--)
                PCHAR(*p++);
            if (ladjust && width > 0)
                while (width--)
                    PCHAR(padc);
            break;
        case 't':
            tflag = 1;
            goto reswitch;
        case 'u':
            base = 10;
            goto handle_nosign;
        case 'X':
            upper = 1;
        case 'x':
            base = 16;
            goto handle_nosign;
        case 'y':
            base = 16;
            sign = 1;
            goto handle_sign;
        case 'z':
            zflag = 1;
            goto reswitch;
handle_nosign:
            sign = 0;
            if (jflag)
                num = va_arg(ap, unsigned  long );
            else if (qflag)
                num = va_arg(ap, unsigned  long );
            else if (tflag)
                num = va_arg(ap, int);
            else if (lflag)
                num = va_arg(ap, unsigned long);
            else if (zflag)
                num = va_arg(ap, size_t);
            else if (hflag)
                num = (unsigned short)va_arg(ap, int);
            else if (cflag)
                num = (unsigned char)va_arg(ap, int);
            else
                num = va_arg(ap, unsigned int);
            goto number;
handle_sign:
            if (jflag)
                num = va_arg(ap,  long );
            else if (qflag)
                num = va_arg(ap,  long );
            else if (tflag)
                num = va_arg(ap, int);
            else if (lflag)
                num = va_arg(ap, long);
            else if (zflag)
                num = va_arg(ap, int);
            else if (hflag)
                num = (short)va_arg(ap, int);
            else if (cflag)
                num = (char)va_arg(ap, int);
            else
                num = va_arg(ap, int);
number:
            if (sign && ( long )num < 0) {
                neg = 1;
                num = -( long )num;
            }
            p = ksprintn(nbuf, num, base, &n, upper);
            tmp = 0;
            if (sharpflag && num != 0) {
                if (base == 8)
                    tmp++;
                else if (base == 16)
                    tmp += 2;
            }
            if (neg)
                tmp++;

            if (!ladjust && padc == '0')
                dwidth = width - tmp;
            width -= tmp + imax(dwidth, n);
            dwidth -= n;
            if (!ladjust)
                while (width-- > 0)
                    PCHAR(' ');
            if (neg)
                PCHAR('-');
            if (sharpflag && num != 0) {
                if (base == 8) {
                    PCHAR('0');
                } else if (base == 16) {
                    PCHAR('0');
                    PCHAR('x');
                }
            }
            while (dwidth-- > 0)
                PCHAR('0');

            while (*p)
                PCHAR(*p--);

            if (ladjust)
                while (width-- > 0)
                    PCHAR(' ');

            break;
        default:
            while (percent < fmt)
                PCHAR(*percent++);
            /*
             * Since we ignore a formatting argument it is no
             * longer safe to obey the remaining formatting
             * arguments as the arguments will no longer match
             * the format specs.
             */
            stop = 1;
            break;
        }
    }
#undef PCHAR
}


#if 0
int sprintf(char *buf, const char *cfmt, ...)
{
    int retval;
    va_list ap;

    va_start(ap, cfmt);
    retval = kvprintf(cfmt, 0, (void *)buf, 10, ap);
    buf[retval] = '\0';
    va_end(ap);
    return (retval);
}
#endif

