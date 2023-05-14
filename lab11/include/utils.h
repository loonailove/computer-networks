#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>

void hex_dump(const void * addr, size_t size);

#define DIE(condition, message, ...) \
do { \
	if ((condition)) { \
		fprintf(stderr, "[(%s:%d)]: " # message "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
		perror(""); \
		exit(1); \
	} \
} while (0)

#endif /* UTILS_H */
