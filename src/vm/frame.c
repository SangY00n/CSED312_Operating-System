#include "vm/frame.h"
#include "vm/swap.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"

struct list_elem *clock_hand; //clock 알고리즘에 사용될 시계바늘

//frame list와 lock 초기화 함수
void
frame_init(void) {
    clock_hand = NULL;
    list_init(&frame_table);
    lock_init(&frame_lock);
}

void *
frame_alloc(struct page *page)
{
    struct frame *new_frame = malloc(sizeof(struct frame)); //새롭게 할당할 frame
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

//kaddr로 frame 찾는 함수
//사용할까? 일단보류
struct frame*
get_frame(void* kaddr)
{
    struct list_elem *e;
    struct frame *frame;

    for(e = list_begin(&frame_table);e != list_end(&frame_table); e = list_next(e))
    {
        frame = list_entry(e, struct frame, elem);

        if(frame->kaddr == kaddr) return frame;
    }
    return NULL;
}

void
evict_page(void)
{
    int index;
    bool is_dirty = false;
    struct frame *cur;

    while(1)
    {
        if(clock_hand == NULL || list_back(&frame_table))
            clock_hand = list_front(&frame_table);
        else
            clock_hand = list_next(clock_hand);

            cur = list_entry(clock_hand, struct frame, elem);

            if(pagedir_is_accessed(cur->thread->pagedir, cur->page->vaddr)) //accesed bit 1
            {
                pagedir_set_accessed(cur->thread->pagedir, cur->page->vaddr, false);
            }
            else //accessed bit 0
            {
                pagedir_clear_page(cur->thread->pagedir, cur->page->vaddr);
                        // is_dirty = pagedir_is_dirty(cur_frame->thread->pagedir, cur_frame->page->vaddr); //아마 오류날수도 있음. page일지 kaddr일지 생각해보자.
                is_dirty = pagedir_is_dirty(cur->thread->pagedir, cur->kaddr); // 이렇게 바꿔도 결과 똑같음..
                break;
                //cur_frame을 victim frame으로 설정
            } 
    }
    
    //victim frame finded
    swap_out(cur->kaddr);
    index = swap_out(cur->kaddr);
    set_page_to_swap(cur->page, index, is_dirty);

    // 위에꺼 대신 아래와 같이 해줘야 할 듯!!
    palloc_free_page(cur->kaddr);
    list_remove(&cur->elem);

    // frame_free(cur_frame); free frame하면 안될 듯. 이미할당되어있으니
}
