#ifndef USERPROG_EXCEPTION_H
#define USERPROG_EXCEPTION_H

#include <stdbool.h>
#include "vm/page.h"

/* Page fault error code bits that describe the cause of the exception.  */
#define PF_P 0x1    /* 0: not-present page. 1: access rights violation. */
#define PF_W 0x2    /* 0: read, 1: write. */
#define PF_U 0x4    /* 0: kernel, 1: user process. */

#define MAX_STACK_SIZE 0x800000 // 8MB == 2^23B

void exception_init (void);
void exception_print_stats (void);
bool load_page(struct page *cur_page);
bool expand_stack(void *esp, void *fault_addr, void *page_addr);


#endif /* userprog/exception.h */
