char *(strchr)(const char *s, int c)
    {   /* find first occurrence of c in char s[] */
    const char ch = (char)c;

    for (; *s != ch; ++s)
        if (*s == '\0')
            return (0);
    return ((char *)s);
    }
