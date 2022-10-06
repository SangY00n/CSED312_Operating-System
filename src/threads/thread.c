#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/fixed_point.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif

#define min(x,y) (x) < (y) ? (x) : (y)
#define max(x,y) (x) > (y) ? (x) : (y)

/* Random value for struct thread's `magic' member.
   Used to detect stack overflow.  See the big comment at the top
   of thread.h for details. */
#define THREAD_MAGIC 0xcd6abf4b

/* List of processes in THREAD_READY state, that is, processes
   that are ready to run but not actually running. */
static struct list ready_list;

/* List of prcesses in THREAD_BLOCKED state */
//add new struct
static struct list list_sleep_thread;

/* List of all processes.  Processes are added to this list
   when they are first scheduled and removed when they exit. */
static struct list all_list;

/* Idle thread. */
static struct thread *idle_thread;

/* Initial thread, the thread running init.c:main(). */
static struct thread *initial_thread;

/* Lock used by allocate_tid(). */
static struct lock tid_lock;

/* Stack frame for kernel_thread(). */
struct kernel_thread_frame 
  {
    void *eip;                  /* Return address. */
    thread_func *function;      /* Function to call. */
    void *aux;                  /* Auxiliary data for function. */
  };

/* Statistics. */
static long long idle_ticks;    /* # of timer ticks spent idle. */
static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
static long long user_ticks;    /* # of timer ticks in user programs. */

/* Scheduling. */
#define TIME_SLICE 4            /* # of timer ticks to give each thread. */
static unsigned thread_ticks;   /* # of timer ticks since last yield. */

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
bool thread_mlfqs;

/* load_avg value for mlfqs */
fixed_t load_avg;

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static bool is_thread (struct thread *) UNUSED;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void thread_schedule_tail (struct thread *prev);
static tid_t allocate_tid (void);
bool thread_compare_priority(const struct list_elem *e1, const struct list_elem *e2, void *aux UNUSED);
void check_list_preemption(void);
void donate_priority(struct thread* t, int depth);
void remove_with_lock(struct lock *lock);
void refresh_priority(void);
void mlfqs_priority(struct thread *t);
int calc_priority(fixed_t _recent_cpu, int _nice);
void mlfqs_recent_cpu (struct thread *t);
fixed_t calc_recent_cpu(fixed_t _load_avg, fixed_t _recent_cpu, int _nice);
void mlfqs_load_avg(void);
fixed_t calc_load_avg(fixed_t _load_avg, int _ready_threads);
void mlfqs_recent_cpu_incr(void);
void mlfqs_update_all(void);
void mlfqs_update_all_recent_cpu(void);
void mlfqs_update_all_priority(void);


/* Initializes the threading system by transforming the code
   that's currently running into a thread.  This can't work in
   general and it is possible in this case only because loader.S
   was careful to put the bottom of the stack at a page boundary.

   Also initializes the run queue and the tid lock.

   After calling this function, be sure to initialize the page
   allocator before trying to create any threads with
   thread_create().

   It is not safe to call thread_current() until this function
   finishes. */
void
thread_init (void) 
{
  ASSERT (intr_get_level () == INTR_OFF);

  lock_init (&tid_lock);
  list_init (&ready_list);
  list_init (&all_list);
  //sleep thread list인 list_sleep_thread를 초기화한다.
  //list_sleep_thread 초기화 추가
  list_init(&list_sleep_thread);

  /* Set up a thread structure for the running thread. */
  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT);
  initial_thread->status = THREAD_RUNNING;
  initial_thread->tid = allocate_tid ();



}

/* Starts preemptive thread scheduling by enabling interrupts.
   Also creates the idle thread. */
void
thread_start (void) 
{
  /* Create the idle thread. */
  struct semaphore idle_started;
  sema_init (&idle_started, 0);
  thread_create ("idle", PRI_MIN, idle, &idle_started);

  /* Start preemptive thread scheduling. */
  intr_enable ();

  /* Wait for the idle thread to initialize idle_thread. */
  sema_down (&idle_started);

  /* Initialize load_avg value */
  load_avg = convert_i2f(0);
}

