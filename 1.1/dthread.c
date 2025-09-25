#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#define CREATE_SUCCES 0
#define THREAD_COUNT 5
#define JOIN_SUCCES 0
int global_int = 20;

void *mythread()
{
	int local = 0;
	printf("До изменения - local: %d, global: %d\n", local, global_int);
	local += 10;
	global_int += 20;
	printf("После изменения - local: %d, global: %d\n", local, global_int);
	return NULL;
}

int main()
{
	pthread_t thread_id[THREAD_COUNT];
	int err;
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		printf("thread %d \n", i);
		err = pthread_create(&thread_id[i], NULL, mythread, NULL);
		if (err != CREATE_SUCCES)
		{
			printf("main: pthread_create() failed: %s\n", strerror(err));
			return EXIT_FAILURE;
		}
        sleep(1);
	}
    pthread_exit(NULL);
	return EXIT_SUCCESS;
}
