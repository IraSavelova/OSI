#include <stdio.h>
#include "unistd.h"
#include "sys/types.h"
#include <stdlib.h>
#include <wait.h>

#define CHILD 0
#define ERROR -1
#define SUCCESS 0
#define FAILURE 1
#define ERROR_WIFEXITED 0

int global_var = 10;

int funk()
{
    int local_var = 20;
    void *address_local = &local_var;
    void *address_global = &global_var;
    printf("In parent proc: Value local_var: %d, address local_var: %p\n", local_var, address_local);
    printf("In parent proc: Value global_var: %d, address global_var: %p\n", global_var, address_global);
    pid_t pid_funk = getpid();
    printf("Parent PID: %d \n", pid_funk);
    pid_t process = fork();
    if (process == ERROR)
    {
        fprintf(stderr, "Ошибка: не удалось создать дочерний процесс\n");
        perror("Error fork:");
        return ERROR;
    }
    if (process == CHILD)
    {
        pid_t child_pid = getpid();
        pid_t parent_pid = getppid();
        printf("Child PID: %d\n", child_pid);
        printf("In child proc: parent pid: %d\n", parent_pid);
        printf("In child proc: Value local_var: %d, address local_var: %p\n", local_var, address_local);
        printf("In child proc: Value global_var: %d, address global_var: %p\n", global_var, address_global);
        local_var = 200;
        global_var = 100;
        printf("In child proc: New value local_var: %d, address local_var: %p\n", local_var, address_local);
        printf("In child proc: New value global_var: %d, address global_var: %p\n", global_var, address_global);
        _exit(5);
    }

    sleep(30);
    printf("In parent proc: Value local_var: %d, address local_var: %p\n", local_var, address_local);
    printf("In parent proc: Value global_var: %d, address global_var: %p\n", global_var, address_global);
    int status;
    pid_t child = wait(&status);
    if (child == ERROR)
    {
        perror("wait error");
        return FAILURE;
    }
    int end_child = WIFEXITED(status);
    if (end_child == ERROR_WIFEXITED)
    {
        int bad_exit = WIFSIGNALED(status);
        printf("Child process %d ended without ok . killed with signal %i\n", child, bad_exit);
        return FAILURE;
    }
    int exit_child = WEXITSTATUS(status);
    printf("Child process %d ended with %d status\n", child, exit_child);

    return SUCCESS;
}

int main()
{
    int result_funk = funk();
    if (result_funk == FAILURE)
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
} 
