#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include "vartype.h"
#include "util.h"

void serialize_uint64(FILE *out, const uint64_t x)
{
	/* assumes little-endian (or, at least, that the binary
	   dictionary files will be generated by the same machine
	   as the one on which they are used) */
	assert(fwrite(&x, sizeof(uint64_t), 1, out));
}

void serialize_uint32(FILE *out, const uint32_t x)
{
	/* assumes little-endian (or, at least, that the binary
	   dictionary files will be generated by the same machine
	   as the one on which they are used) */
	assert(fwrite(&x, sizeof(uint32_t), 1, out));
}

void serialize_uint8(FILE *out, const uint8_t x)
{
	assert(fwrite(&x, sizeof(uint8_t), 1, out));
}

uint64_t read_uint64(FILE *in)
{
	uint64_t x;
	assert(fread(&x, sizeof(uint64_t), 1, in) == 1);
	return x;
}

uint32_t read_uint32(FILE *in)
{
	uint32_t x;
	assert(fread(&x, sizeof(uint32_t), 1, in) == 1);
	return x;
}

uint8_t read_uint8(FILE *in)
{
	uint8_t x;
	assert(fread(&x, sizeof(uint8_t), 1, in) == 1);
	return x;
}

int kmer_cmp(const void *p1, const void *p2)
{
	const kmer_t kmer1 = ((struct kmer_info *)p1)->kmer;
	const kmer_t kmer2 = ((struct kmer_info *)p2)->kmer;
	return (kmer1 > kmer2) - (kmer1 < kmer2);
}

int snp_kmer_cmp(const void *p1, const void *p2)
{
	const kmer_t kmer1 = ((struct snp_kmer_info *)p1)->kmer;
	const kmer_t kmer2 = ((struct snp_kmer_info *)p2)->kmer;
	return (kmer1 > kmer2) - (kmer1 < kmer2);
}

uint64_t encode_base(const char base)
{
	switch (base) {
	case 'A':
	case 'a':
		return BASE_A;
	case 'C':
	case 'c':
		return BASE_C;
	case 'G':
	case 'g':
		return BASE_G;
	case 'T':
	case 't':
		return BASE_T;
	case 'N':
	case 'n':
		return BASE_N;
	default:
		return BASE_X;
	}
}

static char rev(const char c) {
    switch (c) {
        case 'A':
        case 'a':
            return 'T';
        case 'C':
        case 'c':
            return 'G';
        case 'G':
        case 'g':
            return 'C';
        case 'T':
        case 't':
            return 'A';
        default:
            return 'N';
    }
}

char* minimizer(const char *kmer, uint32_t *offset) {
    char seq[SSL];
    char reverse[SSL];
    char min[K];
    char tmpseq[K];
    char tmprev[K];
    for(int i = 0; i < SSL; i++) {
        seq[i] = (char) *(kmer + i);
        reverse[SSL - i - 1] = rev(seq[i]);
        if(i < K) {
            min[i] = 'Z';
        }
    }
    for(int i = 0; i < SSL - K + 1; i++){
        for(int j = 0; j < K; j++){
            tmpseq[j] = seq[i + j];
            tmprev[j] = reverse[i + j];
        }
        if(strcmp(tmpseq, min) < 0) {
            strcpy(min, tmpseq);
            *offset = i;
        }
        if(strcmp(tmprev, min) < 0) {
            strcpy(min, tmprev);
            *offset = i;
        }
    }
    free(seq);
    free(reverse);
    free(min);
    free(tmprev);
    free(tmpseq);
    return min;
}

char* minimizerSNP(const char *kmer, unsigned int index, char var, uint32_t *offset) {
    char seq[SSL];
    char reverse[SSL];
    char min[K];
    char tmpseq[K];
    char tmprev[K];
    for(int i = 0; i < SSL; i++) {
        seq[i] = (char) *(kmer + i);
        reverse[SSL - i - 1] = rev(seq[i]);
        if(i < K) {
            min[i] = 'Z';
        }
    }
    seq[SSL - 1 - index] = var;
    reverse[index] = rev(var);
    for(int i = 0; i < SSL - K + 1; i++){
        if(index > (SSL - K - 1) && index < (K - 1)) {
            for(int j = 0; j < K; j++) {
                tmpseq[j] = seq[i + j];
                tmprev[j] = reverse[i + j];
            }
        }
        else if(index < (SSL - K)) {
            // Variation -> SSL - K character ending the sequence. => 0 <= index < SSL - K
            if(i + K >= SSL - index) {
                for(int j = 0; j < K; j++) {
                    tmpseq[j] = seq[i + j];
                }
            }
            if(i <= index) {
                for(int j = 0; j < K; j++) {
                    tmprev[j] = reverse[i + j];
                }
            }
        }
        else {
            // Variation -> SSL - K character starting the sequence. => K - 1 < index < SSL
            if(i + K >= SSL - index) {
                for(int j = 0; j < K; j++) {
                    tmprev[j] = reverse[i + j];
                }
            }
            if(i <= index) {
                for(int j = 0; j < K; j++) {
                    tmpseq[j] = seq[i + j];
                }
            }
        }
        if(strcmp(tmpseq, min) < 0) {
            strcpy(min, tmpseq);
            *offset = i;
        }
        if(strcmp(tmprev, min) < 0) {
            strcpy(min, tmprev);
            *offset = i;
        }
    }
    free(seq);
    free(reverse);
    free(min);
    free(tmprev);
    free(tmpseq);
    return min;
}

