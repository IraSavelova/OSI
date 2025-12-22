#include "rwlock.h"

int lock_init(pthread_rwlock_t *rwlock, pthread_rwlockattr_t *attr)
{
    return pthread_rwlock_init(rwlock, attr);
}
int lock_destroy(pthread_rwlock_t *rwlock)
{
    return pthread_rwlock_destroy(rwlock);
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
