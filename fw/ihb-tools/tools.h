#ifndef TOOLS_H
#define TOOLS_H

#include <stdio.h>
#include <stdlib.h>

/**
 * struct buffer_info:
 * @lenght lenght of buffer in bytes
 * @data its pointer
 */
struct buffer_info {
	size_t length;
	void **data;
};

void oom(void);
void *xmalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
void buffer_clean(struct buffer_info *b);
char *data2str(const unsigned char *data, size_t len);
uint8_t fletcher8(const unsigned char * data, size_t n);
#endif
