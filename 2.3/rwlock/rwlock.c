#include "rwlock.h"
pthread_rwlock_t stats_lock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t list_lock = PTHREAD_RWLOCK_INITIALIZER;

int lock_init(pthread_rwlock_t *rwlock, pthread_rwlockattr_t *attr)
{
    return pthread_rwlock_init(rwlock, attr);
}
void read_lock(lock_t *n)
{
    pthread_rwlock_rdlock(n);
}

void write_lock(lock_t *n)
{
    pthread_rwlock_wrlock(n);
}

void unlock(lock_t *n)
{
    pthread_rwlock_unlock(n);
}
