#define _GNU_SOURCE
#include <pthread.h>
#include <stdlib.h>
#include "SortedList.h"
#include <stdio.h> //TODO: remove, debugging

//IMPLEMENTATION NOTE: circular doubly linked list with initial empty node


//DEBUGGING FUNCTION:
void SortedList_print(SortedList_t* list)
{
	SortedListElement_t* p;
	for (p = list->next; p != list; p = p->next)
		fprintf(stderr, "key: %c ", *(p->key));
	fprintf(stderr, "\n");

}


void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
	SortedListElement_t* p;
	if (opt_yield & INSERT_YIELD)
		pthread_yield();
	for (p = list->next; p != list; p = p->next)
		if ( *(element->key) < *(p->key) )	//assume 1 char implementation of key? else use strcmp() but requires nullbyte
			break;
	element->next = p;
	element->prev = p->prev;
	p->prev->next = element;
	p->prev = element;

}

int SortedList_delete(SortedListElement_t *element)
{
	if (element->next->prev != element)
		return 1;
	if (element->prev->next != element)
		return 1;
	if (opt_yield & DELETE_YIELD)
		pthread_yield();
	element->next->prev = element->prev;
	element->prev->next = element->next;
	free(element);
	return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
	SortedListElement_t* p;
	if (opt_yield & SEARCH_YIELD)
		pthread_yield();
	for (p = list->next; p != list; p = p->next)
		if (*(p->key) == *key)
			return p;
	return NULL;
}

int SortedList_length(SortedList_t *list)
{
	int count = 0;
	SortedListElement_t* p;
	for(p = list->next; p != list; p = p->next)
	{
		if(p->next->prev != p)
			{ return -1; }
		if (p->prev->next != p)
			{ return -1; }
		count++;
	}
	return count;
}