#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include "threads/synch.h"
#include "vm/page.h"
#include <list.h>

typedef int pid_t;

struct mmap_file {

    int mapid; //mmap 성공 시 리턴된 mapping id
    struct file *file; //매핑 파일 오브젝트
    struct list_elem elem; //thread의 mmap_list 위한 구조체
    void* addr; //시작 주소 저장(munmap에서 사용)

};

// 파일에 대한 동시 접근 방지를 위해 lock 사용
struct lock filesys_lock; // thread *holder와 binary semaphore를 가짐

void syscall_init (void);
void check_address(void *addresss);
void get_argument(int *esp, int *arg, int count);

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

int mmap(int fd, void* addr);
void munmap(int mapping);


#endif /* userprog/syscall.h */
