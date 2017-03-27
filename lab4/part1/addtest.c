#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>

long long counter = 0;
pthread_mutex_t test_mutex;
int test_lock = 0;
int opt_yield = 0;
char sync = 0;


void add_mutex(long long *pointer, long long value) {
	pthread_mutex_lock(&test_mutex);

	long long sum = *pointer + value;
	*pointer = sum;

	pthread_mutex_unlock(&test_mutex);
}

void add_spin(long long *pointer, long long value) {
	while (__sync_lock_test_and_set(&test_lock, 1)); //assign 1 to the test_lock, function will return original value of lock (0), ends while loop.
	//if test_lock val is 1, will assign 1 but also return prev val (1) so while loop will keep going, until ret val = 0, busy wait
	
	long long sum = *pointer + value;
	*pointer = sum;
	
	__sync_lock_release(&test_lock); //writes 0 back to test_lock
}

void add_comp_swap(long long *pointer, long long value) {
	while(__sync_val_compare_and_swap(&test_lock, 0, 1) );

	long long sum = *pointer + value;
	*pointer = sum;

	__sync_val_compare_and_swap(&test_lock, 1, 0);
}

void add(long long *pointer, long long value) {
        long long sum = *pointer + value;
        if (opt_yield)
        	pthread_yield();
        *pointer = sum;
}

void addtocounter(int* iterations)
{
	int i;
	for (i = 0; i < *iterations; i++)
	{
		switch(sync)
		{
			case 'm':
				add_mutex(&counter, 1);
				break;
			case 's':
				add_spin(&counter, 1);
				break;
			case 'c':
				add_comp_swap(&counter, 1);
				break;
			default:
				add(&counter, 1);
				break;
		}
	}
	for (i = 0; i < *iterations; i++)
	{
		switch(sync)
		{
			case 'm':
				add_mutex(&counter, -1);
				break;
			case 's':
				add_spin(&counter, -1);
				break;
			case 'c':
				add_comp_swap(&counter, -1);
				break;
			default:
				add(&counter, -1);
				break;
		}
	}
}

unsigned long timediff(struct timespec t1, struct timespec t2)
{
	unsigned long sec;
	long nsec;

	nsec = t2.tv_nsec - t1.tv_nsec;
	sec = t2.tv_sec - t1.tv_sec;

	//printf("\nerror: sec %ld usec %ld\n", sec, usec);
	
	if (nsec < 0){
		sec--;
		nsec += 1000000;
	}
	unsigned long result = (sec*1000000 + (unsigned long)nsec);
	//printf("error: %f\n", result);
	return result;
}

int main(int argc, char* argv[])
{
	int threads = 1;
	int iterations = 1;
	int ret;

	struct timespec t1;
	struct timespec t2;

	static struct option longopts[] = 
		{
			{"threads", optional_argument, NULL, 'a'},
			{"iterations", optional_argument, NULL, 'b'},
			{"sync", required_argument, NULL, 'c'},
			{"yield", optional_argument, &opt_yield, 1},
			{0, 0, 0, 0}
		};

	while ((ret = getopt_long(argc, argv, "", longopts, NULL)) != -1)
	{
		switch (ret)
		{
			case 0: //do nothing, set flag
				break;
			case 'a': //THREADS
				if (optarg)
				{
					threads = atoi(optarg);
				}
				break;
			case 'b': //ITERATIONS
				if (optarg)
				{
					iterations = atoi(optarg);
				}
				break;
			case 'c': //SYNC
				if (optarg)
				{
					sync = *optarg;
				}
				else
				{
					fprintf(stderr, "sync requires an argument");
					exit(1);
				}
				break;
			default:
				fprintf(stderr, "invalid option");
		}
	}

	pthread_t* tids = (pthread_t*)malloc(threads*sizeof(pthread_t));
	pthread_mutex_init(&test_mutex, NULL);


	clock_gettime(CLOCK_MONOTONIC, &t1);

	//thread creation
	int i;
	for (i = 0; i < threads; i++)
	{
		int r = pthread_create(&tids[i], NULL, (void*) &addtocounter, (void*) &iterations);
		if (r)
        	{
            		printf("Error creating thread. Error code is %d\n", ret);
            		exit(-1);
        	}
	}

	//joining
	for (i = 0; i < threads; i++)
	{
		int r = pthread_join(tids[i], NULL);
		if (r)
        	{
            		printf("Error pthread_join. Error code is %d\n", ret);
            		exit(-1);
        	}
	}

	clock_gettime(CLOCK_MONOTONIC, &t2);

	unsigned long totaltime = timediff(t1, t2);
	int operations = threads*iterations*2;

	printf("%d threads x %d iterations x (add + subtract) = %d operations\n", threads, iterations, operations);
	if (counter != 0)
		fprintf(stderr, "ERROR: final count = %lld\n", counter);
	printf("elapsed time: %lu ns\nper operation: %lu ns\n", totaltime, totaltime/operations);


	free(tids);

	exit(0);
}
