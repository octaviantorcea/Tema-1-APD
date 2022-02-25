build:
	gcc -o tema1_par tema1_par.c genetic_algorithm.c parallel_merge_sort.c -Wall -Werror -lm -lpthread
clean:
	rm -rf tema1_par
