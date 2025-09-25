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
#define NO_EQUAL 0
int global_int = 20;

void *mythread()
{
	printf("\n Внутри потока\n");
	printf("PID: %d\n", getpid());
	printf("PPID: %d\n", getppid());
	printf("TID: %d\n", gettid());
	printf("pthread_self(): %lu\n", (unsigned long)pthread_self());

	int local = 0;
	const int local_const = 10;
	static int local_static = 111;
	void *address_i = &local;
	const void *address_c = &local_const;
	void *address_s = &local_static;
	void *address_f = &global_int;
	printf("address local int : %p from thread: %d\n", address_i, gettid());
	printf("address local static int: %p from thread: %d\n", address_c, gettid());
	printf("address local const int: %p from thread: %d\n", address_s, gettid());
	printf("address global int: %p from thread: %d\n", address_f, gettid());
	return (void *)pthread_self();
}

int main()
{
	pthread_t thread_id[THREAD_COUNT];
	void *thread_res[THREAD_COUNT];
	int err;
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		printf("thread %d ", i);
		err = pthread_create(&thread_id[i], NULL, mythread, NULL);
		if (err != CREATE_SUCCES)
		{
			printf("main: pthread_create() failed: %s\n", strerror(err));
			return EXIT_FAILURE;
		}
        sleep(2);
	}
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		int ret = pthread_join(thread_id[i], &thread_res[i]);
		if (ret != JOIN_SUCCES)
		{
			fprintf(stderr, "ERROR: pthread_join");
			return EXIT_FAILURE;
		}
		pthread_t returned_id = (pthread_t)thread_res[i];
		int res = pthread_equal(thread_id[i], returned_id);
        if (res == NO_EQUAL){
            printf("thread_id from pthread_create and from pthread_self no equal!!!\n");
        }
		printf("Поток %d завершился, thread_id from pthread_create:%ld, from pthread_self:%ld\n", i, thread_id[i], returned_id);
	}
	return EXIT_SUCCESS;
}
