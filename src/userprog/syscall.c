#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "filesys/filesys.h"
#include "devices/shutdown.h"
#ifdef USERPROG
#include "userprog/process.h"
#include "devices/input.h"
#include "filesys/file.h"
#endif


static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock);
}

void check_address(void *address)
{
  //valid 영역 : 0x8048000~0xc0000000 핀토스 공식 문서 참조
  if(address >= 0xc0000000 || address < 0x8048000 || address == NULL)
    syscall_exit(-1); //유저 영역이 아니라면 exit
  
  else return;
}

//유저 스택의 인자들을 arg에 저장
void
get_argument(int *esp, int *arg, int count)
{
  int i;
  for ( i = 0 ; i < count; i ++)
  {
    check_address(esp + 1 + i);
    arg[i] = *(esp + 1 + i);
  }
}

//syscall.h 헤더파일 작성할 것!!!!!!!!!!!!!!!!!!!!!!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static void
syscall_handler (struct intr_frame *f) 
{
  uint32_t *sp = f->esp;
  //스택 포인터 valid check
  check_address(sp);

  // thread_current()->esp = sp; //stack pointer 저장
  //syscall number를 사용하여 syacall 호출
  int syscall_num = *((int*)sp);
  int argv[3];

  switch(syscall_num)
  {
    case SYS_HALT:
      syscall_halt();
      break;
    case SYS_EXIT:
      get_argument(sp, argv, 1);
      syscall_exit(argv[0]);
      break;
    case SYS_EXEC:
      get_argument(sp, argv, 1);
      f->eax = syscall_exec((const char *)argv[0]);
      break;
    case SYS_WAIT:
      get_argument(sp, argv, 1);
      f->eax = syscall_wait(argv[0]);
      break;
    case SYS_CREATE:
      get_argument(sp, argv, 2);
      f-> eax = syscall_create((const char*)argv[0], (unsigned )argv[1]);
      break;
    case SYS_REMOVE:
      get_argument(sp, argv, 1);
      f->eax = syscall_remove((const char*)argv[0]);
      break;
    case SYS_OPEN:
      get_argument (f->esp, argv, 1);
      f->eax = syscall_open ((const char*)argv[0]);
      break;
    case SYS_FILESIZE:
      get_argument (f->esp, argv, 1);
      f->eax = syscall_filesize(argv[0]);
      break;
    case SYS_READ:
      get_argument (f->esp, argv, 3);
      f->eax = syscall_read(argv[0], argv[1], argv[2]);
      break;
    case SYS_WRITE:
      get_argument (f->esp, argv, 3);
      f->eax = syscall_write(argv[0], argv[1], argv[2]);
      break;
    case SYS_SEEK:
      get_argument (f->esp, argv, 2);
      syscall_seek(argv[0], argv[1]);
      break;
    case SYS_TELL:
      get_argument (f->esp, argv, 1);
      f->eax = syscall_tell(argv[0]);
      break;
    case SYS_CLOSE:
      get_argument(f -> esp, argv, 1);
      syscall_close(argv[0]);
      break;
  }

}

void syscall_halt()
{
  shutdown_power_off(); //devices/shutdown.h include
}

pid_t syscall_exec(const char *cmd_line)
{
  //자식 프로세스 생성 및 실행하는 syscall
  pid_t pid;
  struct thread* child_process;
  pid = process_execute(cmd_line); //자식 프로세스 생성
  if(pid == -1) return -1;
  //pid로 자식 검색
  child_process = get_child_process(pid);

  sema_down(& child_process->sema_load); //자식 로드될때까지 대기

  if ( !(child_process->is_load)) return -1;
    
  return pid;
}

bool syscall_create(const char *file, unsigned initial_size)
{
  //file 의 이름, initial size의 크기를 가지는 파일 생성 함수(생성만 하고 openg하지는 X)
  //filesys/filesys.h include
  // bool filesys_create (const char *name, off_t initial_size) 함수 사용
  if (file == NULL) syscall_exit(-1) ; 
  check_address(file);
  //널포인터 및 user 영역 확인

  return filesys_create(file, initial_size);
  //성공 여부를 반환
}

bool syscall_remove (const char *file)
{
  //file(파일명)에 해당하는 파일 제거
  if (file == NULL) syscall_exit(-1) ;
  check_address(file);

  return filesys_remove(file);
  //성공 여부를 반환
}

