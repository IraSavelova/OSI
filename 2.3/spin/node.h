#ifndef __FITOS_QUEUE_H__
#define __FITOS_QUEUE_H__

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
#include "spin.h"

#define SUCCESS 0
#define ERROR -1
#define RETURN_SUCCESS 1
#define RETURN_FAIL 0
#define SLEEP_TIME 10


typedef struct _Node
{
	char value[100];
	struct _Node *next;
	lock_t list_lock;
} Node;

typedef struct _Storage
{
	Node *first;
	lock_t stlist_lock;
} Storage;

typedef enum {
    CMP_LESS,
    CMP_GREATER,
    CMP_EQUAL
} compare_mode_t;

int comparison_node(Node *n1, Node *n2);
int swap_nodes(Node *prev, Node *current, Node *next);
int list_iteration_pairs(Storage *storage, int (*callback)(Node *a, Node *b), compare_mode_t mode);
int try_random_swapper(Storage *storage, size_t* global_var);
void up_count(size_t* global_var);
int storage_init(Storage *st);
Node *node_create(char *str);
void node_destroy(Node *n);
void storage_destroy(Storage *st);
int storage_push_back(Storage *st, char *str);
#endif // __FITOS_QUEUE_H__
