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
 */

/*
 * find - finds a free block that can satisfy the umalloc request.
 */
memory_block_t *find(size_t size) { //size is size of header and payload
    //? STUDENT TODO
    memory_block_t* cur_block = free_head;
    //worst fit will find biggest block
    memory_block_t* best_fit_block = free_head;

    //if possible, coalesce
    if (((uint64_t) cur_block) + get_size(cur_block) == (uint64_t) get_next(cur_block)) {
        coalesce(cur_block);
    }

    //if free head is the one to give away check:
    if (get_size(cur_block) >= size && get_size(cur_block) <=  size + SPLIT_THRESHOLD) {
        //free head not the only one in list. Just increment free head to next freeblock.
        if (get_next(free_head)) {
            free_head = get_next(free_head);
        } else { // freehead is last one. So I need to extend and do pointer properly.
            if ((uint64_t)extend(16*PAGESIZE) < (uint64_t) cur_block ) {
                free_head->next = NULL;
            } else {
                free_head = free_head -> next;
            }
        }
        return cur_block;
    }

    //handles case when free-head is the only block left in the list
    if (!get_next(cur_block)) {
        //this block is the only one in the list.
        if (get_size(cur_block) < size) return split(extend(16*PAGESIZE), size);
        else if (get_size(cur_block) >= size + SPLIT_THRESHOLD) return split(cur_block, size);
        else {
            if ((uint64_t)extend(16*PAGESIZE) < (uint64_t) cur_block ) {
                free_head->next = NULL;
            } else {
                free_head = free_head -> next;
            }
            return cur_block;
        }
    }

    while (get_next(cur_block)) {
        if ((uint64_t) get_next(cur_block) + get_size(get_next(cur_block)) ==
             (uint64_t) get_next(get_next(cur_block))) {
            coalesce(get_next(cur_block));
        }
        if (get_size(get_next(cur_block)) >= size &&
             get_size(get_next(cur_block)) <=  size + SPLIT_THRESHOLD) {
            cur_block->next = get_next(get_next(cur_block));
            return cur_block;
        } else if (get_size(get_next(cur_block)) < get_size(get_next(best_fit_block))) {
            best_fit_block = cur_block;
        }
        cur_block = get_next(cur_block);
    }

    if (get_size(get_next(best_fit_block)) < size) {
        return split(extend(16*PAGESIZE), size);
        //no pointers to handle here. all handled in extend.
    }
    //at this point, we know: get_size(best_fit_block) >= size + SPLIT_THRESHOLD.

    return split(get_next(best_fit_block), size);
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t *extend(size_t size) {
    //* STUDENT TODO 
    //obtain new heap
    void* heap = csbrk(size);
    if (!heap) return NULL;

    int offset = 0;
    while ((uint64_t) heap % ALIGNMENT != 0){
        offset++;
        heap = (memory_block_t*) ((uint64_t) heap + 1);
    }

    //make new heap a free block
    memory_block_t* new_arena = (memory_block_t*) heap;
    put_block(new_arena, size - offset, false);

    //sort free blocks by memory address
    if ((uint64_t) new_arena < (uint64_t) free_head) {
        //new arena is before current arena
        new_arena -> next = free_head;
        free_head = new_arena;
    }else {//new arena comes after current arena
        memory_block_t* cur_free = free_head;
        while(cur_free -> next) cur_free = cur_free -> next;
        cur_free -> next = new_arena;
        new_arena -> next = NULL;
    }
    return new_arena;
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
    // printf("size of block to split: %lu\nsize of block to allocate: %lu\n", get_size(block), size);

    //update size of free block
    //use direct block->size to preserve flag status
    block->block_size_alloc = block->block_size_alloc - size;
    
    //calculate position of next block
    memory_block_t* split_allocated = (memory_block_t*) 
            (((uint64_t) block) + get_size(block));

    //put new block at split location
    put_block(split_allocated, size, true);

    return split_allocated; 
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
 //ONLY DOES LEFT COALESCING. ALL COALESCE CASES CAN BE MADE INTO LEFT COALESCING CASES
memory_block_t *coalesce(memory_block_t *block) {
    block->block_size_alloc += get_size(get_next(block));
    block->next=get_next(get_next(block));
    if ((uint64_t) block + get_size(block) == (uint64_t) get_next(block)) {
        coalesce(block);
    }
    return block;
}

/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    int size = 16 * PAGESIZE; 

    //obtain new heap
    void* heap = csbrk(size);
    if (!heap) return -1;

    int offset = 0;
    while ((uint64_t) heap % ALIGNMENT != 0){
        offset++;
        heap = (memory_block_t*) ((uint64_t) heap + 1);
    }
    
    put_block(heap, size - offset, false);

    //set beginning of free list to beginning of arena
    if (!free_head) {
        free_head = (memory_block_t*) heap;
        free_head->next = NULL;
    }
    // printf("start/end of arena: %p, %p\n", free_head, (void*) ((uint64_t) free_head + size - offset));
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

    //set memory block as allocated
    allocate(mblock);

    //magic number
    mblock->next = (memory_block_t*) 0xDEADBEEF;

    // printf("returning: %p\n", mblock);
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

    //case 1: comes before free head
    if ((uint64_t) new_free_block < (uint64_t) free_head) {
        new_free_block->next = free_head;
        free_head = new_free_block;
        return;
    }

    //case 2: comes somewhere in the list by address
    //look for the two free blocks to place the new one in between
    memory_block_t* cur_free_block = free_head;

    while (get_next(cur_free_block)) {
        if ((uint64_t) cur_free_block < (uint64_t) new_free_block &&
            (uint64_t) new_free_block < (uint64_t) (cur_free_block -> next)) {
            new_free_block -> next = cur_free_block -> next;
            cur_free_block -> next = new_free_block;
            return;
        } else 
            cur_free_block = cur_free_block -> next;
    }
    //case 3: hit end of list
    cur_free_block->next=new_free_block;
    new_free_block->next=NULL;
}