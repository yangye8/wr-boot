/*******************************************************************************
*
* strdup - duplicate a string
*
* This routine takes a string and copies this string allocated in kernel memory.
* A pointer to the newly copies string is returned.
*
* RETURNS: new allocated string or NULL on error.
*
*/
#include <string.h>
#include <stdlib.h>

char * strdup (const char *str)
    {
    unsigned long len = 0;
    char *retStr;

    if (str == 0)
        {
        return (0);
        }

    len = strlen (str);

    retStr = (char *) kmalloc (len+1);

    if (retStr == 0)
        {
        return (0);
        }

    strcpy (retStr, str);

    return (retStr);
    }
