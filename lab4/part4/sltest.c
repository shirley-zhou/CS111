#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include "SortedList.h"

// used to calculate time of operations
unsigned long totaltime;
// creating clock structures to record time in both functions and main()
struct timespec t1, t2;


int iterations = 1,
	opt_yield = 0,
	correct = 0;
SortedList_t **list;

// thread/lock mutexes/locks
pthread_mutex_t test_mutex;
int test_lock = 0;
char sync = 0;

// designed to pass arguments into threadfunction without making them global or confusing
struct arg_struct
{
	SortedListElement_t **elements;
	int num_lists, start;
};

unsigned long timediff(struct timespec t1, struct timespec t2)
{
	unsigned long sec, result;
	long nsec;

	nsec = t2.tv_nsec - t1.tv_nsec;
	sec = t2.tv_sec - t1.tv_sec;

	//printf("\nerror: sec %ld usec %ld\n", sec, usec);
	
	if(nsec < 0)
	{
		sec--;
		nsec += 1000000;
	}
	
	result = (sec*1000000 + (unsigned long)nsec);
	return result;
}

// only really have to lock once
void threadfunction(void *arguments)
{	
	struct arg_struct *args = arguments;
	int i, end, start = (args->start), num_lists = (args->num_lists);
	SortedListElement_t **elements = (args->elements);
	
	if(sync == 's')
	{
		while(__sync_lock_test_and_set(&test_lock, 1));
	}
	else if(sync == 'm')
	{
		// fprintf(stderr, "1\n"); //DEBUGGING
		pthread_mutex_lock(&test_mutex);
		// fprintf(stderr, "acquired lock\n"); //DEBUGGING
	}

	// getting time of just threaded function for accuracy
	if(correct == 1)
		{ clock_gettime(CLOCK_MONOTONIC, &t1); }
	
	// iterates through pertinent element variables and adds to a sorted_list
	for(i = start, end = start + iterations; i < end; i++)
	{
		// inserts element i (already created)
		// into '(key % num_lists)-th' sorted list
		int list_num_to_store = (*(elements[i]->key)) % num_lists;
		
		SortedList_insert(list[list_num_to_store], elements[i]);
		// fprintf(stderr, "inserted element with key \'%d\' into list %d\n", *(elements[i]->key), list_num_to_store); //DEBUGGING
	}

	
	// int length = SortedList_length(list[]);
	for(i = start, end = start + iterations; i < end; i++)
	{
		int list_num_to_lookup = (*(elements[i]->key)) % num_lists;
		SortedListElement_t *p = SortedList_lookup(list[list_num_to_lookup], elements[i]->key);
		
		if(p != NULL)
			{ SortedList_delete(p); }
		//else
		//	fprintf(stderr, "attempting to delete invalid element\n"); //DEBUGGING
	}

	// getting time of just threaded function for accuracy
	if(correct == 1)
	{
		clock_gettime(CLOCK_MONOTONIC, &t2);
		totaltime += timediff(t1, t2);
	}
	
	if(sync == 's')
	{
		__sync_lock_release(&test_lock); //writes 0 back to test_lock
	}
	else if(sync == 'm')
	{
		pthread_mutex_unlock(&test_mutex);
		// fprintf(stderr, "released lock\n"); //DEBUGGING
	}
}

