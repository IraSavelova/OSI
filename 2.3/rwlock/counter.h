#ifndef COUNTER_H
#define COUNTER_H
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <stdbool.h>
#include "rwlock.h"
#define NUM_COUNTERS 4
#define ERROR -1
#define SUCCESS 0

typedef enum
{
    SWAP,
    INCR,
    DECR,
    EQ, 
    COUNTERS_COUNT
} counters_name;

typedef struct _Counter
{
    size_t value;
    lock_t stat_lock;
} Counter;

typedef struct _Counters
{
    Counter counter[COUNTERS_COUNT];
} Counters;

Counter *counters_get(Counters *c, counters_name name);
int counters_init();
int counters_destroy(Counters *c);
void counter_increment(Counter *counter);
void counter_decrement(Counter *counter);
size_t get_counter_value(Counter *counter);

#endif 
