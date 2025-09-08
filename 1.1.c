#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#define CREATE_SUCCES 0

int global_int = 20;

void *mythread(void *arg)
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
	printf("До изменения - local: %d, global: %d\n", local, global_int);
	local += 10;
	global_int += 20;
	printf("После изменения - local: %d, global: %d\n", local, global_int);
	sleep(10);
	return (void *)pthread_self();
}

int main()
{
	pthread_t thread_id[5];
	void *thread_res[5];
	int err;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());
	for (int i = 0; i < 5; i++)
	{
		printf("thread %d \n", i);
		err = pthread_create(&thread_id[i], NULL, mythread, NULL);
		if (err != CREATE_SUCCES)
		{
			printf("main: pthread_create() failed: %s\n", strerror(err));
			return EXIT_FAILURE;
		}
	}
	for (int i = 0; i < 5; i++)
	{
		pthread_join(thread_id[i], &thread_res[i]);
		pthread_t returned_id = (pthread_t)thread_res[i];
		int res = pthread_equal(thread_id[i], returned_id);
		printf("Поток %d завершился, res: %d\n", i, res);
	}
	return EXIT_SUCCESS;
}
