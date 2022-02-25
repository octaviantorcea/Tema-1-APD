#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include "arguments.h"
#include "genetic_algorithm.h"

int main(int argc, char *argv[]) {
	int err;

	// threads vector
	pthread_t *threads;

	//barrier
	pthread_barrier_t *bar;
	
	// arguments for the thread function
	struct arguments *args;
	
	// generations
	individual *current_generation;
	individual *next_generation;

	// array with all the objects that can be placed in the sack
	sack_object *objects = NULL;

	// number of objects
	int object_count = 0;

	// maximum weight that can be carried in the sack
	int sack_capacity = 0;

	// number of generations
	int generations_count = 0;

	// number of threads
	int nr_of_threads = 0;

	if (!read_input(&objects, &object_count, &sack_capacity, &generations_count, &nr_of_threads, argc, argv)) {
		return 0;
	}

	// memory allocation
	current_generation = (individual *) calloc(object_count, sizeof(individual));
	next_generation = (individual *) calloc(object_count, sizeof(individual));

	threads = (pthread_t *) malloc(nr_of_threads * sizeof(pthread_t));
	args = (struct arguments *) malloc(nr_of_threads * sizeof(struct arguments));
	bar = (pthread_barrier_t *) malloc(sizeof(pthread_barrier_t));
	
	// barrier init
	err = pthread_barrier_init(bar, NULL, nr_of_threads);

	if (err) {
		printf("Error: can't init barrier.\n");
		return 0;
	}

	// create threads
	for (int i = 0; i < nr_of_threads; i++) {
		args[i].id = i;
		args[i].generations_count = generations_count;
		args[i].nr_of_threads = nr_of_threads;
		args[i].object_count = object_count;
		args[i].objects = objects;
		args[i].sack_capacity = sack_capacity;
		args[i].current_generation = current_generation;
		args[i].next_generation = next_generation;
		args[i].barrier = bar;

		err = pthread_create(&threads[i], NULL, run_genetic_algorithm, &args[i]);
	
		if (err) {
			printf("Error: can't create thread nr %d.\n", i);
			return 0;
		}
	}

	// join threads
	for (int i = 0; i < nr_of_threads; i++) {
		err = pthread_join(threads[i], NULL);

		if (err) {
			printf("Error: can't join thread nr %d.\n", i);
			return 0;
		}
	}

	// barrier destroy
	pthread_barrier_destroy(bar);

	// free allocated memory
	free(bar);
	free(objects);
	free(threads);
	free(current_generation);
	free(next_generation);
	free(args);

	return 0;
}
