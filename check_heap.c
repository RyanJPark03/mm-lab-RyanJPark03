
#include "umalloc.h"
#include "csbrk.h"

//Place any variables needed here from umalloc.c or csbrk.c as an extern.
extern memory_block_t *free_head;
extern sbrk_block *sbrk_blocks;

/*
 * check_heap -  used to check that the heap is still in a consistent state.
 
 * STUDENT TODO:
 * Required to be completed for checkpoint 1:
 *      - Check that pointers in the free list point to valid free blocks. 
 Blocks should be within the valid heap addresses: look at csbrk.h for some clues.
 *        They should also be allocated as free.
 *      - Check if any memory_blocks (free and allocated) overlap with each other.
  Hint: Run through the heap sequentially and check that
 *        for some memory_block n, memory_block n+1 has a sensible block_size 
 and is within the valid heap addresses.
 *      - Ensure that each memory_block is aligned. 
 * 
 * Should return 0 if the heap is still consistent, otherwise return a non-zero
 * return code. Asserts are also a useful tool here.
 */
int check_heap() {
        //check all free blocks
        memory_block_t *cur = free_head;
        do {
            if (is_allocated(cur)) {
                //checks if pointers in free list point to valid free blocks
            }
            //next is null check
            if (!cur->next) return -1;
            cur=cur->next;
        } while ((uint64_t) cur != (uint64_t) free_head);

        sbrk_block* cur_arena = sbrk_blocks[0];
        


        // //check overlap
        // //loop through all arenas
        // sbrk_block* cur_arena = sbrk_blocks;
        // while (cur_arena) {

        //     //loop through all blocks in arena
        //     uint64_t cur_block = (uint64_t) cur_arena -> sbrk_start;
        //     int cur_block_size = get_size((memory_block_t*) cur_block);
        //     while (cur_block + cur_block_size <= cur_arena -> sbrk_end) {

        //             //alignment check
        //             if (! (cur_block % ALIGNMENT) || 
        //                 ! (cur_block_size % ALIGNMENT) ) {
        //                 return -1;
        //             }
        //         //increment arena and check if in bounds (if header corrupted or incorrect,
        //         // could end up out of bounds)
        //         cur_block = cur_block + (cur_block_size & ~(0x1));

        //         if (check_malloc_output((memory_block_t*) cur_block, cur_block_size)) return -1;
        //         cur_block_size = get_size((memory_block_t*) cur_block);
        //         }
        // }

    return 0;
}