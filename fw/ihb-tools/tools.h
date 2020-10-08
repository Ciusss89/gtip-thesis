#ifndef TOOLS_H
#define TOOLS_H

#include <stdio.h>
#include <stdlib.h>

void oom(void);
void *xmalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
char *data2str(const unsigned char *data, size_t len);
uint8_t fletcher8(const unsigned char * data, size_t n);
#endif
