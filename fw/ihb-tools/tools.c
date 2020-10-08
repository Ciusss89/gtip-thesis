#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

/* Riot API */
#include "periph/pm.h"

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

void oom(void)
{
	puts("[!] out of memory\n");
	pm_reboot();
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
	char *r, *p;
	size_t i;

	r = xmalloc((len * 2 ) + 1);
	p = r;

	for (i=0; i < len; i++) {
		sprintf(p, "%02x", data[i]);
		p += 2;
	}

	*p = '\0';

	return r;
}
