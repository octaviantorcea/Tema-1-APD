#include <stdlib.h>

#include "parallel_merge_sort.h"
#include "genetic_algorithm.h"

void merge(int start, int mid, int end, individual *generation) {
	int size_left = mid - start + 1;
	int size_right = end - mid;

	individual *left_indv = (individual *) malloc(size_left * sizeof(individual));
	individual *right_indv = (individual *) malloc(size_right * sizeof(individual));

	// copy individuals from left vector
	for (int i = 0; i < size_left; i++) {
		left_indv[i] = generation[start + i];
	}

	// copy individuals from right vector
	for (int i = 0; i < size_right; i++) {
		right_indv[i] = generation[mid + i + 1];
	}

	int cursor = start;
	int i = 0;
	int j = 0;

	// sort them based on cmpfunc result
	while (i < size_left && j < size_right) {
		if (cmpfunc(left_indv + i, right_indv + j) < 0) {
			generation[cursor] = left_indv[i];
			cursor++;
			i++;
		} else {
			generation[cursor] = right_indv[j];
			cursor++;
			j++;
		}
	}

	while (i < size_left) {
		generation[cursor] = left_indv[i];
		cursor++;
		i++;
	}

	while (j < size_right) {
		generation[cursor] = right_indv[j];
		cursor++;
		j++;
	}

	free(left_indv);
	free(right_indv);
}

void merge_sort(int start, int end, individual *generation) {
	int mid = start + (end - start) / 2;

	if (start < end) {
		merge_sort(start, mid, generation);
		merge_sort(mid + 1, end, generation);
		merge(start, mid, end, generation);
	}
}

// every thread is responsible of sorting only a part of the vector;
// at the end, a single thread will merge all the parts together
void parallel_merge_sort(struct arguments *args) {
	// compute the start and end for threads
	int start = args->id * (double) args->object_count / args->nr_of_threads;
	int end = (args->id == args->nr_of_threads - 1) ?	// if it's the last thread, end == last element in the vector
				args->object_count - 1 :
				(args->id + 1) * (double) args->object_count / args->nr_of_threads - 1;

	int mid = start + (end - start) / 2;

	merge_sort(start, mid, args->current_generation);
	merge_sort(mid + 1, end, args->current_generation);
	merge(start, mid, end, args->current_generation);

	// wait for every thread to finish merge-sort
	pthread_barrier_wait(args->barrier);

	// because we split the vector at the start in nr_of_threads parts of
	// (nr_of_individuals / nr_of_threads) size, we need to merge those parts together at the end
	if (args->id == 0) {
		// we want only one thread to do the final merge
		for (int i = 1; i < args->nr_of_threads; i++) {
			merge(0,
				i * (double) args->object_count / args->nr_of_threads - 1,
				(i == args->nr_of_threads - 1) ?	// if it's the last merge, end == last element in the vector
					args->object_count - 1 :
					(i + 1) * (double) args->object_count / args->nr_of_threads - 1,
				args->current_generation);
		}
	}
}
