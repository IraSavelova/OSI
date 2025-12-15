#ifndef SPIN_H
#define SPIN_H
#include <pthread.h>
#define PRIVATE 0
extern pthread_spinlock_t stats_lock;
extern pthread_spinlock_t list_lock;

typedef pthread_spinlock_t lock_t;

int lock_init(pthread_spinlock_t *spin, int pshared);

int lock_destroy(pthread_spinlock_t *spin);

void read_lock(lock_t *n);

void write_lock(lock_t *n);

void unlock(lock_t *n);
#endif
