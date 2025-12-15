#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#include "node.h"

#define MAX_COUNT 10000
#define SUCCESS 0
#define SLEEP_TIME 10

size_t global_incr = 0;
size_t global_decr = 0;
size_t global_eq = 0;
size_t global_swaps = 0;

void set_cpu(int n)
{
    int err;
    cpu_set_t cpuset;
    pthread_t tid = pthread_self();

    CPU_ZERO(&cpuset);
    CPU_SET(n, &cpuset);

    err = pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset);
    if (err != SUCCESS)
    {
        printf("set_cpu: pthread_setaffinity failed for cpu %d\n", n);
        return;
    }
    printf("set_cpu: set cpu %d\n", n);
}

void *stats_printer()
{
    set_cpu(0);
    while (1)
    {
        sleep(2);
        read_lock(&stats_lock);
        size_t inc = global_incr;
        size_t dec = global_decr;
        size_t eq = global_eq;
        size_t swaps = global_swaps;
        unlock(&stats_lock);

        printf("STATS: increase=%ld  decrease=%ld  equals=%ld  swaps=%ld\n",
               inc, dec, eq, swaps);
        fflush(stdout);
        pthread_testcancel();
    }
    return NULL;
}

void *decrease(void *arg)
{
    set_cpu(3);
    int count = 0;
    Storage *storage = (Storage *)arg;
    while (true)
    {
        count += list_iteration_pairs(storage, comparison_node, CMP_GREATER);
        pthread_testcancel();
        up_count(&global_decr);
    }
    return (void *)(intptr_t)count;
}

void *equals(void *arg)
{
    set_cpu(2);
    int count = 0;
    Storage *storage = (Storage *)arg;
    while (true)
    {
        count += list_iteration_pairs(storage, comparison_node, CMP_EQUAL);
        pthread_testcancel();
        up_count(&global_eq);
    }
    return (void *)(intptr_t)count;
}

void *increase(void *arg)
{
    set_cpu(1);
    int count = 0;
    Storage *storage = (Storage *)arg;
    while (true)
    {
        count += list_iteration_pairs(storage, comparison_node, CMP_LESS);
        pthread_testcancel();
        up_count(&global_incr);
    }
    return (void *)(intptr_t)count;
}

// Генератор случайных строк
void random_string(char *buf, int max_len)
{
    int len = (rand() % (max_len - 2)) + 1;
    for (int i = 0; i < len; i++)
    {
        buf[i] = 'a' + (rand() % 26);
    }
    buf[len] = '\0';
}

void *random_swapper(void *arg)
{
    Storage *storage = (Storage *)arg;
    while (true)
    {
        try_random_swapper(storage, &global_swaps);
        pthread_testcancel();
    }
    return NULL;
}

int main()
{
    pthread_t tid_increase, tid_equals, tid_decrease;
    pthread_t tid_swapper[3];
    pthread_t tid_stats;
    Storage st;

    int err;

    srand(time(NULL)); // Инициализация генератора случайных чисел

    printf("Initializing storage with %d nodes...\n", MAX_COUNT);
    err = storage_init(&st);
    if (err != SUCCESS)
    {
        printf("main: storage_init failed: \n");
        return EXIT_FAILURE;
    }
    for (int i = 0; i < MAX_COUNT; i++)
    {
        char str[100];
        random_string(str, sizeof(str));
        err = storage_push_back(&st, str);
        if (err == ERROR)
        {
            printf("main: storage_push_back failed: \n");
            return EXIT_FAILURE;
        }
    }
    printf("Storage initialized with %d nodes\n", MAX_COUNT);

    err = pthread_create(&tid_stats, NULL, stats_printer, NULL);
    if (err != SUCCESS)
    {
        printf("main: pthread_create(stats) failed: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_create(&tid_increase, NULL, increase, &st);
    if (err != SUCCESS)
    {
        printf("main: pthread_create(increase) failed: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_create(&tid_decrease, NULL, decrease, &st);
    if (err != SUCCESS)
    {
        printf("main: pthread_create(decrease) failed: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_create(&tid_equals, NULL, equals, &st);
    if (err != SUCCESS)
    {
        printf("main: pthread_create(equals) failed: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    for (int i = 0; i < 3; i++)
    {
        err = pthread_create(&tid_swapper[i], NULL, random_swapper, &st);
        if (err != SUCCESS)
        {
            printf("main: pthread_create(swapper%d) failed: %s\n", i, strerror(err));
            return EXIT_FAILURE;
        }
    }

    printf("All threads created. Running for %d seconds...\n", SLEEP_TIME);
    sleep(SLEEP_TIME);
    printf("Cancelling threads...\n");

    pthread_t threads[] = {tid_decrease, tid_equals, tid_increase, tid_stats, tid_swapper[0], tid_swapper[1], tid_swapper[2]};
    const char *thread_names[] = {"decrease", "equals", "increase", "stats", "swapper0", "swapper1", "swapper2"};

    for (int i = 0; i < 7; i++)
    {
        int cancel_err = pthread_cancel(threads[i]);
        if (cancel_err != SUCCESS)
        {
            printf("main: pthread_cancel(%s) failed: %s\n", thread_names[i], strerror(cancel_err));
        }
        int join_err = pthread_join(threads[i], NULL);
        if (join_err != SUCCESS)
        {
            printf("main: pthread_join(%s) failed: %s\n", thread_names[i], strerror(join_err));
        }
        else
        {
            printf("Thread %s terminated successfully\n", thread_names[i]);
        }
    }

    printf("\nFINAL STATS:\n");
    printf("Increase iterations: %ld\n", global_incr);
    printf("Decrease iterations: %ld\n", global_decr);
    printf("Equals iterations: %ld\n", global_eq);
    printf("Total swaps: %ld\n", global_swaps);

    storage_destroy(&st);

    printf("Program completed successfully\n");
    return EXIT_SUCCESS;
}
