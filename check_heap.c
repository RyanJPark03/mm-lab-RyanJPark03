
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
        do {
            //checks if pointers in free list point to valid free blocks
            if (is_allocated(cur)) return -1;
            cur=cur->next;
        } while (cur);

        sbrk_block* cur_arena = sbrk_blocks;
        
        while (cur_arena) {
            memory_block_t* cur_block = (memory_block_t*) (cur_arena->sbrk_start);
             while (cur_block->next) {

                printf("Arena Start: %lu    \nArena End: %lu       \nCur_Block: %lu  \nSize of Arena: %lu                      \nSize of cur block: %lu\n",
                     cur_arena->sbrk_start, cur_arena->sbrk_end, (uint64_t)cur_block, (uint64_t) cur_arena->sbrk_start - (uint64_t) cur_arena->sbrk_end, cur_block->next->block_size_alloc & ~(0x1));

                //checks if block is inside an arena
                unsigned long block_size = get_size(cur_block->next);
                unsigned long arena_size = cur_arena->sbrk_start - cur_arena->sbrk_end;
                
                if (!check_malloc_output((cur_block->next + 1), block_size - sizeof(memory_block_t))) {
                    if (block_size <= 0 || block_size > arena_size) return -1;
                }
                //get size is 0...
                //checks alignment
                if ((uint64_t) cur_block % ALIGNMENT != 0) return -1;
                
                cur_block = (memory_block_t*) 
                    (((uint64_t) cur_block)+get_size(cur_block));
            }
            //checks to make sure block ends at end of heap. Checks overlap, since if overlap is
            //occurring, the size field will be corrupted, leading to an instance in which the rvalue
            //of cur_block does not coincide with the last address of the heap.
            // if ((uint64_t) cur_block <= cur_arena->sbrk_end) return -1;
            cur_arena = cur_arena->next;
        }

    return 0;
}