/* Called by the timer interrupt handler at each timer tick.
   Thus, this function runs in an external interrupt context. */
void
thread_tick (void) 
{
  struct thread *t = thread_current ();

  /* Update statistics. */
  if (t == idle_thread)
    idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;

  /* Enforce preemption. */
  if (++thread_ticks >= TIME_SLICE)
    intr_yield_on_return ();
}

/* Prints thread statistics. */
void
thread_print_stats (void) 
{
  printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
          idle_ticks, kernel_ticks, user_ticks);
}

/* Creates a new kernel thread named NAME with the given initial
   PRIORITY, which executes FUNCTION passing AUX as the argument,
   and adds it to the ready queue.  Returns the thread identifier
   for the new thread, or TID_ERROR if creation fails.

   If thread_start() has been called, then the new thread may be
   scheduled before thread_create() returns.  It could even exit
   before thread_create() returns.  Contrariwise, the original
   thread may run for any amount of time before the new thread is
   scheduled.  Use a semaphore or some other form of
   synchronization if you need to ensure ordering.

   The code provided sets the new thread's `priority' member to
   PRIORITY, but no actual priority scheduling is implemented.
   Priority scheduling is the goal of Problem 1-3. */
tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux) 
{
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;

  ASSERT (function != NULL);

  /* Allocate thread. */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;

  /* Initialize thread. */
  init_thread (t, name, priority);
  tid = t->tid = allocate_tid ();

  /* Stack frame for kernel_thread(). */
  kf = alloc_frame (t, sizeof *kf);
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;

  /* Stack frame for switch_entry(). */
  ef = alloc_frame (t, sizeof *ef);
  ef->eip = (void (*) (void)) kernel_thread;

  /* Stack frame for switch_threads(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;
  sf->ebp = 0;

  /* Add to run queue. */
  thread_unblock (t);

  // compare created thread's priority with current thread's
  // if created thread's priority is higher than cur's, call thread_yield
  if (priority > thread_current()->priority) {
    thread_yield();
  }

  return tid;
}

/* Puts the current thread to sleep.  It will not be scheduled
   again until awoken by thread_unblock().

   This function must be called with interrupts turned off.  It
   is usually a better idea to use one of the synchronization
   primitives in synch.h. */
void
thread_block (void) 
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == INTR_OFF);

  thread_current ()->status = THREAD_BLOCKED;
  schedule ();
}

/* Transitions a blocked thread T to the ready-to-run state.
   This is an error if T is not blocked.  (Use thread_yield() to
   make the running thread ready.)

   This function does not preempt the running thread.  This can
   be important: if the caller had disabled interrupts itself,
   it may expect that it can atomically unblock a thread and
   update other data. */
void
thread_unblock (struct thread *t) 
{
  enum intr_level old_level;

  ASSERT (is_thread (t));

  old_level = intr_disable ();
  ASSERT (t->status == THREAD_BLOCKED);

  // insert t into ready_list by calling list_insert_ordered in stead of list_push_back
  list_insert_ordered(&ready_list, &t->elem, thread_compare_priority, NULL);

  t->status = THREAD_READY;
  intr_set_level (old_level);
}

/* Returns the name of the running thread. */
const char *
thread_name (void) 
{
  return thread_current ()->name;
}

/* Returns the running thread.
   This is running_thread() plus a couple of sanity checks.
   See the big comment at the top of thread.h for details. */
struct thread *
thread_current (void) 
{
  struct thread *t = running_thread ();
  
  /* Make sure T is really a thread.
     If either of these assertions fire, then your thread may
     have overflowed its stack.  Each thread has less than 4 kB
     of stack, so a few big automatic arrays or moderate
     recursion can cause stack overflow. */
  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_RUNNING);

  return t;
}

/* Returns the running thread's tid. */
tid_t
thread_tid (void) 
{
  return thread_current ()->tid;
}

/* Deschedules the current thread and destroys it.  Never
   returns to the caller. */
