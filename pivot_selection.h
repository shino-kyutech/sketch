#include "sketch.h"
ftr_type *get_sample(struct_dataset *ds);
ftr_type get_median(struct_dataset *ds);
void select_pivot_QBP(pivot_type *pivot, ftr_type median, ftr_type sample[], struct_dataset *ds, int nt);
void select_pivot_random_QBP(pivot_type *pivot, ftr_type median, struct_dataset *ds);
double collision(sketch_type sk[]);
// wide スケッチ用は，まずは，QBP のみで 
// optimize_pivot_by_precision // LS and AIR
