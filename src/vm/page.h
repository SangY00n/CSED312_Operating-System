// #include "vm/frame.h"
#include <hash.h>
#include "filesys/off_t.h"

enum page_type { ZERO, SWAP, FILE, CLEAR };

struct page
{
    enum page_type page_type;
    void *vaddr;
    bool writable;

    struct frame* frame; // 찬호가 frame 파트 구현하면서 사용

    struct hash_elem hash_elem;

    struct file *file;
    off_t offset;      /*  */
    size_t read_bytes;  /* writen data size */
    size_t zero_bytes;  /* remaining data size to be filled with 0 */

    bool is_dirty;      /* dirty bit */    
};

unsigned page_hash_hash (const struct hash_elem *e, void *aux);
bool page_hash_less (const struct hash_elem *a,
                             const struct hash_elem *b,
                             void *aux);
struct page *page_find (void *addr);
void init_page_table(void);
void hash_elem_destructor (struct hash_elem *e, void *aux);
void free_page_table(void);
void free_page(struct page *page);
bool alloc_page_with_file(uint8_t *upage, bool writable, struct file *file, off_t offset, size_t read_bytes, size_t zero_bytes);
bool alloc_page_with_zero(uint8_t upage);

// bool set_page_to_swap (struct page *page, int asdfasdf, bool is_dirty);