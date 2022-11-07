#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "filesys/filesys.h"
#include "devices/shutdown.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif


static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void check_address(void *address)
{
  //valid 영역 : 0x8048000~0xc0000000 핀토스 공식 문서 참조
  if(address => 0xc0000000 || address < 0x8048000 || address == NULL)
    exit(-1); //유저 영역이 아니라면 exit
  
  else return;
}

//유저 스택의 인자들을 arg에 저장
void
get_argument(void *esp, int *arg, int count)
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

  thread_current()->esp = sp; //stack pointer 저장
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
      f->eax = syscall_exec(argv[0]);
      break;
    case SYS_WAIT:
      get_argument(sp, argv, 1);
      f->eax = syscall_wait(argv[0]);
      break;
    case SYS_CREATE:
      get_argument(sp, argv, 2);
      f-> eax = syscall_create(argv[0], argv[1]);
      break;
    case SYS_REMOVE:
      get_argument(sp, argv, 1);
      f->eax = syscall_remove(argv[0]);
      break;
    case SYS_OPEN:
      get_argument (f->esp, argv, 1);
      f->eax = syscall_open (argv[0]);
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
      f->eax = syscall_seek(argv[0], argv[1]);
      break;
    case SYS_TELL:
      get_argument (f->esp, argv, 1);
      f->eax = syscall_tell(argv[0]);
      break;
    case SYS_CLOSE:
      get_argument(f -> esp, argv, 1);
      f->eax = syscall_close(argv[0]);
      break;
  }

}

// static void
// syscall_handler (struct intr_frame *f)
// {
//   printf("syscall!!\n");
//   return;
// }

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

int syscall_open (const char *file)
{

}

int syscall_filesize (int fd)
{

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
syscall_wait (tid_t tid)
{
  return process_wait(tid); // 찬호 코드에서는 이거 지워져 있음
}