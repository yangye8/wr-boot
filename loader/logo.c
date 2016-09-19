#include <wrboot.h>

extern void printf(char * frmt,...);
const char *creationDate = __DATE__ ", " __TIME__;

void banner(void)
    {
    printf("  _    __      ____              __  __                    __         \n");
    printf(" | |  / /     / __ )____  ____  / /_/ /   ____  ____ _____/ /__  _____\n"); 
    printf(" | | / /_____/ __  / __ \\/ __ \\/ __/ /   / __ \\/ __ `/ __  / _ \\/ ___/\n");
    printf(" | |/ /_____/ /_/ / /_/ / /_/ / /_/ /___/ /_/ / /_/ / /_/ /  __/ /    \n");
    printf(" |___/     /_____/\\____/\\____/\\__/_____/\\____/\\__,_/\\__,_/\\___/_/     \n");
    printf("\r\n");
    printf ("Build date: %s\n",creationDate);
    printf ("Author: Yang Ye\n");
    printf ("Email: goodwillyang@163.com\n");
    printf("\r\n");
    }
