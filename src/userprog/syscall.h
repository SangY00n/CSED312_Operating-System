#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include "threads/synch.h"

typedef int pid_t;

// 파일에 대한 동시 접근 방지를 위해 lock 사용
struct lock filesys_lock; // thread *holder와 binary semaphore를 가짐

void syscall_init (void);
void check_address(void *addresss);
void get_argument(void *esp, int *arg, int count);

void syscall_halt();
pid_t syscall_exec(const char *cmd_line);
bool syscall_create(const char *file, unsigned initial_size);
bool syscall_remove (const char *file);
void syscall_exit (int status);
int syscall_wait (pid_t tid);
int syscall_open (const char *file);
int syscall_filesize (int fd);
int syscall_read (int fd, void *buffer, unsigned size);
int syscall_write (int fd, void *buffer, unsigned size);
void syscall_seek(int fd, unsigned position);
unsigned syscall_tell (int fd);
void syscall_close(int fd);



#endif /* userprog/syscall.h */
