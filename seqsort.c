// COMP3230 Programming Assignment Two
// The sequential version of the sorting using qsort

/*
# Filename: 
# Student name and No.:
# Development platform:
# Remark:
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

int checking(unsigned int *, long);
int compare(const void *, const void *);

// global variables
long size;  // size of the array
unsigned int * intarr; // array of random integers

int main (int argc, char **argv)
{
	long i, j;
	struct timeval start, end;

	if ((argc != 2))
	{
		printf("Usage: seq_sort <number>\n");
		exit(0);
	}
	  
	size = atol(argv[1]);
	intarr = (unsigned int *)malloc(size*sizeof(unsigned int));
	if (intarr == NULL) {perror("malloc"); exit(0); }
	
	// set the random seed for generating a fixed random
	// sequence across different runs
  char * env = getenv("RANNUM");  //get the env variable
  if (!env)                       //if not exists
		srandom(3230);
	else
		srandom(atol(env));
	
	for (i=0; i<size; i++) {
		intarr[i] = random();
	}
	
	// measure the start time
	gettimeofday(&start, NULL);
	
	// just call the qsort library
	// replace qsort by your parallel sorting algorithm using pthread
	qsort(intarr, size, sizeof(unsigned int), compare);

	// measure the end time
	gettimeofday(&end, NULL);
	
	if (!checking(intarr, size)) {
		printf("The array is not in sorted order!!\n");
	}
	
	printf("Total elapsed time: %.4f s\n", (end.tv_sec - start.tv_sec)*1.0 + (end.tv_usec - start.tv_usec)/1000000.0);
	  
	free(intarr);
	return 0;
}

int compare(const void * a, const void * b) {
	return (*(unsigned int *)a>*(unsigned int *)b) ? 1 : ((*(unsigned int *)a==*(unsigned int *)b) ? 0 : -1);
}

int checking(unsigned int * list, long size) {
	long i;
	printf("First : %d\n", list[0]);
	printf("At 25%%: %d\n", list[size/4]);
	printf("At 50%%: %d\n", list[size/2]);
	printf("At 75%%: %d\n", list[3*size/4]);
	printf("Last  : %d\n", list[size-1]);
	for (i=0; i<size-1; i++) {
		if (list[i] > list[i+1]) {
		  return 0;
		}
	}
	return 1;
}