void
thread_exit (void) 
{
  ASSERT (!intr_context ());

#ifdef USERPROG
  process_exit ();
#endif

  /* Remove thread from all threads list, set our status to dying,
     and schedule another process.  That process will destroy us
     when it calls thread_schedule_tail(). */
  intr_disable ();
  list_remove (&thread_current()->allelem);
  thread_current ()->status = THREAD_DYING;
  schedule ();
  NOT_REACHED ();
}

/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void
thread_yield (void) 
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (cur != idle_thread) 
    // insert t into ready_list by calling list_insert_ordered in stead of list_push_back
    list_insert_ordered(&ready_list, &cur->elem, thread_compare_priority, NULL);
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}

/* Invoke function 'func' on all threads, passing along 'aux'.
   This function must be called with interrupts off. */
void
thread_foreach (thread_action_func *func, void *aux)
{
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  for (e = list_begin (&all_list); e != list_end (&all_list);
       e = list_next (e))
    {
      struct thread *t = list_entry (e, struct thread, allelem);
      func (t, aux);
    }
}

/* Sets the current thread's priority to NEW_PRIORITY. */
void
thread_set_priority (int new_priority) 
{
  //mlfqs스케쥴러를 사용할때는 새로운 priority를 임의로 설정할 수 없도록 해야 한다.
  if(!thread_mlfqs)
  {
  thread_current ()->origin_priority = new_priority;
  // set origin_priority as new one
  // then, it is nesessary to compare new origin priority and donation priority
  refresh_priority();

  // after set priority as new one,
  // compare ready list elements' priority with current's
  // if current's priority is lower, call thread_yield
  // do this by calling check_list_preemption()
  check_list_preemption();
  }
  else return;
}

/* Returns the current thread's priority. */
int
thread_get_priority (void) 
{
  return thread_current ()->priority;
}

/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice) 
{
  //intr disable
  enum intr_level prev_level = intr_disable();
  
  struct thread *cur = thread_current();

  //current_thread nice 설정
  cur->nice = nice;

  //nice 계산하였으니 priority 다시 계산
  mlfqs_priority(cur);

  //priority에 따라 다시 스케쥴링
  check_list_preemption();

  //intr level 이전으로 되돌림
  intr_set_level(prev_level);
}

/* Returns the current thread's nice value. */
int
thread_get_nice (void) 
{
  enum intr_level prev_level = intr_disable();
  struct thread *cur = thread_current();
  int cur_nice = cur->nice;

  intr_set_level(prev_level);
  return cur_nice;
}

/* Returns 100 times the system load average. */
int
thread_get_load_avg (void) 
{
  enum intr_level prev_level = intr_disable();
  int load_avg_100;

  load_avg_100 = convert_f2i_round(mul_inf(100, load_avg));
  intr_set_level(prev_level);
  return load_avg_100;
}

/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void)
{
  enum intr_level prev_level = intr_disable();
  int recent_cpu_100;
  struct thread *cur = thread_current();
  fixed_t recent_cpu = cur->recent_cpu;

  recent_cpu_100 = convert_f2i_round(mul_inf(100, recent_cpu));

  intr_set_level(prev_level);
  return recent_cpu_100;
}

/* Idle thread.  Executes when no other thread is ready to run.

   The idle thread is initially put on the ready list by
   thread_start().  It will be scheduled once initially, at which
   point it initializes idle_thread, "up"s the semaphore passed
   to it to enable thread_start() to continue, and immediately
   blocks.  After that, the idle thread never appears in the
   ready list.  It is returned by next_thread_to_run() as a
   special case when the ready list is empty. */
static void
idle (void *idle_started_ UNUSED) 
{
  struct semaphore *idle_started = idle_started_;
  idle_thread = thread_current ();
  sema_up (idle_started);

  for (;;) 
    {
      /* Let someone else run. */
      intr_disable ();
      thread_block ();

      /* Re-enable interrupts and wait for the next one.

         The `sti' instruction disables interrupts until the
         completion of the next instruction, so these two
         instructions are executed atomically.  This atomicity is
         important; otherwise, an interrupt could be handled
         between re-enabling interrupts and waiting for the next
         one to occur, wasting as much as one clock tick worth of
         time.

         See [IA32-v2a] "HLT", [IA32-v2b] "STI", and [IA32-v3a]
         7.11.1 "HLT Instruction". */
      asm volatile ("sti; hlt" : : : "memory");
    }
}

