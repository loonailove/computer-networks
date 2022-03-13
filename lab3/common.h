#include <stddef.h>
#include <stdint.h>

struct lab3_pkthdr {
	size_t len;
	uint16_t sum;
};

static inline uint8_t hamming_4to7(uint8_t c) {
    
}

static inline uint8_t hamming_7to4(uint8_t c) {
	// TODO: Hamming decode one nibbleuint16_t
}


uint16_t inet_csum(uint8_t *buf, size_t len);