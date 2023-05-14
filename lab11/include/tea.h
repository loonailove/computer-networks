#ifndef TEA_H
#define TEA_H

#include <stdint.h>
#include <stdlib.h>

#define MAX_PAD_SIZE 8
#define KEY_SIZE 16

uint8_t *encrypt(uint8_t *plaintext, uint32_t *size, const uint32_t *k);
uint8_t *decrypt(uint8_t *cipher, uint32_t *size, const uint32_t *k);

uint32_t *create_key();
void destroy_key(uint32_t *k);

#endif /* TEA_H */