kmer_t encode_kmer(const char *kmer, bool *kmer_had_n)
{
#define KMER_ADD_BASE(x) (encoded_kmer |= (x))

	kmer_t encoded_kmer = 0UL;
	char *base = (char *)&kmer[31];
	for (int i = 0; i < 32; i++) {
		encoded_kmer <<= 2;
		switch (*base--) {
		case 'A': case 'a': KMER_ADD_BASE(0UL); break;
		case 'C': case 'c': KMER_ADD_BASE(1UL); break;
		case 'G': case 'g': KMER_ADD_BASE(2UL); break;
		case 'T': case 't': KMER_ADD_BASE(3UL); break;
		case 'N': case 'n': *kmer_had_n = true; return 0;
		default: assert(0); break;
		}
	}

	*kmer_had_n = false;
	return encoded_kmer;

#undef KMER_ADD_BASE
}

kmer_t shift_kmer(const kmer_t kmer, const char next_base)
{
#define KMER_SHIFT(x) ((kmer >> 2) | ((x) << 62))

	switch (next_base) {
	case 'A': case 'a': return KMER_SHIFT(0UL);
	case 'C': case 'c': return KMER_SHIFT(1UL);
	case 'G': case 'g': return KMER_SHIFT(2UL);
	case 'T': case 't': return KMER_SHIFT(3UL);
	default: assert(0); break;
	}
	return 0;

#undef KMER_SHIFT
}

unsigned kmer_get_base(const kmer_t kmer, unsigned base)
{
	unsigned s = 2*base;
	return ((kmer & (0x3UL << s)) >> s);
}

/*
 * Adapted from:
 * https://github.com/yunwilliamyu/quartz/blob/master/misra_gries_dict.cpp#L106
 */
kmer_t rev_compl(const kmer_t orig)
{
	static uint16_t table[0x10000] = {};
	static bool init = false;
	if (!init) {
		uint16_t x;
		uint16_t y;
		for (unsigned i = 0; i < 0x10000; i++) {
			x = 0;
			y = i;
			for (unsigned int j = 0; j < 8; j++) {
				x <<= 2;
				switch (y & 3) {
					case 0:
						x+=3;
						break;
					case 1:
						x+=2;
						break;
					case 2:
						x+=1;
						break;
					case 3:
						break;
				}
				y >>= 2;
			}
			table[i] = x;
		}
		init = true;
	}

	kmer_t ans;
	uint16_t *c_orig = (uint16_t*)(&orig);
	uint16_t *c_ans = (uint16_t*)(&ans);

	for (unsigned i = 0; i < 4; i++) {
		c_ans[i] = table[c_orig[3-i]];
	}

	return ans;
}

void decode_kmer(const kmer_t kmer, char *buf)
{
	static const char bases[] = {'A', 'C', 'G', 'T'};
	for (unsigned i = 0; i < 32; i++) {
		buf[i] = bases[kmer_get_base(kmer, i)];
	}
	buf[32] = '\0';
}

void split_line(const char *str, char **out)
{
	char *p = (char *)str;
	size_t i = 0;
	while (*p) {
		out[i++] = p;
		while (*p != '\t' && *p != '\n') ++p;
		++p;
	}
	out[i] = NULL;
}

void copy_until_space(char *dest, const char *src)
{
	size_t i = 0;
	while (!isspace(src[i])) {
		dest[i] = src[i];
		++i;
	}
	dest[i] = '\0';
}

bool equal_up_to_space(const char *a, const char *b)
{
	for (size_t i = 0; a[i] && b[i] && !isspace(a[i]) && !isspace(b[i]); i++) {
		if (a[i] != b[i])
			return false;
	}
	return true;
}

