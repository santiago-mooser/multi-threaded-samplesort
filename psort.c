// COMP3230 Programming Assignment Two
// The sequential version of the sorting using qsort

/*
# Filename: 				psort.c
# Student name and No.: 	3035557399
# Development platform:		Arch Linux 5.15.77-1-lts x86_64
# GCC version:				gcc (GCC) 12.2.0
# Remark:
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h> // Header file for pthreads
#include <semaphore.h> // Header file for semaphores

int checking(unsigned int *, long);
int compare(const void *, const void *);
int* psort(unsigned int * intarr, long size, int num_of_threads);
void* sorter_thread(void * arg);
// global variables
long size;  // size of the array
unsigned int * intarr; // array of random integers
int num_of_threads; // number of threads

// Strucure to contain the arguments for the threads
struct thread_args {
	int thread_id;
	int arr_size;
	unsigned int * intarr; // This holds the original array
	sem_t * child_to_main; // this semaphore allows the child to signal the main thread that it is done with phase 1
	sem_t * main_to_child; // this semaphore allows the main thread to signal the children that it is done with phase 2
	sem_t * subarr_moving_phase; // this semaphore allows the children to signal to the main thread that they are done moving the subarrays
	sem_t * sorting_phase; // this semaphore allows the main thread to signal the children that all threads are done with the partition exchange and they may start sorting their subarrays (phase 3)
	int * samples; // this array holds the samples from the children
	sem_t * samples_lock; // this semaphore locks the samples array so that only one thread can access it at a time
	int * pivots;	// this array holds the pivots from the main thread
	int ** subarrays; // These subarrays hold the subarrays of the children
	int * subarray_sizes; // This array holds the sizes of the subarrays of the children
	sem_t * subarray_lock; // This array holds the locks for the subarrays
	int * final_array;
	sem_t * final_array_lock;
};

int main (int argc, char **argv)
{
	long i;
	struct timeval start, end;

	if ((argc < 2) || (argc > 3))
	{
		printf("Usage: seq_sort <number>\n");
		exit(0);
	}

	size = atol(argv[1]);
	intarr = (unsigned int *)malloc(size*sizeof(unsigned int)+1);
	if (intarr == NULL) {perror("malloc"); exit(0); }

	if ((argc = 3))
	{
		num_of_threads = atoi(argv[2]);
		if ((num_of_threads == 1) || (num_of_threads < 1)){
			printf("Number of threads must be greater than 1\n");
			exit(1);
		}
	}

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
	intarr = psort(intarr, size, num_of_threads);

	// measure the end time
	gettimeofday(&end, NULL);

	if (!checking(intarr, size)) {
		printf("The array is not in sorted order!!\n");
	}

	printf("Total elapsed time: %.4f s", (end.tv_sec - start.tv_sec)*1.0 + (end.tv_usec - start.tv_usec)/1000000.0);

	return 0;
}

// Get an int array, a size int, and a numf_of_threads int
// return pointer to the sorted array
int *psort(unsigned int * intarr, long arrsize, int num_of_threads)
{
	// Create an array of pthreads
	pthread_t threads[num_of_threads];

	// Create a semaphore for the main thread to wait on the threads:
	sem_t child_to_main;
	sem_t main_to_child;
	sem_t subarr_moving_phase;
	sem_t sorting_phase;

	// Initialize binary semaphore with pshared = 1 to allow main thread to communicate with children
	sem_init(&child_to_main, 1, 0);
	sem_init(&main_to_child, 1, 0);
	sem_init(&subarr_moving_phase, 1, 0);
	sem_init(&sorting_phase, 1, 0);

	// Create an array of subarrays
	int ** subarrays = (int **)malloc(num_of_threads * sizeof(int *));
	if (subarrays == NULL) {perror("malloc"); exit(0); }

	// Create one mutex lock per thread to allow communication between the threads
	sem_t subarray_lock;
	sem_init(&subarray_lock, 1, 1);

	// Create an array of subarray sizes
	int * subarray_sizes = (int *)malloc(num_of_threads* sizeof(int)+1);
	//intialize all subarray sizes to 0
	for (int i = 0; i < num_of_threads; i++)
	{
		subarray_sizes[i] = 0;
	}

	// Create an array for the structs used as thread arguments
	struct thread_args thread_args[num_of_threads];

	// Initialize the array used for the samples from the threads
	int number_of_samples = num_of_threads * num_of_threads;
	int * samples = (int *)malloc(number_of_samples * sizeof(int)+1);
	if (samples == NULL) {perror("malloc"); exit(1); }

	// Create a mutex lock for the samples array
	sem_t samples_lock;
	sem_init(&samples_lock, 1, 1);

	// Initliaze the pivots array
	int * pivots = malloc(num_of_threads * sizeof(int));
	if (pivots == NULL) {perror("malloc"); exit(1); }

	// intialize the final array
	// int * final_array = malloc(arrsize * sizeof(int));
	// if (final_array == NULL) {perror("malloc"); exit(1); }

	// as well as the final array lock
	sem_t final_array_lock;
	sem_init(&final_array_lock, 1, 1);

	// instantiate the structs for the threads and create the threads
	for (int i = 0; i < num_of_threads; i++)
	{
		// Initialize the structure
		struct thread_args args = {
			.thread_id 	= i,
			.arr_size 	= arrsize,
			.intarr 	= intarr,

			.samples	= samples,
			.samples_lock 	= &samples_lock,

			.child_to_main 	= &child_to_main,
			.main_to_child 	= &main_to_child,

			.pivots 		= pivots,

			.subarr_moving_phase = &subarr_moving_phase,
			.sorting_phase = &sorting_phase,

			.subarrays = subarrays,
			.subarray_sizes = subarray_sizes,
			.subarray_lock = &subarray_lock,

			.final_array = intarr,
		};
		thread_args[i] = args;

		pthread_create(&threads[i], NULL, sorter_thread, (void *)&thread_args[i]);
	}

	// Get the value of the semaphore
	// int sem_value;
	// Wait for all threads to finish using the semaphores
	// printf("Waiting for all threads to finish phase 1\n");
	for (int i = 0; i < num_of_threads; i++)
	{
		sem_wait(&child_to_main);
	}

	samples = thread_args[0].samples;

	// Once all threads are done, we simply sort the samples array
	// qsort the samples
	qsort(samples, number_of_samples, sizeof(int), compare);


	// Now that we have sorted the samples, we can calculate the pivots
	//The pivot values are selected at indices p+⌊p/2⌋-1, 2p+⌊2p/2⌋-1, …, (p−1)p+⌊(p−1)p/2⌋-1, where p is the number of threads.
	int index = 0;
	for (int i = 0; i < num_of_threads-1; i++)
	{
		index = (i+1)*num_of_threads + num_of_threads/2 - 1;
		pivots[i] = samples[index];
	}

	// Since the threads already have a pointer to the pivots array,
	// we just nee to release the threads to continue
	for (int i = 0; i < num_of_threads; i++)
	{
		sem_post(&main_to_child);
	}

	// Wait for all the threads to finish moving their subarrays
	// printf("\n\nWaiting for all threads to finish moving their subarrays\n\n");
	for (int i = 0; i < num_of_threads; i++)
	{
		sem_wait(&subarr_moving_phase);
	}

	// printf("\nAll threads have finished moving their subarrays\n\n");

	// Tell the threads they can now sort their subarrays
	for (int i = 0; i < num_of_threads; i++)
	{
		// printf("Releasing thread %d to tell it to sort its subarray\n", i);
		sem_post(&sorting_phase);
	}

	// thread join to wait for the threads to finish
	for (int i = 0; i < num_of_threads; i++)
	{
		pthread_join(threads[i], NULL);
	}
	free(thread_args->subarrays);
	free(thread_args->subarray_sizes);
	// free(thread_args->intarr);
	return intarr;
}


void * sorter_thread(void * args)
{
	// Cast the args to the struct
	struct thread_args * thread_args = (struct thread_args *)args;

	// Cast to semaphore and mutex lock in structure
	sem_t * child_to_main = thread_args->child_to_main;
	sem_t * main_to_child = thread_args->main_to_child;
	sem_t * subarr_moving_phase = thread_args->subarr_moving_phase;
	sem_t * sorting_phase = thread_args->sorting_phase;
	sem_t * subarray_lock = thread_args->subarray_lock;

	// Calculate the start index and end index in the intarr for this thread so we can
	// create the subarray for this thread
	int subarray_start_index;
	int subarray_end_index;

	int thread_work = 0;
	int remaining_work = thread_args->arr_size;

	// Calculate the start index and end index for this thread
	for (int i = 0; i < num_of_threads; i++)
	{
		thread_work = remaining_work / (num_of_threads-i);
		remaining_work -= thread_work;

		if (i == thread_args->thread_id){
			subarray_start_index = thread_args->arr_size - remaining_work - thread_work;
			subarray_end_index = thread_args->arr_size - remaining_work - 1;
			break;
		}
	}
	// Get the subarray for this thread
	int subarray_size = subarray_end_index - subarray_start_index + 1;
	int * subarray = (int *)malloc(subarray_size * sizeof(int)+1);

	// copy the subarray into the subarray array
	memcpy(subarray, &thread_args->intarr[subarray_start_index], subarray_size * sizeof(int));

	// Sort the subarray using qsort
	// No mutex lock is needed because each thread is sorting a different part of the intarray
	qsort
	(
		subarray,
		subarray_size,
		sizeof(unsigned int),
		compare
	);

	// Choose the samples from the subarray
	// Where the number of samples == number of threads
	// and the index of the sample is based on the number of threads:
	// 0, (n/p^2), (2n/p^2), (3n/p^2), ..., ((p-1)n/p^2)
	int sample_index;
	for (int i = 0; i <  num_of_threads; i++)
	{
		// Add the samples to the samples array
		sample_index = ((i * thread_args->arr_size) /(num_of_threads*num_of_threads));
		memcpy(&thread_args->samples[(thread_args->thread_id * num_of_threads) + i], &subarray[sample_index], sizeof(int));
	}
	// Post to the semaphore to indicate that this thread is done with part 1
	sem_post(child_to_main);

	// wait on the section2 semaphore to get notified when the main thread is done with part 2
	sem_wait(main_to_child);

	// Get the pivots from the struct
	int * pivots = thread_args->pivots;

	int ** partitions = (int **)malloc((num_of_threads) * sizeof(int*));
	if (partitions == NULL) {perror("malloc"); exit(1); }

	//initialize the partitions
	for (int i = 0; i < num_of_threads; i++)
	{
		partitions[i] = (int*)malloc(subarray_size * sizeof(int));
		if (partitions[i] == NULL) {perror("malloc"); exit(0); }
	}

	// index we will use to iterate over the array of pivots
	int pivot_index = 0;

	// Index used to figure out which partition a subarray goes into
	int partition_index = 0;
	int partition_elem_index = 0;

	// Initialize partition array
	// printf("Thread %d: Allocating memory for partitions\n", thread_args->thread_id);
	int * partition_sizes = (int *)malloc(num_of_threads * sizeof(int));
	if (partition_sizes == NULL) {perror("malloc"); exit(1); }
	// Allocate memory for the partition sizes

	for (int i = 0; i < num_of_threads; i++)
	{
		partition_sizes[i] = 0;
	}

	// With the variables ready, we can divide the subarray into partitions based on the pivots
	// To do this, we iterate over the subarray
	for (int i = 0; i< subarray_size; i++){

		// fflush(stdout);

		if (subarray[i] <= pivots[pivot_index]){
			partitions[partition_index][partition_elem_index] = subarray[i];
			partition_sizes[partition_index] = partition_elem_index+1;
			// printf("Thread %d: subarray[%d] %d <= pivot[%d] %d. partition [%d][%d]. Partition size is now %d\n", thread_args->thread_id, i, partitions[partition_index][partition_elem_index], pivot_index, pivots[pivot_index], partition_index, partition_elem_index, partition_sizes[partition_index]);
			partition_elem_index++;

		// Otherwise if this is not the last pivot index
		} else if(pivot_index < num_of_threads-1){
			// printf("Thread %d: subarray[%d] %d > pivot[%d] %d. Adding to partition [%d][%d] Increasing pivot to [%d]: %d\n", thread_args->thread_id, i, subarray[i], pivot_index, pivots[pivot_index], partition_index+1, 0, pivot_index+1, pivots[pivot_index+1]);
			// Move to the next pivot
			pivot_index++;
			// Move to the next partition
			partition_index++;
			// Reset the partition element index
			partition_elem_index = 0;
			// Add the element to the partition
			partitions[partition_index][partition_elem_index] = subarray[i];
			partition_elem_index+=1;
			partition_sizes[partition_index] = partition_elem_index;

		} else {
			// printf("Thread %d: subarray[%d] %d. At last partition: [%d][%d]. Partition size is now %d / %d\n", thread_args->thread_id, i, subarray[i], partition_index, partition_elem_index,partition_elem_index+1, subarray_size);
			// Otherwise this is the last pivot and all elements go in the last partition
			partitions[partition_index][partition_elem_index] = subarray[i];
			partition_elem_index+=1;
			partition_sizes[partition_index] = partition_elem_index;
		}
	}
	// printf("Thread %d: Finished partitioning\n", thread_args->thread_id);

	// Add the partitions to the right place in the struct
	for (int i = 0; i < num_of_threads; i++)
	{
		// printf("Thread %d: partition %d size: %d\n", thread_args->thread_id, i, partition_sizes[i]);
		// fflush(stdout);
		if(partition_sizes[i] == 0){
			// printf("Thread %d: partition %d is empty\n", thread_args->thread_id, i);
			continue;
		}
		// Aquire the right subarray lock
		sem_wait(subarray_lock);

		// If the subarray is empty, we just copy the partition into the subarray
		if (thread_args->subarray_sizes[i] == 0)
		{
			// printf("Thread %d: partition %d is empty. Copying partition into subarray\n", thread_args->thread_id, i);
			fflush(stdout);

			// allocate memory for the subarray
			thread_args->subarrays[i] = (int *)malloc((partition_sizes[i]+1) * sizeof(int));
			// Copy the partition into the subarray
			memcpy(thread_args->subarrays[i], partitions[i], partition_sizes[i] * sizeof(int));
			// thread_args->subarrays[i] = partitions[i];
			thread_args->subarray_sizes[i] = partition_sizes[i];
			// printf("Thread %d creating subarray %d. new size: %d\n", thread_args->thread_id, i,thread_args->subarray_sizes[i]);
		}
		else
		{
			// Otherwise if both the subarray and the partition are not empty, we need to concatenate the partition to the subarray
			// First, we need to allocate enough memory for the new subarray


			int new_subarray_size = thread_args->subarray_sizes[i] + partition_sizes[i];

			// reallocate memory for the new subarray
			// printf("Thread %d: partition %d is not empty. Reallocating memory for new subarray. Old: %d. new: %d\n", thread_args->thread_id, i, thread_args->subarray_sizes[i], new_subarray_size);
			// fflush(stdout);
			thread_args->subarrays[i] = realloc(thread_args->subarrays[i], (new_subarray_size) * sizeof(int));

			//free the unused memory
			// printf("Thread %d: partition %d is not empty. Freeing memory for partition\n", thread_args->thread_id, i);
			// fflush(stdout);

			// printf("Thread %d: partition %d is not empty. Copying partition into subarray\n", thread_args->thread_id, i);
			// fflush(stdout);

			// Copy the partition into the subarray
			memcpy(&thread_args->subarrays[i][thread_args->subarray_sizes[i]], partitions[i], partition_sizes[i] * sizeof(int));

			thread_args->subarray_sizes[i] = new_subarray_size;
			// printf("Thread %d concatenating to subarray %d new size[%d]: %d\n", thread_args->thread_id, i, i, thread_args->subarray_sizes[i]);
		}

		// then we post to the semaphore
		// printf("Thread %d: Releasing lock for subarray %d\n", thread_args->thread_id, i);
		sem_post(subarray_lock);
		// print the subarray size
		// printf("Thread %d: Subarray size: %d\n", thread_args->thread_id, thread_args->subarray_sizes[i]);

	}

	// printf("Thread %d: Done moving subarrays\n", thread_args->thread_id);
	sem_post(subarr_moving_phase);
	// printf("Thread %d: Waiting for other threads to finish moving subarrays\n", thread_args->thread_id);
	sem_wait(sorting_phase);


	// Finally, we sort the final subarrays
	// printf("Thread %d: Sorting subarrays\n", thread_args->thread_id);
	int * array_to_sort = thread_args->subarrays[thread_args->thread_id];
	qsort(array_to_sort, thread_args->subarray_sizes[thread_args->thread_id], sizeof(int), compare);

	// And calculate where the subarray should be inserted into the final array
	int final_array_index = 0;
	if (thread_args->thread_id == 0){
		final_array_index = 0;
	}
	else{
		for (int i = 0; i < thread_args->thread_id; i++){
			final_array_index += thread_args->subarray_sizes[i];
		}
	}

	// printf("Thread %d: Inserting subarray into final array at index %d\n", thread_args->thread_id, final_array_index);
	// then we copy the sorted subarray into the final array
	memcpy(&thread_args->final_array[final_array_index], array_to_sort, thread_args->subarray_sizes[thread_args->thread_id] * sizeof(int));

	free(partitions);
	free(array_to_sort);
	free(subarray);

	return NULL;
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
			printf("Error at %ld: %d > %d\n", i, list[i], list[i+1]);
		  return 0;
		}
	}
	return 1;
}