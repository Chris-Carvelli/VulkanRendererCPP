#ifndef CC_HASH_H
#define CC_HASH_H

#include <stdint.h>

uint64_t SuperFastHash(const char * data, size_t len);
uint64_t Lookup2      (const char * data, size_t len);
uint64_t Lookup3      (const char * data, size_t len);
uint64_t CaseyHash    (const char * data, size_t len);
#endif