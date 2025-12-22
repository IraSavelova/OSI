#include "counter.h"

int counters_init(Counters *c)
{
    if (c == NULL)
        return ERROR;

    for (int i = 0; i < NUM_COUNTERS; i++)
    {
        c->counter[i].value = 0;
        int err = lock_init(&c->counter[i].stat_lock, NULL);
        if (err != SUCCESS)
        {
            fprintf(stderr, "counters_init: lock_init failed for counter %d: %s\n",
                    i, strerror(err));

            for (int j = 0; j < i; j++)
            {
                lock_destroy(&c->counter[j].stat_lock);
            }
            return ERROR;
        }
    }
    return SUCCESS;
}

int counters_destroy(Counters *c)
{
    if (c == NULL)
        return ERROR;

    int ret = SUCCESS;

    for (int i = 0; i < NUM_COUNTERS; i++)
    {
        int err = lock_destroy(&c->counter[i].stat_lock);
        if (err != SUCCESS)
        {
            fprintf(stderr, "counters_destroy: lock_destroy failed for counter %d: %s\n",
                    i, strerror(err));
            ret = ERROR; 
        }
    }
    return ret; 
}

Counter *counters_get(Counters *c, counters_name name)
{
    if (!c || name >= COUNTERS_COUNT)
        return NULL;
    return &c->counter[name];
}

void counter_increment(Counter *counter)
{
    if (counter == NULL)
    {
        fprintf(stderr, "counter_increment: not valid Counter *counter");
        return;
    }
    write_lock(&(counter->stat_lock));
    counter->value++;
    unlock(&(counter->stat_lock));
}
void counter_decrement(Counter *counter)
{
    if (counter == NULL)
    {
        fprintf(stderr, "counter_decrement: not valid Counter *counter");
        return;
    }
    write_lock(&(counter->stat_lock));
    counter->value--;
    unlock(&(counter->stat_lock));
}
size_t get_counter_value(Counter *counter)
{
    if (counter == NULL)
    {
        fprintf(stderr, "counter_get_value: not valid Counter *counter");
        return ERROR;
    }
    read_lock(&(counter->stat_lock));
    size_t result = counter->value;
    unlock(&(counter->stat_lock));
    return result;
}
