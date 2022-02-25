#ifndef PARALLE_MERGE_SORT_H
#define PARALLE_MERGE_SORT_H

#include "individual.h"
#include "arguments.h"

// merge two vectors of individuals
void merge(int start, int mid, int end, individual *generation);

// the actual merge sort function
void merge_sort(int start, int end, individual *generation);

// the first call of merge sort function
void parallel_merge_sort(struct arguments *args);

#endif
