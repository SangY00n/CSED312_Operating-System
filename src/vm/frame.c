#include "vm/frame.h"
#include "vm/swap.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"

static struct list_elem *clock_hand; //clock 알고리즘에 사용될 시계바늘

//frame list와 lock 초기화 함수
void
frame_init(void) {
    clock_hand = NULL;
    list_init(&frame_table);
    lock_init(&frame_lock);
}

//4test
void *
frame_alloc(struct page *page)
{  
    struct frame *new_frame = malloc(sizeof(struct frame)); //새롭게 할당할 frame
    if(new_frame == NULL) return NULL;

    void* kaddr;
    enum palloc_flags flag = page->page_type == PT_ZERO ? PAL_ZERO : PAL_USER;
    if(new_frame==NULL) return NULL;

    lock_acquire(&frame_lock);
    kaddr = palloc_get_page(PAL_USER | flag);

    if(kaddr == NULL) //frame이 가득 찬 경우, evict 필요
    {
        evict_page();
        kaddr = palloc_get_page(PAL_USER | flag);
        ASSERT(kaddr != NULL);
    }
    
    new_frame->kaddr = kaddr;
    new_frame->page = page;
    new_frame->thread = thread_current();
    new_frame->is_accessed = true;
    page->frame = new_frame;
    list_push_back(&frame_table, &new_frame->elem);

    lock_release(&frame_lock);
    return kaddr;
}


//매개변수가 kaddr이면 get_frame으로 frame을 얻을 수 있다.
//일단은 frame받아서 free하는 것으로 구현
void
frame_free(struct frame *frame)
{
    struct list_elem *e;
    struct frame *list_frame;

    if(frame == NULL) return; //예외처리

    lock_acquire(&frame_lock); //critical section

    for(e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e))
    {
        list_frame = list_entry(e, struct frame, elem);

        if(list_frame == frame)
        {
            list_remove(e);
            break;
        }
    }
    if(list_frame != frame) return; //frame table에서 못찾은 경우 예외처리

    palloc_free_page(frame->kaddr);
    free(frame);

    lock_release(&frame_lock);
}

void
evict_page(void)
{
    int index;
    bool is_dirty = false;
    struct frame *cur;
    struct thread *t = thread_current();
    while(1)
    {
        if(clock_hand == NULL || list_next(clock_hand) == list_end(&frame_table))
            {
                clock_hand = list_begin(&frame_table);
            }
        else
        {
            clock_hand = list_next(clock_hand);
        }

            cur = list_entry(clock_hand, struct frame, elem);
        if(cur->is_accessed == true)
        {
            cur->is_accessed = false;
        }
        else break;
    }
    
  struct page *cur_page = cur->page;
  pagedir_clear_page(cur->thread->pagedir, cur_page->vaddr);
  index = swap_out(cur->kaddr);
  is_dirty = pagedir_is_dirty(cur->thread->pagedir, cur->kaddr) || pagedir_is_dirty(cur->thread->pagedir, cur->page->vaddr);
  set_page_to_swap(cur_page, index, is_dirty);

  palloc_free_page(cur->kaddr);
  list_remove(&cur->elem);
}

