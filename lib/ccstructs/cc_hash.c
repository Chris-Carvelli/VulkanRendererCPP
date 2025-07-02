#include <cc_hash.h>

#include <lookup3.c>

#include <intrin.h>
#include <emmintrin.h>
#include <wmmintrin.h>  //for intrinsics for AES-NI


// chris: this is a hashing library, we expect to do weird tricks with bits and ugly casting
//        let's assume we know what we are doing
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wcast-align"

/*
* Code from https://www.azillionmonkeys.com/qed/hash.html
* license: LGPL 2.1
* modified to get rid of variable size integers
*/
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

uint64_t SuperFastHash (const char * data, size_t len) {
    uint64_t hash = len, tmp;
    int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
    case 3: hash += get16bits (data);
        hash ^= hash << 16;
        hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
        hash += hash >> 11;
        break;
    case 2: hash += get16bits (data);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
    case 1: hash += (signed char)*data;
        hash ^= hash << 10;
        hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

uint64_t Lookup2 (const char * data, size_t len) {
    uint32_t pc, pb;
    hashlittle2(data, len, &pc, &pb);

    return pb;
}

uint64_t Lookup3 (const char * data, size_t len) {
    uint32_t pc, pb;
    hashlittle2(data, len, &pc, &pb);

    return pc + ((uint64_t)pb << 32);
}

static char unsigned OverhangMask[32] =
{
    255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0
};
static char unsigned DefaultSeed[16] =
{
    178, 201,  95, 240,  40,  41, 143, 216,
      2, 209, 178, 114, 232,   4, 176, 188
};

uint64_t CaseyHash(const char *At, size_t Count)
{
    /* TODO(casey):

    Consider and test some alternate hash designs.  The hash here
    was the simplest thing to type in, but it is not necessarily
    the best hash for the job.  It may be that less AES rounds
    would produce equivalently collision-free results for the
    problem space.  It may be that non-AES hashing would be
    better.  Some careful analysis would be nice.
    */

    // TODO(casey): Does the result of a grapheme composition
    // depend on whether or not it was RTL or LTR?  Or are there
    // no fonts that ever get used in both directions, so it doesn't
    // matter?

    // TODO(casey): Double-check exactly the pattern
    // we want to use for the hash here

    __m128i Result = {0};

    // TODO(casey): Should there be an IV?
    __m128i HashValue = _mm_cvtsi64_si128(Count);
    HashValue = _mm_xor_si128(HashValue, _mm_loadu_si128((__m128i *)DefaultSeed));

    size_t ChunkCount = Count / 16;
    while(ChunkCount--)
    {
        __m128i In = _mm_loadu_si128((const __m128i *)At);
        At += 16;

        HashValue = _mm_xor_si128(HashValue, In);
        HashValue = _mm_aesdec_si128(HashValue, _mm_setzero_si128());
        HashValue = _mm_aesdec_si128(HashValue, _mm_setzero_si128());
        HashValue = _mm_aesdec_si128(HashValue, _mm_setzero_si128());
        HashValue = _mm_aesdec_si128(HashValue, _mm_setzero_si128());
    }

    size_t Overhang = Count % 16;


#if 0
    __m128i In = _mm_loadu_si128((__m128i *)At);
#else
    // TODO(casey): This needs to be improved - it's too slow, and the #if 0 branch would be nice but can't
    // work because of overrun, etc.
    unsigned char Temp[16];
    __movsb(
        (unsigned char *)Temp, 
        (const unsigned char *)At, 
        Overhang
    );
    __m128i In = _mm_loadu_si128((__m128i *)Temp);
#endif
    In = _mm_and_si128(In, _mm_loadu_si128((__m128i *)(OverhangMask + 16 - Overhang)));
    HashValue = _mm_xor_si128(HashValue, In);
    HashValue = _mm_aesdec_si128(HashValue, _mm_setzero_si128());
    HashValue = _mm_aesdec_si128(HashValue, _mm_setzero_si128());
    HashValue = _mm_aesdec_si128(HashValue, _mm_setzero_si128());
    HashValue = _mm_aesdec_si128(HashValue, _mm_setzero_si128());

    Result = HashValue;

    uint64_t ret = Result[0];

    return ret;
}