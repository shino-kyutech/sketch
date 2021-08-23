// 経過時間計測および使用メモリ表示
#include <sys/resource.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

double e_time(struct timespec *start, struct timespec *end);

// VmSize or VmRSS
void use_system(char *rs);
