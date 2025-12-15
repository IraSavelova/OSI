#ifndef RWLOCK_H
#define RWLOCK_H
#include <pthread.h>
extern pthread_rwlock_t stats_lock;
extern pthread_rwlock_t list_lock;

typedef pthread_rwlock_t lock_t;

int lock_init(pthread_rwlock_t *rwlock, pthread_rwlockattr_t *attr);

void read_lock(lock_t *n);

void write_lock(lock_t *n);

void unlock(lock_t *n);
#endif