/* Function used as the basis for a kernel thread. */
static void
kernel_thread (thread_func *function, void *aux) 
{
  ASSERT (function != NULL);

  intr_enable ();       /* The scheduler runs with interrupts off. */
  function (aux);       /* Execute the thread function. */
  thread_exit ();       /* If function() returns, kill the thread. */
}

/* Returns the running thread. */
struct thread *
running_thread (void) 
{
  uint32_t *esp;

  /* Copy the CPU's stack pointer into `esp', and then round that
     down to the start of a page.  Because `struct thread' is
     always at the beginning of a page and the stack pointer is
     somewhere in the middle, this locates the curent thread. */
  asm ("mov %%esp, %0" : "=g" (esp));
  return pg_round_down (esp);
}

/* Returns true if T appears to point to a valid thread. */
static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}

/* Does basic initialization of T as a blocked thread named
   NAME. */
static void
init_thread (struct thread *t, const char *name, int priority)
{
  enum intr_level old_level;

  ASSERT (t != NULL);
  ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
  ASSERT (name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  strlcpy (t->name, name, sizeof t->name);
  t->stack = (uint8_t *) t + PGSIZE;
  t->priority = priority;
  t->magic = THREAD_MAGIC;

  /* initialize member variables for multiple donation */
  t->origin_priority = priority;
  t->lock_waiting_for = NULL;
  list_init(&(t->donations));

  /* initialize member variables for advanced scheduler */
  t->nice = 0;
  t->recent_cpu = 0;

  old_level = intr_disable ();
  list_push_back (&all_list, &t->allelem);
  intr_set_level (old_level);
}

/* Allocates a SIZE-byte frame at the top of thread T's stack and
   returns a pointer to the frame's base. */
static void *
alloc_frame (struct thread *t, size_t size) 
{
  /* Stack data is always allocated in word-size units. */
  ASSERT (is_thread (t));
  ASSERT (size % sizeof (uint32_t) == 0);

  t->stack -= size;
  return t->stack;
}

/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   idle_thread. */
static struct thread *
next_thread_to_run (void) 
{
  if (list_empty (&ready_list))
    return idle_thread;
  else
    return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/* Completes a thread switch by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.

   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.  This function is normally invoked by
   thread_schedule() as its final action before returning, but
   the first time a thread is scheduled it is called by
   switch_entry() (see switch.S).

   It's not safe to call printf() until the thread switch is
   complete.  In practice that means that printf()s should be
   added at the end of the function.

   After this function and its caller returns, the thread switch
   is complete. */
void
thread_schedule_tail (struct thread *prev)
{
  struct thread *cur = running_thread ();
  
  ASSERT (intr_get_level () == INTR_OFF);

  /* Mark us as running. */
  cur->status = THREAD_RUNNING;

  /* Start new time slice. */
  thread_ticks = 0;

#ifdef USERPROG
  /* Activate the new address space. */
  process_activate ();
#endif

  /* If the thread we switched from is dying, destroy its struct
     thread.  This must happen late so that thread_exit() doesn't
     pull out the rug under itself.  (We don't free
     initial_thread because its memory was not obtained via
     palloc().) */
  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread) 
    {
      ASSERT (prev != cur);
      palloc_free_page (prev);
    }
}

/* Schedules a new process.  At entry, interrupts must be off and
   the running process's state must have been changed from
   running to some other state.  This function finds another
   thread to run and switches to it.

   It's not safe to call printf() until thread_schedule_tail()
   has completed. */
static void
schedule (void) 
{
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);
  thread_schedule_tail (prev);
}

/* Returns a tid to use for a new thread. */
static tid_t
allocate_tid (void) 
{
  static tid_t next_tid = 1;
  tid_t tid;

  lock_acquire (&tid_lock);
  tid = next_tid++;
  lock_release (&tid_lock);

  return tid;
}


