#include <stdint.h>

size_t strlen (const char * s)
    {
    const char *save = s + 1;

    while (*s++ != '\0')
	;

    return (s - save);
    }