void
syscall_exit (int status)
{
  struct thread *cur = thread_current ();
  cur->exit_status = status;
  printf("%s: exit(%d)\n", cur->name, status); /* Print Termination Messages */
  thread_exit();
}

int
syscall_wait (pid_t tid)
{
  return process_wait(tid); // 찬호 코드에서는 이거 지워져 있음
}




// syscall_open
int
syscall_open(const char *file) 
{
  struct thread *cur_t = thread_current();
  struct file *fp;

  // address 를 check 하는 함수 따로 만들어줘야할지 생각해봐야 함
  // 현재 check_address는 주소가 유저영역이 아니라면 exit하는 형태
  // 주소가 유저영역이 아니어도 되나?
  check_address(file);

  lock_acquire(&filesys_lock);
  if(file==NULL) {
    lock_release(&filesys_lock);
    syscall_exit(-1);
  }

  fp = filesys_open(file);
  if(fp==NULL) {
    lock_release(&filesys_lock);
    return -1;
  } else {
    if(strcmp(cur_t->name, file)==0) {
      file_deny_write(fp);
    }
    cur_t->fd_table[cur_t->fd_counter] = fp;
    lock_release(&filesys_lock);
    return cur_t->fd_counter++;
  }
}

int
syscall_filesize(int fd)
{
  struct thread *cur_t = thread_current();
  if (cur_t->fd_table[fd]==NULL)
    return -1;
  else
    return file_length(cur_t->fd_table[fd]);
}



// syscall_read()는 file_read() in file.h 이용 - 파일 접근 전에 lock 획득 필요
// syscall_write()은 file_write() in file.h 이용 - 파일 접근 전에 lock 획득 필요

// input_getc(void) in devices/input.h 키보드로 입력 받은 문자를 반환

int
syscall_read (int fd, void *buffer, unsigned size)
{
  int read_result;
  struct thread *cur_t = thread_current();
  check_address(buffer);
  if(fd<0 || fd>cur_t->fd_counter) {
    syscall_exit(-1);
  }
  else if (fd == 0) { // 이거 맞나? 여차 하면 걍 날리자
    lock_acquire(&filesys_lock);
    read_result = input_getc();
    lock_release(&filesys_lock);
  } else {
    struct file *fp = cur_t->fd_table[fd];
    if(fp==NULL)
      syscall_exit(-1);
    
    lock_acquire(&filesys_lock);
    read_result = file_read(fp, buffer, size);
    lock_release(&filesys_lock);
  }

  return read_result;
}

// putbuf (const char *, size_t) in stdio.h 문자열을 화면에 출력
int
syscall_write (int fd, void *buffer, unsigned size)
{
  int write_result;
  struct thread *cur_t = thread_current();
  if(fd<1 || fd>=cur_t->fd_counter) {
    syscall_exit(-1);
  } else if (fd == 1) {
    lock_acquire(&filesys_lock);
    putbuf(buffer, size);
    lock_release(&filesys_lock);
    return size;
  } else {
    struct file* fp = cur_t->fd_table[fd];
    if(fp == NULL)
      syscall_exit(-1);
    lock_acquire(&filesys_lock);
    write_result = file_write(fp, buffer, size);
    lock_release(&filesys_lock);
  }

  return write_result;
}

// 열려있는 파일의 offset을 이동. 현재 offset을 기준으로 position만큼 이동
void
syscall_seek(int fd, unsigned position)
{
  struct file *fp;
  fp = thread_current()->fd_table[fd];
  if(fp!=NULL)
    file_seek(fp, position);
}

unsigned
syscall_tell (int fd)
{
  struct file *fp;
  fp = thread_current()->fd_table[fd];
  if(fp==NULL)
    return -1;
  else
    return file_tell(fp);
}

void
syscall_close(int fd)
{
  struct file *fp;
  struct thread *cur_t = thread_current();
  int fd_walker;

  if(fd<2 || fd>=cur_t->fd_counter) {
    syscall_exit(-1);
  } else {
    fp = cur_t->fd_table[fd];
    if(fp!=NULL) {
      file_close(fp);
      cur_t->fd_table[fd] = NULL;
      fd_walker = fd;
      while(fd_walker<cur_t->fd_counter) {
        cur_t->fd_table[fd_walker] = cur_t->fd_table[fd_walker+1];
        fd_walker++;
      }
      cur_t->fd_counter--;
    }
  }
}