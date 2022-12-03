#ifndef VM_SWAP_H
#define VM_SWAP_H
#include <bitmap.h>
#include "vm/page.h"
#include "threads/synch.h"
#include "devices/block.h"

struct bitmap *swap_table;
struct block *swap_block;
struct lock *swap_lock;

//매개변수 추가할 것
void swap_init(void);
void swap_in(struct page *page, int index, void*kaddr);
int swap_out(void* kaddr);

#endif