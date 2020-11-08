#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "tools.h"

uint8_t fletcher8(const unsigned char * data, size_t n)
{	
	uint8_t sum = 0xff, sumB = 0xff;
	unsigned char tlen;

	while (n) {
		tlen = n > 20 ? 20 : n;

		n -= tlen;
		do {
			sumB += sum += *data++;
			} while (--tlen);
			sum = (sum & 0xff) + (sum >> 8);
			sumB = (sumB & 0xff) + (sumB >> 8);
	}
	sum = (sum & 0xff) + (sum >> 8);
	sumB = (sumB & 0xff) + (sumB >> 8);

	return sumB << 8 | sum;
}

void  __attribute__((noreturn)) oom(void)
{
	puts("[!] out of memory\n");
	abort();
}

void *xmalloc(size_t size)
{
	void *p = malloc(size ? size : 1);
	if (p == NULL)
		oom();
	return p;
}

void *xcalloc(size_t nmemb, size_t size)
{
	void *p = nmemb && size ? calloc(nmemb, size) : malloc(1);
	if (p == NULL)
		oom();
	return p;
}

char *data2str(const unsigned char *data, size_t len)
{
	size_t i = 0;
	char *r, *p;

	r = xmalloc((len * 2 ) + 1);
	p = r;

	for (; i < len; i++) {
		if ((sprintf(p, "%02x", data[i])) != 2) {
			free(r);
			return NULL;
		}
		p += 2;
	}

	*p = '\0';

	return r;
}
