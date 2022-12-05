#include "vm/page.h"
#include "threads/thread.h"
#include "threads/malloc.h"

unsigned page_hash_hash (const struct hash_elem *e, void *aux)
{
    const struct page *p = hash_entry(e, struct page, hash_elem);
    return hash_bytes(p->vaddr, sizeof(p->vaddr));
}

bool page_hash_less (const struct hash_elem *a,
                             const struct hash_elem *b,
                             void *aux)
{
    const struct page *pa = hash_entry(a, struct page, hash_elem);
    const struct page *pb = hash_entry(b, struct page, hash_elem);
    return (pa->vaddr < pb->vaddr);
}

struct page *page_find (void *addr)
{
    struct thread *cur_t = thread_current();
    struct page *temp_page = (struct page *)malloc(sizeof(struct page));
    struct hash_elem *e;

    temp_page->vaddr = addr;
    e = hash_find (cur_t->page_table, &temp_page->hash_elem);
    free(temp_page);
    if(e==NULL)
    {
        return e;
    }
    else
    {
        return hash_entry(e, struct page, hash_elem);
    }
}

void init_page_table(void)
{
    struct thread *cur_t = thread_current();
    cur_t->page_table = (struct hash *)malloc(sizeof(struct hash));
    hash_init(cur_t->page_table, page_hash_hash, page_hash_less, NULL);
}

void hash_elem_destructor (struct hash_elem *e, void *aux)
{
    struct page *page = hash_entry(e, struct page, hash_elem);
    if (page->page_type == CLEAR)
    {
        free_frame(page->frame);
    }
    else if (page->page_type == SWAP)
    {
        // SWAP 파트에서 다룰 예정
        // swap 관련 destory 처리
    }
    free_page(page);
}

void free_page_table(void)
{
    struct hash* page_table = thread_current()->page_table;
    hash_destroy(page_table, hash_elem_destructor);
    thread_current()->page_table = NULL;
}

void free_page(struct page *page)
{
    if (page == NULL) return;
    hash_delete(thread_current()->page_table, &page->hash_elem);
    free(page);
}

bool alloc_page_with_file (uint8_t *upage, bool writable, struct file *file, off_t offset, size_t read_bytes, size_t zero_bytes)
{
    struct thread *cur_t = thread_current();
    struct page *alloc_page = (struct page *)malloc(sizeof(struct page));
    if(alloc_page == NULL)
    {
        return false;
    }
    else
    {
        alloc_page->page_type = FILE;
        alloc_page->vaddr = upage;
        alloc_page->writable = writable;

        alloc_page->frame = NULL;

        alloc_page->file = file;
        alloc_page->offset = offset;
        alloc_page->read_bytes = read_bytes;
        alloc_page->zero_bytes = zero_bytes;

        alloc_page->is_dirty = false;

        // hash_insert는 insert를 시도하기 전 만약 동일한 element 존재 시 해당 element를 return 한다.
        if(hash_insert(cur_t->page_table, &alloc_page->hash_elem) == NULL)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

bool alloc_page_with_zero(uint8_t *upage)
{
    struct thread *cur_t = thread_current();
    struct page *alloc_page = (struct page *)malloc(sizeof(struct page));
    if(alloc_page == NULL)
    {
        return false;
    }
    else
    {
        alloc_page->page_type = ZERO;
        alloc_page->vaddr = upage;
        alloc_page->writable = true;

        alloc_page->frame = NULL;

        alloc_page->file = NULL;
        alloc_page->offset = -1;
        alloc_page->read_bytes = -1;
        alloc_page->zero_bytes = -1;

        alloc_page->is_dirty = false;

        // hash_insert는 insert를 시도하기 전 만약 동일한 element 존재 시 해당 element를 return 한다.
        if(hash_insert(cur_t->page_table, &alloc_page->hash_elem) == NULL)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

void set_page_to_swap (struct page *page, int swap_index, bool is_dirty)
{
    page->swap_index = swap_index;
    page->is_dirty = is_dirty;
    page->page_type = SWAP;
}

