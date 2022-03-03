#include "umalloc.h"
#include "csbrk.h"
#include "ansicolors.h"
#include <stdio.h>
#include <assert.h>

//size in memory_block_t includes header size.
const int SPLIT_THRESHOLD = 32; //minimum size for a split (16 + 16) for another header+payload combo
const char author[] = ANSI_BOLD ANSI_COLOR_RED "Ryan Park rjp2764" ANSI_RESET;

/*
 * The following helpers can be used to interact with the memory_block_t
 * struct, they can be adjusted as necessary.
 */

// A sample pointer to the start of the free list.
memory_block_t *free_head;
memory_block_t *second_head;

//pointer to search spot in free ist
// memory_block_t *search_entry;

/*
 * is_allocated - returns true if a block is marked as allocated.
 */
bool is_allocated(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & 0x1;
}

/*
 * allocate - marks a block as allocated.
 */
void allocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc |= 0x1;
}


/*
 * deallocate - marks a block as unallocated.
 */
void deallocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc &= ~0x1;
}

/*
 * get_size - gets the size of the block.
 */
size_t get_size(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & ~(ALIGNMENT-1);
}

/*
 * get_next - gets the next block. (for free blocks only)
 */
memory_block_t *get_next(memory_block_t *block) {
    assert(block != NULL);
    return block->next;
}

/*
 * put_block - puts a block struct into memory at the specified address.
 * Initializes the size and allocated fields, along with NUlling out the next 
 * field.
 */
void put_block(memory_block_t *block, size_t size, bool alloc) {
    assert(block != NULL);
    assert(size % ALIGNMENT == 0);
    assert(alloc >> 1 == 0);
    block->block_size_alloc = size | alloc;
    block->next = NULL;
}

/*
 * get_payload - gets the payload of the block.
 */
void *get_payload(memory_block_t *block) {
    assert(block != NULL);
    return (void*)(block + 1);
}

/*
 * get_block - given a payload, returns the block.
 */
memory_block_t *get_block(void *payload) {
    assert(payload != NULL);
    return ((memory_block_t *)payload) - 1;
}

/*
 *  STUDENT TODO:
 *      Describe how you select which free block to allocate. What placement strategy are you using?

 I'm simply choosing the first free block that fits, then incrementing a pointer to the next free block.
 There are some edge cases where I do not inrement second head, but in general, I increment a pointer
 to the next free block.
 */

/*
 * find - finds a free block that can satisfy the umalloc request.
 */
memory_block_t *find(size_t size) { //size is size of header and payload
    memory_block_t *cur_search = second_head;
    // printf("second head: %p\n", second_head);

    do {//search whole list
        // printf("cur_search address: %p\n", get_next(cur_search));

        int cur_size = get_size(get_next(cur_search));

        // printf("size of cur block: %lu\nsize of block to find: %lu\n",
        //     (uint64_t) cur_size, (uint64_t) size);
        
        //if size of next block works, return it
        if (cur_size >= size && cur_size <= size + SPLIT_THRESHOLD) {
            // printf("perfect fit\n");
            memory_block_t *found = get_next(cur_search);
            allocate(found);
            //handle free list pointers
            cur_search->next = get_next(found);
            //do next fit pointer incrementing
            second_head = (is_allocated(get_next(cur_search))) ? 
                second_head : get_next(cur_search);
            //what if i have found free_head?
            return found;
        } else if (cur_size >= size + SPLIT_THRESHOLD) {//next block is too big, split it
            // printf("time to split\n");
            //increment next fit pointer
            second_head = get_next(get_next(cur_search));
            //give newly split block
            return split(get_next(cur_search), size);
        } else {
            //next block doesn't fit. GG go next
            cur_search = get_next(cur_search);
        }
    } while((uint64_t)cur_search!=(uint64_t)second_head);

    // printf("time to extend\n");
    //at this point, we know nothing in the list fits.
    if (!extend(3*PAGESIZE)) return NULL;
    return split(get_next(second_head), size);
}

/*
 * extend - extends the heap if more memory is required. 
 */
memory_block_t *extend(size_t size) {
    //* STUDENT TODO 
    //obtain new heap
    void* heap = csbrk(size);
    if (!heap) return NULL;

    //make new heap a free block
    memory_block_t* new_arena = (memory_block_t*) heap;
    put_block(new_arena, size, false);

    //case 1: new arena belongs in front -> also edge case not worth worrying abt
    // if ((uint64_t)new_arena < (uint64_t)free_head) {
    //     new_arena->next = free_head;
    //     while ((uint64_t)(second_head->next) != (uint64_t) free_head) second_head = get_next(second_head);
    //     second_head->next = new_arena;
    //     //keep list in order of memory
    //     free_head = new_arena;
    //     return new_arena;
    // }

    // memory_block_t *temp = second_head;

    //case 2: arena belongs in the list -> just an edge case I shouldn't worry abt?
    //find the two block between which the new arena belongs
    // do {
    //     if ((uint64_t)temp < (uint64_t)new_arena &&
    //         (uint64_t)get_next(temp) > (uint64_t)new_arena) {
    //         //found the two blocks the arena goes in between
    //         new_arena->next = get_next(second_head);
    //         second_head->next = new_arena;
    //         return new_arena;
    //     }
    //     second_head = get_next(second_head);
    // } while ((uint64_t)temp != (uint64_t) second_head);

    //case 3: arena goes at end of list
    while ((uint64_t)get_next(second_head) != (uint64_t) free_head) second_head = get_next(second_head);
    second_head->next = new_arena;
    new_arena->next = free_head;

    return new_arena;
}

