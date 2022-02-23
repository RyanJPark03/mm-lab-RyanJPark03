#include "umalloc.h"
#include "csbrk.h"
#include "ansicolors.h"
#include <stdio.h>
#include <assert.h>

const int SPLIT_THRESHOLD = 128; //minimum size for a split (64 + 32) for another header+payload combo
const char author[] = ANSI_BOLD ANSI_COLOR_RED "Ryan Park rjp2764" ANSI_RESET;

/*
 * The following helpers can be used to interact with the memory_block_t
 * struct, they can be adjusted as necessary.
 */

// A sample pointer to the start of the free list.
memory_block_t *free_head;

//pointer to search spot in free ist
memory_block_t *search_entry;

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
 * get_next - gets the next block.
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
 
 */

/*
 * find - finds a free block that can satisfy the umalloc request.
 */
memory_block_t *find(size_t size) { //size is size of payload, since header already exists.
    //? STUDENT TODO
    //handle splitting!!
    memory_block_t* cur = free_head;

    if ( (cur->block_size_alloc & ~(ALIGNMENT - 1) )>= size) return cur;
    
    while(cur->next){
        //Will allocated bit flag change outcome?
        if ((cur->next->block_size_alloc & ~(ALIGNMENT - 1)) >= size) {
            return cur->next;
        }else {//if too big split
        cur = cur->next;
        }
    }
    
    return NULL;
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t *extend(size_t size) {
    //? STUDENT TODO
    return NULL;
}

/*
 *  STUDENT TODO:
 *      Describe how you chose to split allocated blocks. Always? Sometimes? Never? Which end?

 I decided to split such that the free block will be on the "left" and the allocated block will be on the "right".
 I chose to implement a thresholded split as to guarantee a minimum block size for every free block.
*/

/*
 * split - splits a given block in parts, one allocated, one free. Returns block with the input size.
 */
memory_block_t *split(memory_block_t *block, size_t size) {
    //assume input size is always aligned
    //input size includes the size of the header

    //update size of free block
    block->block_size_alloc = block->block_size_alloc - size;

    // preserve allocated status
    put_block((memory_block_t*) ((uint64_t)block+block->block_size_alloc), size, true);
    // assume the allocated part is free

    return (memory_block_t*) ((uint64_t)block + block->block_size_alloc);
} // done for now

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
memory_block_t *coalesce(memory_block_t *block) {
    //? STUDENT TODO
    return NULL;
}



/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    //* STUDENT TODO 

    int size = 16 * PAGESIZE;
    //obtain new heap
    void* heap = csbrk(size);
    if (!heap) return -1;

    //account for header size in size
    int offset = 16;

    //check for block alignment
    while ((uint64_t) heap % ALIGNMENT != 0) {
        offset++;
        (uint64_t) heap++;
    } //could I ever go out of bounds?

    //set beginning of free list to beginning of arena
    if (!free_head) free_head = heap;    

    //make the new arena a freeblock
    put_block(heap, size - offset, 0);
    memory_block_t* new_arena = (memory_block_t*) heap;

    if ((uint64_t) new_arena < (uint64_t) free_head) {//new arena is before current arena
        new_arena -> next = free_head;
        free_head = new_arena;
    }else {//new arena comes after current arena
        memory_block_t* cur_free = free_head;
        while(cur_free -> next) cur_free = cur_free -> next;
        cur_free -> next = new_arena;
    }

    //set next of freeblock to self for circular linked list
    //not dealing with circular linked list for now
    // (memory_block_t*) free_head -> next = (memory_block_t*) heap;

    return 0;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory
 '
 TODO: handle making size a data aligned size here;
 TODO: increment free head to next free block.
 */
void *umalloc(size_t size) {
    //* STUDENT TODO
    return NULL;
}

/*
 *  STUDENT TODO:
 *      Describe your free block insertion policy.
*/

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {
    //* STUDENT TODO

    //typecast for ease
    memory_block_t* new_free_block = (memory_block_t*) ((uint64_t) ptr - 16);
    //reset allocated flag
    new_free_block -> block_size_alloc = new_free_block -> block_size_alloc & ~(0x1);

    /*new free block should never come before the free head. In the currrent arena, the free
    head is set to the beginning of the arena, and new mallocs will be splits off the arena, meaning
    they are at the end. So the origninal free head stays at the beginning of the current and gets
    smaller. If there is another arena to consider, I handle moving the free head when requesting new
    information.
    */

    //look for the two free blocks to place the new one in between
    memory_block_t* cur_free_block = free_head;
    while (cur_free_block && cur_free_block -> next) {
        if ((uint64_t) cur_free_block < (uint64_t) new_free_block &&
            (uint64_t) new_free_block < (uint64_t) (cur_free_block -> next)) {
            new_free_block -> next = cur_free_block -> next;
            cur_free_block -> next = new_free_block;
            break;
        } else cur_free_block = cur_free_block -> next;
    }

    //the new free block belongs at the end
    if ( !(cur_free_block -> next) ) {
        cur_free_block -> next = new_free_block;
        new_free_block -> next = NULL;
   }
} // done for now - complies