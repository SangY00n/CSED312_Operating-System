#include "vm/swap.h"
#include "threads/vaddr.h"

void
swap_init()
{   
    int sector_num = PGSIZE / BLOCK_SECTOR_SIZE; //값 : 8 (한 page당 sector 개수)
    swap_block = block_get_role(BLOCK_SWAP); //swap을 위한 block을 swap_block에 저장
    swap_table = bitmap_create(block_size(swap_block)/ sector_num); //사이즈만큼 bitmap 생성

    //bitmap 0으로 초기화(0 : free | 1 : used)
    bitmap_set_all(swap_table, false);
    lock_init(&swap_lock);
}


// swap partition -> main memory
void
swap_in(struct page *page, int index, void*kaddr)
{
    lock_acquire(&swap_lock);
    bitmap_set(swap_table, index, false); //해당 id를 free로 표시
    int i;
    //page size는 4KB, sector size는 512바이트
    //한 page에 8개의 sector가 존재한다....
    //slot index에 8(pgsize/block_sector_size)을 곱해서 sector번호로 변환
    int sector_num = PGSIZE / BLOCK_SECTOR_SIZE;

    for(i = 0 ; i < sector_num ; i++)
    {
        block_read(swap_block, index*sector_num, kaddr + BLOCK_SECTOR_SIZE*i ); // 순서대로 block, sector, number : Reads sector SECTOR from BLOCK into BUFFER
    }

    lock_release(&swap_lock);
}

// main memory -> swap partition
int
swap_out(void* kaddr)
{
    int index;
    int i;
    int sector_num;

    lock_acquire(&swap_lock);
    index = bitmap_scan_and_flip(swap_table, 0, 1, false); //swap table에서 0부터 시작해서 false인 bit 하나 찾아서 true로 flip

    sector_num = PGSIZE / BLOCK_SECTOR_SIZE;

    for(i = 0 ; i < sector_num ; i++)
    {
        block_write(swap_block, index*sector_num + i, kaddr + BLOCK_SECTOR_SIZE*i );
    }
    lock_release(&swap_lock);

    return index;
}