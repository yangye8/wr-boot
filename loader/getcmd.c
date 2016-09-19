#include <stdio.h>
#include <string.h>

#define CTL_CH(c)               ((c) - 'a' + 1)
#define MAX_CMDBUF_SIZE          256

extern int getc (void);

void getcmd(char * cmd_buf)
{
    unsigned char curpos = 0;    /* current position - index into cmd_buf */
    char c;

    for (;;) 
    {
        c = getc();
        switch (c) 
        {
        case 0x08:
        case 0x06:
        case 0x07:
        case 0x7E:
        case 0x7F:  /* backspace or delete */
            /* we're not at the beginning of the line */
            if (curpos) {
                curpos--;
                putc(0x08); /* go backwards */
                putc(' ');  /* overwrite the char */
                putc(0x08); /* go back again */
            }
            cmd_buf[curpos] = '\0';
            break;
        case '\r':
        case '\n':
        case '\0':
            putc('\r');
            putc('\n');
            goto end_cmd;
        case CTL_CH('x'):
            curpos = 0;
            break;
        default:
            if (curpos < MAX_CMDBUF_SIZE) 
            {
                cmd_buf[curpos++] = c;
                /* echo it back out to the screen */
                putc(c);
            }
            break;
        }
    }
end_cmd:

    return;
}
