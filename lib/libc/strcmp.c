int strcmp(const char * s1, const char * s2)
    {
    while (*s1++ == *s2++)
	if (s1 [-1] == '\0')
	    return (0);

    return ((int)((unsigned char)s1 [-1] - (unsigned char)s2 [-1]));
    }
