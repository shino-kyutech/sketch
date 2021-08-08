// ビット操作
void print_bin(unsigned int s);
void write_bit(int offset, int on_off, unsigned int *d);
void print_bin_long(unsigned long s);
void write_bit_long(int offset, int on_off, unsigned long *d);
void print_bin_expanded(unsigned long *s, int lg);
void write_bit_expanded(int offset, int on_off, unsigned long *d);
int msb_pos(unsigned int x);
int lsb_pos(unsigned int x);
int msb_pos_long(unsigned long x);
int lsb_pos_long(unsigned long x);
void make_bitcnt_tbl(int width);
int bit_count(unsigned int a);
int bit_count_long(unsigned long a);
int bit_count_expanded(unsigned long *a, int lg);
int bit_check(unsigned a, int pos);
int bit_check_long(unsigned long a, int pos);
int bit_check_expanded(unsigned long a[], int pos);
int comp_bit(const void *a, const void *b);
void make_bitnum_pat(int width);

