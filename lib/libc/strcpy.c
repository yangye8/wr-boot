char * strcpy
    (
    char *       s1,	/* string to copy to */
    const char * s2	/* string to copy from */
    )
    {
    char *save = s1;

    while ((*s1++ = *s2++) != '\0')
	;

    return (save);
    }
