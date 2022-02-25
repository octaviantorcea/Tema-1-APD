#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "genetic_algorithm.h"
#include "parallel_merge_sort.h"

int minimum(int a, int b) {
	return (a < b) ? a : b;
}

int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int *nr_of_threads,
				 int argc, char *argv[]) {
	FILE *fp;

	if (argc < 4) {
		fprintf(stderr, "Usage:\n\t./tema1_par in_file generations_count nr_threads\n");
		return 0;
	}

	fp = fopen(argv[1], "r");

	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);
	
	if (*generations_count == 0) {
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	// get the number of threads
	*nr_of_threads = atoi(argv[3]);

	return 1;
}

void print_best_fitness(const individual *generation) {
	printf("%d\n", generation[0].fitness);
}

void compute_fitness_function(const sack_object *objects, individual *generation, int sack_capacity, int start, int end) {
	int weight;
	int profit;

	for (int i = start; i < end; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

int cmpfunc(const void *a, const void *b) {
	int i;
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = 0, second_count = 0;

		for (i = 0; i < first->chromosome_length && i < second->chromosome_length; ++i) {
			first_count += first->chromosomes[i];
			second_count += second->chromosomes[i];
		}

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			return second->index - first->index;
		}
	}

	return res;
}

void mutate_bit_string_1(const individual *ind, int generation_index) {
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index) {
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index) {
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count,
			parent2->chromosomes + count,
			(parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count,
			parent1->chromosomes + count,
			(parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to) {
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation, int t_id, int nr_of_threads) {
	int i;
	int start = t_id * (double) generation->chromosome_length / nr_of_threads;
	int end = minimum(((t_id + 1) * (double) generation->chromosome_length / nr_of_threads), generation->chromosome_length);

	for (i = start; i < end; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}

void *run_genetic_algorithm(void *argument) {
	struct arguments *args = (struct arguments *) argument;

	int count, cursor;
	individual *tmp = NULL;

	int start = args->id * (double) args->object_count / args->nr_of_threads;
	int end = minimum((args->id + 1) * (double) args->object_count / args->nr_of_threads, args->object_count);

	int start_elite;
	int end_elite;

	int start_mutation;
	int end_mutation;

	int start_crossover;
	int end_crossover;

	// set initial generation (composed of object_count individuals with a single item in the sack)
	for (int i = start; i < end; ++i) {
		args->current_generation[i].fitness = 0;
		args->current_generation[i].chromosomes = (int*) calloc(args->object_count, sizeof(int));
		args->current_generation[i].chromosomes[i] = 1;
		args->current_generation[i].index = i;
		args->current_generation[i].chromosome_length = args->object_count;

		args->next_generation[i].fitness = 0;
		args->next_generation[i].chromosomes = (int*) calloc(args->object_count, sizeof(int));
		args->next_generation[i].index = i;
		args->next_generation[i].chromosome_length = args->object_count;
	}

	// need to wait for every thread to finish initialization and memory allocation 
	pthread_barrier_wait(args->barrier);

	// iterate for each generation
	for (int k = 0; k < args->generations_count; ++k) {
		cursor = 0;

		// ----- COMPUTE FITNESS ----- //
		compute_fitness_function(args->objects, args->current_generation, args->sack_capacity, start, end);

		// we wait for every thread to finish the fitness function
		pthread_barrier_wait(args->barrier);

		// ----- PARALLEL MERGESORT ----- //
		parallel_merge_sort(args);

		// wait for merge sort to finish
		pthread_barrier_wait(args->barrier);

		// ----- ELITE CHILDREN SELECTION (first 30%) ----- //
		count = args->object_count * 3 / 10;

		// compute new start and end
		start_elite = args->id * (double) count / args->nr_of_threads;
		end_elite = minimum((args->id + 1) * (double) count / args->nr_of_threads, count);
			
		for (int i = start_elite; i < end_elite; ++i) {
			copy_individual(args->current_generation + i, args->next_generation + i);
		}
		cursor = count;

		// wait for threads to select elite children
		pthread_barrier_wait(args->barrier);

		// ----- MUTATE FIRST 20% CHILDREN WITH THE FIRST VERSION OF BIT STRING MUTATION ----- //
		count = args->object_count * 2 / 10;

		// compute new start and end
		start_mutation = args->id * (double) count / args->nr_of_threads;
		end_mutation = minimum((args->id + 1) * (double) count / args->nr_of_threads, count);

		for (int i = start_mutation; i < end_mutation; ++i) {
			copy_individual(args->current_generation + i, args->next_generation + cursor + i);
			mutate_bit_string_1(args->next_generation + cursor + i, k);
		}

		cursor += count;

		// wait for threads to mutate first 20% children
		pthread_barrier_wait(args->barrier);

		// ----- MUTATE NEXT 20% CHILDREN WITH THE SECOND VERSION OF BIT STRING MUTATION ----- //
		count = args->object_count * 2 / 10;

		for (int i = start_mutation; i < end_mutation; ++i) {
			copy_individual(args->current_generation + i + count, args->next_generation + cursor + i);
			mutate_bit_string_2(args->next_generation + cursor + i, k);
		}

		cursor += count;

		// wait for threads to mutate next 20% children
		pthread_barrier_wait(args->barrier);

		// ----- CROSSOVER FIRST 30% PARENTS WITH ONE-POINT CROSSOVER ----- //
		count = args->object_count * 3 / 10;

		// (if there is an odd number of parents for each thread, the last child from the first 30% is kept as such)
		if (count % (args->nr_of_threads * 2) != 0) {
			if (args->id == 0) { // we need only one thread to do this
				copy_individual(args->current_generation + args->object_count - 1,
								args->next_generation + cursor + count - 1);
			}

			count--;
		}

		// wait for the first thread to copy the last individual
		pthread_barrier_wait(args->barrier);

		// compute new start and end
		start_crossover = args->id * (double) count / args->nr_of_threads;
		end_crossover = minimum((args->id + 1) * (double) count / args->nr_of_threads, count);

		for (int i = start_crossover; i < end_crossover; i += 2) {
			crossover(args->current_generation + i, args->next_generation + cursor + i, k);
		}

		// wait for all threads to finish
		pthread_barrier_wait(args->barrier);

		// ----- SWITCH GENERATIONS ----- //
		if (args->id == 0) {
			// we only need one thread
			tmp = args->current_generation;
			args->current_generation = args->next_generation;
			args->next_generation = tmp;
		}

		// wait for the first thread to finish switching generations
		pthread_barrier_wait(args->barrier);

		// ----- RESET -----
		for (int i = start; i < end; ++i) {
			args->current_generation[i].index = i;
		}

		// wait for all threads
		pthread_barrier_wait(args->barrier);

		// ----- PRINT ONCE EVERY 5 GENERATIONS -----
		if (k % 5 == 0 && args->id == 0) {
			print_best_fitness(args->current_generation);
		}
	}

	// compute fitness for last generation
	compute_fitness_function(args->objects, args->current_generation, args->sack_capacity, start, end);

	// we wait for every thread to finish the fitness function
	pthread_barrier_wait(args->barrier);

	// last sort
	parallel_merge_sort(args);

	// wait for merge sort to finish
	pthread_barrier_wait(args->barrier);

	if (args->id == 0) {
		print_best_fitness(args->current_generation);
	}

	// wait for print
	pthread_barrier_wait(args->barrier);

	// free resources
	free_generation(args->current_generation, args->id, args->nr_of_threads);
	free_generation(args->next_generation, args->id, args->nr_of_threads);

	return NULL;
}