/* if current thread is idle thread, return 1. Else, return 0*/
int
thread_check_idle(void)
{
  struct thread *cur = thread_current ();
  if(cur == idle_thread) return 1;
  else return 0;
}

/*list_sleep_thread의 ticks가 작은 순서대로 삽입
list.c의 list_insert_ordered 사용 */
//list_insert_ordered (struct list *list, struct list_elem *elem, list_less_func *less, void *aux)

bool
compare_tick(const struct list_elem *a, const struct list_elem *b, void *aux)
{
  //a 가 b보다 작으면 true, 아니면 false
  struct thread *A = list_entry(a, struct thread, elem);
  struct thread *B = list_entry(b, struct thread, elem);
  if(A->wakeup_tick < B->wakeup_tick)
  {
    return true;
  }
  else return false;

}
void insert_sleep_thread()
{
  struct thread *cur = thread_current();
  list_insert_ordered (&list_sleep_thread, &cur->elem, compare_tick, NULL);
}

void thread_awake(int64_t ticks)
{
  struct list_elem *element; //이 element로 리스트를 돌아가며 확인
  struct thread *present_thread; //현재 element에 해당하는 thread 저장

  for(element = list_begin(&list_sleep_thread) ; element != list_end(&list_sleep_thread) ; )
  {
    present_thread = list_entry(element, struct thread, elem);
    if(present_thread -> wakeup_tick <= ticks) //현재 틱보다 wakeup tick이 작거나 같으면 sleep 리스트에서 제거하고 unblocking해야한다.
    {
      element = list_remove(element); //unblocking 후에 list_remove하면 오류발생. unblocking하면 ready list에 들어가 ready list에서 제거하기 때문이다.
      thread_unblock(present_thread);
    }
    else break;
  }
}
int64_t get_next_awake_tick()
{
  struct thread *first_thread;
  first_thread = list_entry(list_begin(&list_sleep_thread), struct thread, elem);
  //tick의 오름차순이므로 첫번째 원소가 최소 tick가지는 원소, 즉 next awake tick이다.
  return first_thread-> wakeup_tick;
}

/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);


// A function comparing threads priority
// return true if t1's priority is higher than t2's
bool thread_compare_priority(const struct list_elem *e1, const struct list_elem *e2, void *aux UNUSED) {
  struct thread *t1 = list_entry(e1, struct thread, elem);
  struct thread *t2 = list_entry(e2, struct thread, elem);
  return (t1->priority > t2->priority);
}

void check_list_preemption(void) {
  if (list_empty(&ready_list)) return;
  else {
    struct thread *t_ready = list_entry(list_begin(&ready_list), struct thread, elem);
    struct thread *t_cur = thread_current();
    if(t_ready->priority > t_cur->priority) {
      thread_yield();
    }
  }
}

// 다른 함수에서 호출할 때 t=thread_current(), depth=0으로 호출하도록 한다.
void donate_priority(struct thread* t, int depth) {
  struct thread* t_holder;
  if(depth >= NESTED_DEPTH_MAX || t->lock_waiting_for==NULL)
    return;
  else {
    t_holder = t->lock_waiting_for->holder;
    // if current thread priority is higher than holder thread,
    // donate current thread priority to holder thread
    if (t->priority > t_holder->priority)
      t_holder->priority = t->priority;
    donate_priority(t_holder, depth+1);
  }
}


void remove_with_lock(struct lock *lock) {
  struct thread* t_cur = thread_current();
  struct list_elem* cur_elem = list_begin(&(t_cur->donations));
  struct thread* cur_elem_thread;
  while(cur_elem!=list_end(&(t_cur->donations))) {
    cur_elem_thread = list_entry(cur_elem, struct thread, donation_elem);
    if(cur_elem_thread->lock_waiting_for == lock)
      cur_elem = list_remove(cur_elem);
    else
      cur_elem = list_next(cur_elem);
  }
}

