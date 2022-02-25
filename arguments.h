#ifndef ARGUMEMTS_H
#define ARGUMEMTS_H

#include <pthread.h>
#include "sack_object.h"
#include "individual.h"

// structure for the argument of the thread function
struct arguments {
	sack_object *objects;
	int object_count;
	int sack_capacity;
	int generations_count;
	int nr_of_threads;
	int id;

    individual *current_generation;
    individual *next_generation;

    pthread_barrier_t *barrier;
};

#endif
