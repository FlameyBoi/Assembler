#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

void* myMalloc(size_t len) /*malloc + failure check*/
{
	void* ptr;
	ptr = malloc(len);
	if (errno == ENOMEM)
	{
		printf("Fatal error: bad allocation, terminating\n");
		exit(ENOMEM);
	}
	return ptr;
}

void* myRealloc(void* ptr, size_t len) /*realloc + failure check*/
{
	void* newptr;
	newptr = realloc(ptr, len);
	if (errno == ENOMEM)
	{
		printf("Fatal error: bad allocation, terminating\n");
		exit(ENOMEM);
	}
	return newptr;
}