// donations 리스트가 변화하였을 때 priority도 그에 따라 함께 변화하도록 한다.
void refresh_priority(void) {
  struct thread* t_cur = thread_current();
  int origin_priority = t_cur->origin_priority;
  int donations_max_priority = PRI_MIN - 1;
  
  // donations 리스트에서 가장 높은 priority가 origin_priority 보다 높은 경우
  // 해당 priority를 갖도록 한다.
  if(!list_empty(&(t_cur->donations))) {
    donations_max_priority = (list_entry(list_max(&(t_cur->donations), thread_compare_priority, NULL), struct thread, donation_elem))->priority;
  }

  t_cur->priority = max(origin_priority, donations_max_priority);
}


/* Functions for Advanced Scheduler implementation */
void mlfqs_priority(struct thread *t) {
  int priority;
  // first, check if t is idle thread
  if (t != idle_thread) {
    // priority calculation part
    // priority = PRI_MAX - (recent_cpu / 4) - (nice * 2)
    priority = calc_priority(t->recent_cpu, t->nice);

    if (priority < PRI_MIN)
      t->priority = PRI_MIN;
    else if (priority > PRI_MAX)
      t->priority = PRI_MAX;
    else
      t->priority = priority;
  }
}
int calc_priority(fixed_t _recent_cpu, int _nice) {
  // return priority = PRI_MAX - (recent_cpu / 4) - (nice * 2)
  int priority;
  priority = conver_f2i_round(sub_inf(PRI_MAX, add_f(div_xbn(_recent_cpu, 4), convert_i2f(2*_nice))));
  return priority;
}


void mlfqs_recent_cpu (struct thread *t) {
  t->recent_cpu = calc_recent_cpu(load_avg, t->recent_cpu, t->nice);
}
fixed_t calc_recent_cpu(fixed_t _load_avg, fixed_t _recent_cpu, int _nice) {
  // return recent_cpu = (2*load_avg)/(2*load_avg + 1) * recent_cpu + nice
  return add_inf(_nice, mul_f(div_xby(mul_inf(2, _load_avg), add_inf(1, mul_inf(2, _load_avg))), _recent_cpu));
}


void mlfqs_load_avg(void) {
  int num_ready_threads = list_size(&ready_list);
  // ready_list 외에도 현재 실행 중인 thread가 idle thread가 아닌 경우,
  // load_avg의 계산을 위해 포함해야 한다.
  struct thread *t_cur = thread_current();
  if(t_cur != idle_thread) num_ready_threads++;
  
  load_avg = calc_load_avg(load_avg, num_ready_threads);
}
fixed_t calc_load_avg(fixed_t _load_avg, int _ready_threads) {
  // return load_avg = (59/60)*load_avg + (1/60)*ready_threads
  return add_f(mul_f(div_xbn(convert_i2f(59), 60), _load_avg), mul_inf(_ready_threads, div_xbn(convert_i2f(1), 60)));
}


void mlfqs_recent_cpu_incr(void) {
  struct thread *t_cur = thread_current();
  // check if current thread is idle_thread,
  // if not, increase recent_cpu value of current thread by 1
  if (t_cur!=idle_thread)
    t_cur->recent_cpu = add_inf(1, t_cur->recent_cpu);
}


// recalculate recent_cpu and priority of all threads
void mlfqs_update_all(void) {
  mlfqs_update_all_recent_cpu();
  mlfqs_update_all_priority();
}
void mlfqs_update_all_recent_cpu(void) {
  struct list_elem* elem=list_begin(&all_list);
  while(elem!=list_end(&all_list)) {
    mlfqs_recent_cpu(list_entry(elem, struct thread, allelem));
    elem = list_next(elem);
  }
}
void mlfqs_update_all_priority(void) {
  struct list_elem* elem=list_begin(&all_list);
  while(elem!=list_end(&all_list)) {
    mlfqs_priority(list_entry(elem, struct thread, allelem));
    elem = list_next(elem);
  }
  // after update all, should sort ready_list for new updated priority
  list_sort(&ready_list, thread_compare_priority, NULL);
}