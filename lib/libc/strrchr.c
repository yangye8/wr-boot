/* strrchr.c - string search, string 
 * find the last occurrence of a character in a string (ANSI)
 *
 * This routine locates the last occurrence of <c> in the string pointed
 * to by <s>.  The terminating null is considered to be part of the string.
*/

char * strrchr
    (
    const char * s,         /* string to search */
    int          c          /* character to look for */
    )
    {
    char *save = ((void *) 0);

    do                  /* loop, searching for character */
    {
    if (*s == (char) c)
        save =  (char *) (s);
        } while (*s++ != '\0');

    return ( (char *)(save));
    }
