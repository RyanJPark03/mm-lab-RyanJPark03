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
    if (alloc) block->next = 0xFEE1DEAD;
    else block->next = NULL;
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

 I'm simply choosing the first free block that fits.
 */

/*
 * find - finds a free block that can satisfy the umalloc request.
 */
memory_block_t *find(size_t size) { //size is size of payload, since header already exists.
    //? STUDENT TODO
    memory_block_t* cur = free_head;

    if (cur->block_size_alloc & 0xFFFFFFF0 >= size) return cur;
    
    while(cur->next){
        //Will allocated bit flag change outcome?
        if ((cur->next->block_size_alloc & 0xFFFFFFF0) >= size) {
            return cur->next;
        }else cur = cur->next;
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

    block->block_size_alloc = block->block_size_alloc - (size & 0xFFFFF0); // preserve allocated status
    put_block((memory_block_t*) ((char* block) + block->block_size_alloc), size, false); // will likely be allocated though
    return (memory_block_t*) ((char* block) + block->block_size_alloc);
}

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
    return 0;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 '
 TODO: handle making size a data aligned size here;
 TODO: handle splitting here;
 TODO: handle creating magic number here;
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
}