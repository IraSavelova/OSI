#include "spin.h"

lock_t stats_lock;
lock_t list_lock;

int lock_init(pthread_spinlock_t *spin, int pshared)
{
    return pthread_spin_init(spin, pshared);
}

int lock_destroy(pthread_spinlock_t *spin)
{
    return pthread_spin_destroy(spin);
}

void read_lock(lock_t *n)
{
    pthread_spin_lock(n);
}

void write_lock(lock_t *n)
{
    pthread_spin_lock(n);
}

void unlock(lock_t *n)
{
    pthread_spin_unlock(n);
}
