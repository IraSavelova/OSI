#include "mutex.h"

lock_t stats_lock = PTHREAD_MUTEX_INITIALIZER;
lock_t list_lock = PTHREAD_MUTEX_INITIALIZER;

int lock_init(lock_t *mutex, pthread_mutexattr_t* attr)
{
    return pthread_mutex_init(mutex, attr);
}

int lock_destroy(lock_t *mutex)
{
    return pthread_mutex_destroy(mutex);
}

void read_lock(lock_t *n)
{
    pthread_mutex_lock(n);
}

void write_lock(lock_t *n)
{
    pthread_mutex_lock(n);
}

void unlock(lock_t *n)
{
    pthread_mutex_unlock(n);
}
