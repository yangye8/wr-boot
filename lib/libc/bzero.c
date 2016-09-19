#include <stdint.h>

#undef bzero			/* in case of LIBSA_USE_MEMSET */

void
bzero(void * dstv, size_t length)
{
	unsigned char *dst = dstv;

	while (length-- > 0) {
		*dst++ = 0;
	}
}

