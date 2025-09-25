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

void *mythread(void *arg)
{
    printf("mythread [%d %d %d %p]: Hello from mythread!\n", getpid(), getppid(), gettid(), arg);
    return NULL;
}

int main()
{
	pthread_t thread_id[THREAD_COUNT];
	int err;
	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		err = pthread_create(&thread_id[i], NULL, mythread, NULL);
		if (err != CREATE_SUCCES)
		{
			printf("main: pthread_create() failed: %s\n", strerror(err));
			return EXIT_FAILURE;
		}
	}
    pthread_exit(NULL);
	return EXIT_SUCCESS;
}
