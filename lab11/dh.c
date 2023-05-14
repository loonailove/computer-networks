#include "dh.h"
#include "tea.h"

#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#include "utils.h"

uint32_t generate_secret(uint32_t p)
{
	int fd = open("/dev/urandom", O_RDONLY);
	DIE(fd == -1, "open /dev/urandom");

	uint32_t secret;
	int res = read(fd, &secret, sizeof(secret));
	DIE(res == -1, "read");

	return secret % p;
}

uint32_t mod_pow(uint32_t b, uint32_t e, uint32_t m)
{
	if (m == 1)
		return 0;

	uint64_t c = 1;
	for (uint32_t i = 0; i < e; ++i)
		c = (c * b) % m;

	return c;
}

uint32_t *derive_key(uint32_t shared_secret)
{
	/*
	 * NOTE: This is an unbelievably bad key derivation function; it
	 * effectively reduces the key size to a quarter;
	 * we use it here simply because it's easy to follow!
	 */
	uint32_t *key = malloc(KEY_SIZE);
	for (int i = 0; i < KEY_SIZE / sizeof(uint32_t); ++i)
		key[i] = shared_secret;

	return key;
}
