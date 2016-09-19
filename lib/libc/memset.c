void *  memset(void * p, unsigned int v, unsigned int length)
	{
	char *s = p;

	while(length--)
	*s++ = v;

	return p;
	}
	