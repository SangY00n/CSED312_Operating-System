#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/synch.h" //for lock
#include "threads/thread.h" //for thread
#include <list.h> //for list
#include "threads/palloc.h"
#include "vm/page.h"



struct frame{
    struct page *page; //점유 중인 page가리키는 포인터
    void* kaddr; //kaddr = physical addr + PHYS_BASE

    struct thread *thread;
    
    struct list_elem elem;

    bool is_accessed; // clock algorithm을 통한 evict page를 위한 bit
};

struct list frame_table; //frame table을 이루는 list
struct lock frame_lock; //frame table 관리를 위한 lock

void frame_init(void);
void* frame_alloc(struct page *page);
void frame_free(struct frame *frame);
struct frame* get_frame(void* kaddr);
// struct frame* evict_page(void);
void evict_page(void);



#endif