#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void syscall_exit (int status);
int syscall_wait (tid_t tid);

#endif /* userprog/syscall.h */
