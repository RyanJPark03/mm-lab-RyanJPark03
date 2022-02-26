
#include "umalloc.h"
#include "csbrk.h"
#include <stdio.h>

//Place any variables needed here from umalloc.c or csbrk.c as an extern.
extern memory_block_t *free_head;
extern sbrk_block *sbrk_blocks;

/*
 * check_heap -  used to check that the heap is still in a consistent state.
 
 * STUDENT TODO:
 * Required to be completed for checkpoint 1:
 *      c- Check that pointers in the free list point to valid free blocks. 
        c- Blocks should be within the valid heap addresses: look at csbrk.h for some clues.
 *      c- They should also be allocated as free.

 *      c- Check if any memory_blocks (free and allocated) overlap with each other.
        - Hint: Run through the heap sequentially and check that
 *        for some memory_block n, memory_block n+1 has a sensible block_size 
          and is within the valid heap addresses.
 *      c- Ensure that each memory_block is aligned. 
 * 
 * Should return 0 if the heap is still consistent, otherwise return a non-zero
 * return code. Asserts are also a useful tool here.
 */
int check_heap() {
        //check all free blocks
        memory_block_t *cur = free_head;
         while (cur) {
            //checks if pointers in free list point to valid free blocks
            if (cur->block_size_alloc & 0x1) return -1;
            cur=cur->next;
        }
        
        memory_block_t* cur_arena = (memory_block_t*) sbrk_blocks->sbrk_start;
        while ((uint64_t)cur_arena < sbrk_blocks->sbrk_end) {
            //
            if ((uint64_t)cur_arena + get_size(cur_arena) > sbrk_blocks->sbrk_end ||
            ((uint64_t)cur_arena < sbrk_blocks->sbrk_start) ||
            ((uint64_t)cur_arena % 16 != 0) ||
            (get_size(cur_arena) > sbrk_blocks->sbrk_end - sbrk_blocks->sbrk_start) ||
            (get_size(cur_arena) > sbrk_blocks->sbrk_end - sbrk_blocks->sbrk_start)) return -1;
            cur_arena = (memory_block_t*) ((uint64_t)cur_arena + get_size(cur_arena) + 16);
        }

    return 0;
}
