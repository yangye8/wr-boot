int strncmp
    (
    const char * s1,           	/* string to compare */
    const char * s2,           	/* string to compare <s1> to */
    unsigned int       n             	/* max no. of characters to compare */
    )
    {
    if (n == 0)
	return (0);

    while (*s1++ == *s2++)
	{
	if ((s1 [-1] == '\0') || (--n == 0))
	    return (0);
        }

    return ((int)((unsigned char)s1 [-1] - (unsigned char)s2 [-1]));
    }
