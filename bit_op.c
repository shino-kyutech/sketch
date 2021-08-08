#include <stdio.h>
#include <stdlib.h>
#include "bit_op.h"

#ifdef DEBUG
int main(void)
{
	unsigned long i;
	unsigned long a, b, c, d;
	unsigned long e[3] = {0}, f[3] = {0xa0a0a0a0a0a0a0a0, 0xffffffffffffffff, 0x5050505050505050};

	for(int i = 0; i < 192; i++) {
		printf("i = %d, bit(e) = %d, bit(f) = %d\n", i, bit_check_expanded(e, i), bit_check_expanded(f, i));
	}
/*
	make_bitcnt_tbl(8);
	for(int j = 0; j < 5; j++) {
		a = random() & 0xffff; b = random() & 0xffff; c = random() & 0xffff; d = random() & 0xffff;
		i = a | (b << 16) | (c << 32) | (d << 48);
		printf("i = %20lu (", i); print_bin_long(i); printf("), bit_count = %2d\n", bit_count_long(i));
		e[0] = i;
		a = random() & 0xffff; b = random() & 0xffff; c = random() & 0xffff; d = random() & 0xffff;
		i = a | (b << 16) | (c << 32) | (d << 48);
		printf("i = %20lu (", i); print_bin_long(i); printf("), bit_count = %2d\n", bit_count_long(i));
		e[1] = i;
		printf("####");
		print_bin_expanded(e, 2);
		printf("####\n");
		printf("bit_count_expand = %d\n", bit_count_expanded(e, 2));
//		int on = random() % 128;
		int on = 127;
		printf("on = %d\n", on);
		write_bit_expanded(0, 0, f);
		write_bit_expanded(on, 0, f);
		printf("####");
		print_bin_expanded(f, 2);
		printf("####\n");
		write_bit_expanded(on, 1, e);
		printf("####");
		print_bin_expanded(e, 2);
		printf("####\n");
		write_bit_expanded(on, 0, e);
		printf("####");
		print_bin_expanded(e, 2);
		printf("####\n");
	}
*/	
	return 0;
}
#endif

// ビット操作
void print_bin(unsigned int s)
{
	int i;
	for(i = 0; i < 32; i++)
		printf("%1d",(s >> (31 - i)) & 1);
}

void write_bit(int offset, int on_off, unsigned int *d)
{
	if(on_off == 1) {
		*d = *d | (1 << offset);
	} else {
		*d = *d & ~(1 << offset);
	}
}

// ビット操作（long用）
void print_bin_long(unsigned long s)
{
	int i;
	for(i = 0; i < 64; i++)
		printf("%1ld",(s >> (63 - i)) & 1);
}

void write_bit_long(int offset, int on_off, unsigned long *d)
{
	if(on_off == 1) {
		*d = *d | ((unsigned long)1 << offset);
	} else {
		*d = *d & ~((unsigned long)1 << offset);
	}
}

// ビット操作（expanded用）
void print_bin_expanded(unsigned long *s, int lg)
{
	for(int i = 0; i < lg; i++)
		print_bin_long(s[i]);
}

void write_bit_expanded(int offset, int on_off, unsigned long *d)
{
	int pos = offset / 64;
	offset = offset - pos * 64;
	if(on_off == 1) {
		d[pos] = d[pos] | ((unsigned long)1 << offset);
	} else {
		d[pos] = d[pos] & ~((unsigned long)1 << offset);
	}
}
int msb_pos(unsigned int x) {
  int pos = -1;
 
  if (x != 0) {
    __asm__("bsrl %1, %0": "=r" (pos): "m" (x));
  }
 
  return pos;
}

int lsb_pos(unsigned int x) {
  int pos = -1;
 
  if (x != 0) {
    __asm__("bsfl %1, %0": "=r" (pos): "m" (x));
  }
 
  return pos;
}

int msb_pos_long(unsigned long x) {
  long pos = -1;
 
  if (x != 0) {
    __asm__("bsrq %1, %0": "=r" (pos): "m" (x));
  }
 
  return pos;
}

int lsb_pos_long(unsigned long x) {
  long pos = -1;
 
  if (x != 0) {
    __asm__("bsfq %1, %0": "=r" (pos): "m" (x));
  }
 
  return pos;
}

// ビットカウントのため（ハミング距離で使用する）
int *bitcnt_tbl = NULL;

// width = 8 で十分
void make_bitcnt_tbl(int width)
{
	int BIT = (1 << width);
	int i;
	unsigned int v;

	if(bitcnt_tbl == NULL) {
		bitcnt_tbl = (int *)malloc(sizeof(int) * BIT);
	}
	for(i = 0; i < BIT; i++) {
		v = i;
		for (bitcnt_tbl[i] = 0; v; v >>= 1) {
			bitcnt_tbl[i] += v & 1;
		}
	}
}

int bit_count(unsigned int a)
{
	int c = 0;

	// if(sizeof(sketch_type) == sizeof(short))
	//	c = bitcnt_tbl[a]; // 16bit
	// else {
		//for(; a; a >>= 8)  // 64bit等にも対応　ただし遅い
		//	c += bitcnt_tbl[a & 0xff]; 
		c = bitcnt_tbl[a & 0xff] + bitcnt_tbl[(a >> 8) & 0xff] + bitcnt_tbl[(a >> 16) & 0xff] + bitcnt_tbl[(a >> 24) & 0xff]; // 32bit
	//}
	
	return c;
}

int bit_count_long(unsigned long a)
{
	int c = 0;

	c = bitcnt_tbl[a & 0xff] + bitcnt_tbl[(a >> 8) & 0xff] + bitcnt_tbl[(a >> 16) & 0xff] + bitcnt_tbl[(a >> 24) & 0xff]
	  + bitcnt_tbl[(a >> 32) & 0xff] + bitcnt_tbl[(a >> 40) & 0xff] + bitcnt_tbl[(a >> 48) & 0xff] + bitcnt_tbl[(a >> 56) & 0xff]; // 64bit

	return c;
}

int bit_count_expanded(unsigned long *a, int lg)
{
	int c = 0;
	for(int i = 0; i < lg; i++) {
		c += bit_count_long(a[i]);
	}

	return c;
}

int comp_bit(const void *a, const void *b)
{
	if(bitcnt_tbl[*((int *) a)] < bitcnt_tbl[*((int *) b)])
		return -1;
	else if(bitcnt_tbl[*((int *) a)] == bitcnt_tbl[*((int *) b)])
		return 0;
	else
		return 1;
}

int bit_check(unsigned a, int pos)
{
	unsigned m = 1;
	return (a & (m << pos) ? 1 : 0);
}

int bit_check_long(unsigned long a, int pos)
{
	unsigned long m = 1;
	return (a & (m << pos) ? 1 : 0);
}

int bit_check_expanded(unsigned long a[], int pos)
{
	int n = pos / 64;
	pos = pos - n * 64;
	unsigned long m = 1;
	return (a[n] & (m << pos) ? 1 : 0);
}

// ハミング距離順の列挙で使用するときに使用する
// ON_BITの昇順に並べておく
int *bitnum_pat = NULL;

void make_bitnum_pat(int width)
{
	int BIT = (1 << width);
	int i;

	if(bitnum_pat == NULL) {
		bitnum_pat = (int *)malloc(sizeof(int) * BIT);
	}
	for(i = 0; i < BIT; i++) {
		bitnum_pat[i] = i;
	}
	qsort(bitnum_pat, BIT, sizeof(int), comp_bit);
}

