void * memcpy (void *dest, void * src, unsigned int length)
	{
	char * s = src;
	char * d = dest;
	
	while(length --) 
		*d++ = *s++;
	
	return dest;
	}
