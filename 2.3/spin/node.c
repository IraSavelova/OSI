#include "node.h"

int storage_init(Storage *st)
{
    st->first = NULL;
    lock_init(&stats_lock, PRIVATE);
    lock_init(&list_lock, PRIVATE);
    int err = lock_init(&st->stlist_lock, PRIVATE);
    if (err)
    {
        printf("storage_create: pthread_mutex_init() failed: %s\n", strerror(err));
        return ERROR;
    }
    return SUCCESS;
}


void up_count(size_t *global_var)
{
    write_lock(&stats_lock);
    (*global_var)++;
    unlock(&stats_lock);
}
Node *node_create(char *str)
{
    Node *n = malloc(sizeof(Node));
    if (n == NULL)
    {
        printf("node_create: Cannot allocate memory for a Node\n");
        return NULL;
    }
    strncpy(n->value, str, sizeof(n->value));
    n->value[sizeof(n->value) - 1] = '\0';
    n->next = NULL;
    int err = lock_init(&n->list_lock, PTHREAD_PROCESS_PRIVATE);
    if (err)
    {
        printf("node_create: pthread_mutex_init() failed: %s\n", strerror(err));
        free(n);
        return NULL;
    }
    return n;
}

void node_destroy(Node *n)
{
    int err = lock_destroy(&n->list_lock);
    if (err != SUCCESS)
        printf("node_destroy: pthread_mutex_destroy() failed: %s\n", strerror(err));
    free(n);
}

void storage_destroy(Storage *st)
{
    Node *cur = st->first;

    while (cur != NULL)
    {
        Node *next = cur->next;
        node_destroy(cur);
        cur = next;
    }
    st->first = NULL;
    int err = lock_destroy(&st->stlist_lock);
    if (err != SUCCESS)
        printf("storage_destroy: pthread_mutex_destroy() failed: %s\n", strerror(err));
}

int list_iteration_pairs(Storage *storage, int (*callback)(Node *a, Node *b), compare_mode_t mode)
{
    int ret;
    size_t count = 0;
    read_lock(&storage->stlist_lock);
    read_lock(&storage->first->list_lock);
    Node *current = storage->first;
    unlock(&storage->stlist_lock);
    Node *next = NULL;

    while (current->next != NULL)
    {
        read_lock(&current->next->list_lock);
        next = current->next;
        ret = callback(current, next);
        switch (mode)
        {
        case CMP_LESS:
            if (ret < 0)
                count++;
            break;
        case CMP_GREATER:
            if (ret > 0)
                count++;
            break;
        case CMP_EQUAL:
            if (ret == 0)
                count++;
            break;
        }
        unlock(&current->list_lock);
        current = next;
    }
    unlock(&current->list_lock);
    return count;
}

int comparison_node(Node *n1, Node *n2)
{
    return (int)strlen(n1->value) - (int)strlen(n2->value);
}

int swap_nodes(Node *prev, Node *current, Node *next)
{
    prev->next = next;
    current->next = next->next;
    next->next = current;
    return SUCCESS;
}

int try_random_swapper(Storage *storage, size_t *global_var)
{
    write_lock(&storage->stlist_lock);
    write_lock(&storage->first->list_lock);
    Node *prev = storage->first;
    unlock(&storage->stlist_lock);
    Node *current = NULL;
    Node *next = NULL;
    while (prev->next != NULL)
    {
        write_lock(&prev->next->list_lock);
        current = prev->next;
        if (current->next == NULL)
        {
            unlock(&prev->list_lock);
            prev = current;
            continue;
        }
        write_lock(&current->next->list_lock);
        next = current->next;
        if (next != NULL && rand() % 100 < 50)
        {
            swap_nodes(prev, current, next);
            up_count(global_var);
        }
        unlock(&next->list_lock);
        unlock(&prev->list_lock);
        prev = current;
    }
    unlock(&prev->list_lock);
    return SUCCESS;
}

int storage_push_back(Storage *st, char *str)
{
    Node *n = node_create(str);
    if (n == NULL)
        return ERROR;

    write_lock(&st->stlist_lock);
    if (st->first == NULL)
    {
        st->first = n;
        unlock(&st->stlist_lock);
        return SUCCESS;
    }

    Node *cur = st->first;
    write_lock(&cur->list_lock);
    while (cur->next != NULL)
    {
        Node *next = cur->next;
        write_lock(&next->list_lock);
        unlock(&cur->list_lock);
        cur = next;
    }
    cur->next = n;
    unlock(&cur->list_lock);
    unlock(&st->stlist_lock);
    return SUCCESS;
}
