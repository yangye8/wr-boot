char * strstr
    (
    const char * s,        /* string to search */
    const char * find      /* substring to look for */
    )
    {
    char *t1;
    char *t2;
    char c;
    char c2;

    if ((c = *find++) == '\0')		/* <find> an empty string */
	return ((char *)(s));

    for(;;)
	{
	while (((c2 = *s++) != '\0') && (c2 != c))
	    ;

	if (c2 == '\0')
	    return (0);

	t1 = (char *)(s);
	t2 = (char *)(find); 

	while (((c2 = *t2++) != 0) && (*t1++ == c2))
	    ;

	if (c2 == '\0')
	    return ((char *)(s - 1));
	}
    }