int main(int argc, char* argv[])
{
	int threads = 1,
		num_lists = 1,
		ret;

	static struct option longopts[] = 
	{
		{"correct",     no_argument,		NULL,	'c'},
		{"threads",		optional_argument,	NULL,	't'},
		{"iterations",	optional_argument,	NULL,	'i'},
		{"lists",       required_argument,	NULL,	'l'},
		{"yield", 		required_argument,	NULL,	'y'},
		{"sync",		required_argument,	NULL,	's'},
		{0, 0, 0, 0}
	};

	while((ret = getopt_long(argc, argv, "-y:t::i::l:s:c01", longopts, NULL)) != -1)
	{
		switch(ret)
		{
			case 0: //do nothing, set flag
			{
				break;
			}
			case 'c': //CORRECT
			{
				correct = 1;
				break;
			}
			case 't': //THREADS
			{
				if(optarg)
				{
					threads = atoi(optarg);
				}
				break;
			}
			case 'i': //ITERATIONS
			{
				if(optarg)
				{
					iterations = atoi(optarg);
				}
				break;
			}
			case 'l': //LISTS
			{
				if(optarg)
				{
					num_lists = atoi(optarg);
				}
				break;
			}
			case 'y': //YIELD
			{
				if (optarg)
				{
					int i;
					for(i = 0; optarg[i] != 0; i++)
					{
						switch(optarg[i])
						{
							case 'i':
							{
								opt_yield = INSERT_YIELD;
								break;
							}
							case 'd':
							{
								opt_yield = DELETE_YIELD;
								break;
							}
							case 's':
							{
								opt_yield = SEARCH_YIELD;
								break;
							}
							default:
							{
								fprintf(stderr, "invalid arg to 'yield'\n");
							}
						}
					}	
				}
				else
				{
					fprintf(stderr, "yield takes an argument");
					exit(1);
				}
				break;
			}
			case 's': //SYNC
			{
				if (optarg)
				{
					sync = *optarg;
				}
				else
				{
					fprintf(stderr, "invalid arg to sync");
					exit(1);
				}
				break;
			}
			default:
			{
				fprintf(stderr, "invalid option");
			}
		}
	}

	int	num_elements = threads*iterations,
		i,
		temp;
	
	//create array of lists
	list = malloc(num_lists * sizeof(SortedList_t*));
	//initialize each list within the list_array
	for(i = 0; i < num_lists; i++)
	{
		list[i]         = malloc(sizeof(SortedList_t));
		(list[i]->prev) = list[i];
		(list[i]->next) = list[i];
		(list[i]->key)  = NULL;
	}
		
	// allocating memory and assigning random keys
	SortedListElement_t **elements = malloc(num_elements * sizeof(SortedListElement_t*));
	for (i = 0; i < num_elements; i++)
	{
		elements[i] = malloc(sizeof(SortedListElement_t));
		
		//necessary because key is const char*, can't assign to '*key', can only change ptr 'key'
		char* tmp = malloc(sizeof(char));
		*tmp =	rand() % 128;	
		elements[i]->key = tmp;
	}

	pthread_t *tids = (pthread_t*)malloc(threads * sizeof(pthread_t));
	pthread_mutex_init(&test_mutex, NULL);
	
	if(correct == 0)
	{
		// Starting clock
		clock_gettime(CLOCK_MONOTONIC, &t1);
	}

	// thread creation and having elements, num_lists, and num_elements_in_list inside arg_struct
	// arg_struct is defined above 
	for (i = 0; i < threads; i++)
	{
		// start specifies which chunk of elements belongs to this thread
		// it is a chunk of elements[], and there are 'iterations' of them
		// (i.e. from [i * iterations] to [i+1 * iterations])
		struct arg_struct args;
		args.start		= i * iterations;
		args.num_lists	= num_lists;
		args.elements	= elements;
		
		int r = pthread_create(&tids[i], NULL, (void*) &threadfunction, (void*)&args);
		
		// != 0 implies thread was not created, end function
		if(r != 0)
		{
				fprintf(stderr, "Error creating thread. Error code is %d\n", ret);
				exit(-1);
		}
	}
	
	//joining
	for (i = 0; i < threads; i++)
	{
		int r = pthread_join(tids[i], NULL);
		if(r)
		{
			fprintf(stderr, "Error pthread_join. Error code is %d\n", ret);
			exit(-1);
		}
	}
	
	if(correct == 0)
		{ clock_gettime(CLOCK_MONOTONIC, &t2); }
	
	// SortedList_print(list[1]); //DEBUGGING

	// length of all lists (even if there is 0 or 1 list)
	// I'm not going to lie, I kinda cheated here by not 're-implementing' the length function
	int length_of_all_lists = 0;
	for(i = 0; i < num_lists; i++) { length_of_all_lists += SortedList_length(list[i]); }
	
	if (length_of_all_lists != 0)
		{ fprintf(stderr, "ERROR: list length: %d\n", length_of_all_lists); }
	
	
	// calculating official time difference of all ops + artifacts/overhead
	if(correct == 0)
		{ totaltime = timediff(t1, t2); }
	
	int len = iterations/num_lists,
		avg = len/2;
	// this makes it so if avg = 0, then avg += 1, but if avg != 0, avg += 0 (no change)
	avg += (!avg);
	int operations = threads*iterations*2*avg;

	printf("%d threads x %d iterations x (ins + lookup/del) x (%d/2 avg len)= %d operations\n", threads, iterations, len, operations);
	printf("elapsed time: %lu ns\nper operation: %lu ns\n", totaltime, totaltime/operations);

	// changed this around a bit to prevent dynamically allocated array leaks
	
	// leak still exists @ 'free(elements)'. should work fine, but it seems there are still members within elements with allocated data
	// with either SortedList_length: who is failing to report proper length
	// or leak exists with SortedList_delete: who is failing to de-allocate data so that elements as a whole will stay empty
	for(i = 0; i < num_lists; i++) { free(list[i]); }
	free(list);
	free(tids);
	// fprintf(stderr, "problem when freeing elements\n"); //DEBUGGING
	// free(elements);

	exit(0);
}
