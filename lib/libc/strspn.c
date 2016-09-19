/******************************************************************************
*
* strspn - return the string length up to the first character not in a given set (ANSI)
*
* This routine computes the length of the maximum initial segment of
* string <s> that consists entirely of characters from the string <sep>.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* The length of the string segment.
*
* SEE ALSO: strcspn()
*/
 
int strspn
    (
    const char * s,             /* string to search */
    const char * sep            /* set of characters to look for in <s> */
    )
    {
    const char *save;
    const char *p;
    char c1;
    char c2;

    for (save = s + 1; (c1 = *s++) != '\0'; )
    for (p = sep; (c2 = *p++) != c1; )
        {
        if (c2 == '\0')
        return (s - save);
            }

    return (s - save);
    }
