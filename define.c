#include <stdio.h>

//typedef enum {MAIN_MEMORY, SECONDARY_MEMORY} ftr_on;

#if !(defined(MAIN_MEMORY) ^ defined(SECONDARY_MEMORY))
#error "MAIN_MEMORY and SECONDARY_MEMORY should be exclusive."
#endif

typedef enum {EUCLID, L_1} metric;
#ifndef L_P
#define L_P 1.2
#endif

int main(void)
{
	#if defined(MAIN_MEMORY)
		printf("FTR ON MAIN MEMORY\n");
	#elif defined(SECONDARY_MEMORY)
		printf("FTR ON SECONDARY MEMORY\n");
	#endif
	
	#if METRIC == EUCLID
	printf("EUCLID\n");
	#elif METRIC == L_P
	printf("L_P = %lf\n", L_P);
	#elif METRIC == L_1
	printf("L_1\n");
	#else
	printf("OTHER\n");
	#endif
	
	return 0;

}