/*
 *  STUDENT TODO:
 *      Describe how you chose to split allocated blocks. Always? Sometimes? Never? Which end?

 I decided to split such that the free block will be on the "left" and the allocated block will be on the "right".
 I chose to implement a thresholded split as to guarantee a minimum block size for every free block. However,
 I do all the split checking on the calling side, rather than in the split function.
*/

/*
 * split - splits a given block in parts, one allocated, one free. Returns block with the input size.
 */
memory_block_t *split(memory_block_t *block, size_t size) {
    if (block == NULL) return NULL;

    //update size of free block
    //use direct block->size to preserve flag status
    block->block_size_alloc = block->block_size_alloc - size;

    memory_block_t* split_allocated = (memory_block_t*) 
            (((uint64_t) block)+get_size(block));

    // preserve allocated status
    put_block(split_allocated, size, true);
    // assume the allocated part is free

    return split_allocated; 
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
 //ONLY DOES LEFT COALESCING. ALL COALESCE CASES CAN BE MADE INTO LEFT COALESCING CASES
memory_block_t *coalesce(memory_block_t *block) {
    if ((uint64_t)block == (uint64_t)free_head) return block;
    //change size of input block
    block->block_size_alloc += get_size(get_next(block));
    //handle free list pointers
    block->next = get_next(get_next(block));
    return block;
}

/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    int size = 4 * PAGESIZE; 

    //obtain new heap
    void* heap = csbrk(size);
    if (!heap) return -1;
    
    //free head
    put_block(heap, sizeof(memory_block_t), false);

    //set beginning of free list to beginning of arena
    free_head = heap;

    memory_block_t *second = (memory_block_t*) ((uint64_t)heap + sizeof(memory_block_t));

    put_block(second, size - sizeof(memory_block_t), false);
    free_head -> next = second;
    second ->next = free_head;

    second_head = second;

    return 0;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory
 '
 TODO: handle making size a data aligned size here;
       increment search entry
       set found block as allocated
 */
void *umalloc(size_t size) {
    //* STUDENT TODO
    
    //find right block, add 16 for header size
    memory_block_t* mblock = find(ALIGN(size + sizeof(memory_block_t)));

    if(mblock==NULL) return NULL;

    // mblock->block_size_alloc = ALIGN(size + sizeof(memory_block_t));
    //set memory block as allocated
    allocate(mblock);

    //magic number
    mblock->next = (memory_block_t*) 0xDEADBEEF;

    //should be:
    return get_payload(mblock);
}

/*
 *  STUDENT TODO:
 *      Describe your free block insertion policy.
 I insert my free blocks into the free list in order of increasing memory. The free list is updated during
 free. I also coalesce during each free call.
*/

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {
    //* STUDENT TODO

    //typecast for ease
    //assume void* ptr points to the payload, so convert to header pointer
    memory_block_t* new_free_block = (memory_block_t*) ((uint64_t) ptr);

    //check to make sure I've been passed an allocated block
    //also stops double frees
    if ((uint64_t) new_free_block->next != 0xDEADBEEF ||
        !is_allocated(new_free_block)) return;

    //reset allocated flag
    deallocate(new_free_block);

    //case 1: comes before free head -> assuming arenas don't come before, can't happen
    //since free head never moves
    // if ((uint64_t) new_free_block < (uint64_t) free_head) {
    //     new_free_block->next = free_head;
    //     while ((uint64_t)get_next(second_head) != 
    //         (uint64_t) free_head) second_head = get_next(second_head);
    //     second_head -> next = new_free_block;
    //     free_head = new_free_block;

    //     //check if coalesce can happen
    //     if (((uint64_t) new_free_block) + get_size(new_free_block) == 
    //         (uint64_t) get_next(new_free_block)) coalesce(new_free_block);
    //     return;
    // }

    //case 2: comes somewhere in the list by address
    //look for the two free blocks to place the new one in between
    memory_block_t* cur_free_block = free_head;

     while ((uint64_t)get_next(cur_free_block) != (uint64_t) free_head) {
        if ((uint64_t) cur_free_block < (uint64_t) new_free_block &&
            (uint64_t) new_free_block < (uint64_t) get_next(cur_free_block)) {
            new_free_block -> next = get_next(cur_free_block);
            cur_free_block -> next = new_free_block;

            //check right then left coalesce
            if ((uint64_t)get_next(new_free_block) == (uint64_t)new_free_block + get_size(new_free_block)) {
                coalesce(new_free_block);
            }
            if ((uint64_t)new_free_block == (uint64_t)cur_free_block + get_size(cur_free_block)) {
                coalesce(cur_free_block);
            }
            return;
        } else 
            cur_free_block = cur_free_block -> next;
    }
    //case 3: hit end of list
    //check left coalesce of cur_free_block

    cur_free_block->next=new_free_block;
    new_free_block->next=free_head;
    if ((uint64_t)new_free_block == (uint64_t)cur_free_block + get_size(cur_free_block)) {
        coalesce(cur_free_block);
    }
}