#ifndef DH_H
#define DH_H

#include <stdint.h>

// chosen arbitrarily
/*#define PUBLIC_PRIME 5006909*/
/*#define PUBLIC_ROOT  10001*/

#define PUBLIC_PRIME 23
#define PUBLIC_ROOT  5


uint32_t mod_pow(uint32_t b, uint32_t e, uint32_t m);
uint32_t generate_secret(uint32_t p);
uint32_t *derive_key(uint32_t shared_secret);

#endif /* DH_H */
