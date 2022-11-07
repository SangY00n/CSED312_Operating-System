#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>

typedef int pid_t;

void check_address(void *address);
void get_argument(int *esp, int *arg, int count);
void syscall_init (void);
void syscall_exit (int status);
int syscall_wait (pid_t tid);

void syscall_halt();
pid_t syscall_exec(const char *cmd_line);
bool syscall_create(const char *file, unsigned initial_size);
bool syscall_remove (const char *file);
int syscall_open (const char *file);
int syscall_filesize (int fd);



#endif /* userprog/syscall.h */
