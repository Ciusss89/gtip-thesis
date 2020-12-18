#ifndef TOOLS_H
#define TOOLS_H

#include <stdio.h>
#include <stdlib.h>

#define ATTR_UNUSED __attribute__((unused))
#define ATTR_NORET __attribute__((noreturn))

#define WAIT_2000ms	(2000LU * US_PER_MS)	/* delay of 2s */
#define WAIT_1000ms	(1000LU * US_PER_MS)	/* delay of 1s */
#define WAIT_500ms	(500LU * US_PER_MS)	/* delay of 500ms */
#define WAIT_100ms	(100LU * US_PER_MS)	/* delay of 100ms */
#define WAIT_20ms	(20LU * US_PER_MS)	/* delay of 020ms */
#define WAIT_10ms	(10LU * US_PER_MS)	/* delay of 010ms */
#define WAIT_5ms	(5LU * US_PER_MS)       /* delay of 005ms */

void ATTR_NORET oom(void);
void *xmalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
char *data2str(const unsigned char *data, size_t len);
uint8_t fletcher8(const unsigned char * data, size_t n);

#endif
