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
    memory_block_t* cur = search_entry;

    //with search entry code
    while(cur && (uint64_t) cur->next != (uint64_t) search_entry){
        if (get_size(cur) >= size && get_size(cur) - SPLIT_THRESHOLD <= size) {
        //we don't want to give up free head
            if ((uint64_t) cur == (uint64_t) free_head && cur->next) { 
                //search from the next free block
                cur = cur -> next;
            } else if ((uint64_t) cur == (uint64_t) free_head) {
                //free head is the only remaining free block that works
                //extend, search again
                extend(4*PAGESIZE);
                return find(size);
            } else {
                //cur is not the free head, so we can just return after handling pointers
                remove_from_free_list(cur); // must write
                return cur;
            }
        } else if (get_size(cur) - SPLIT_THRESHOLD >= size) {
        //cur must be split, no pointers to handle.
            search_entry = (cur->next) ? get_next(cur) : free_head;
            return split(cur, size);
        } else {
        //cur is too small, go to next
            cur = cur -> next;
        }
    }
    
    int size_multiplier = 1;
    while (size_multiplier * PAGESIZE <= size) size_multiplier++;
    //if size multiplier is > 16, csbrk will shit the bed for me
    extend(size_multiplier*PAGESIZE);
    return find(size);
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t *extend(size_t size) {
    //obtain new heap
    void* heap = csbrk(size);
    if (!heap) return NULL;

    //check for block alignment
    int offset = 0;
    while ((uint64_t) heap % ALIGNMENT != 0) {
        offset++;
        (uint64_t) heap++;
    } //could I ever go out of bounds?

    //make the new arena a freeblock
    put_block(heap, size - offset, 0);
    memory_block_t* new_arena = (memory_block_t*) heap;

    if ((uint64_t) new_arena < (uint64_t) free_head) {
        //new arena is before current arena
        new_arena -> next = free_head;
        free_head = new_arena;
    }else {//new arena comes after current arena
        memory_block_t* cur_free = free_head;
        while(cur_free -> next) cur_free = cur_free -> next;
        cur_free -> next = new_arena;
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

    //update size of free block
    block->block_size_alloc = get_size(block) - size;
    
    // preserve allocated status
    put_block((memory_block_t*) (((uint64_t) block)+get_size(block)), size, true);
    // assume the allocated part is free

    return (memory_block_t*) (((uint64_t) block)+get_size(block));
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
 //ONLY DOES LEFT COALESCING. ALL COALESCE CASES CAN BE MADE INTO LEFT COALESCING CASES
memory_block_t *coalesce(memory_block_t *block) {
    block -> block_size_alloc += get_size((memory_block_t*) 
        (((uint64_t) block) + get_size(block)));
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

    //check for block alignment
    int offset = 0;
    while ((uint64_t) heap % ALIGNMENT != 0) {
        offset++;
        heap = (void*) (((uint64_t)heap)+1);
    } //could I ever go out of bounds?

    //set beginning of free list to beginning of arena
    //free list is null terminated
    if (!free_head) {
        free_head = heap;
        search_entry = free_head;
        free_head->next = NULL;
    }

    //make the new arena a freeblock
    put_block(heap, size - offset, 0);

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
    
    //make size a data aligned size
    while (size % ALIGNMENT != 0) size++;
    //find right block
    memory_block_t* mblock = find(size);
    
    //set memory block as allocated
    allocate(mblock);

    //magic number
    mblock->next = (memory_block_t*) 0xDEADBEEF;

    return (void*) ((uint64_t) mblock);
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

    //make sure I'm not double freeing
    //what if entire block is allocated, free head null
    //free head is not at beginning of csbrk block
    //    meaning free head must be moved.

    //typecast for ease
    //assume void* ptr points to the payload, so convert to header pointer
    memory_block_t* new_free_block = (memory_block_t*) ((uint64_t) ptr);
    //reset allocated flag
    new_free_block -> block_size_alloc = new_free_block -> block_size_alloc & ~(0x1);

    //look for the two free blocks to place the new one in between
    memory_block_t* cur_free_block = free_head;

    while (cur_free_block && cur_free_block -> next) {
        if ((uint64_t) cur_free_block < (uint64_t) new_free_block &&
            (uint64_t) new_free_block < (uint64_t) (cur_free_block -> next)) {
                if (((uint64_t) new_free_block) + ((uint64_t) new_free_block -> block_size_alloc) ==
                    (uint64_t) get_next(cur_free_block) ) {
                    coalesce(new_free_block);
                    new_free_block->next = cur_free_block->next->next;
                    cur_free_block->next = new_free_block;
                    //maybe free coalesced block
                } else {
                    new_free_block -> next = cur_free_block -> next;
                    cur_free_block -> next = new_free_block;
                }
                if (((uint64_t) cur_free_block) + ((uint64_t) cur_free_block -> block_size_alloc) ==
                    (uint64_t) new_free_block ) {
                    coalesce(cur_free_block);
                    cur_free_block -> next = new_free_block -> next;
                } else {
                    cur_free_block -> next = new_free_block;
                }
            break;
        } else cur_free_block = cur_free_block -> next;
    }

    //the new free block belongs at the end
    if ( !(cur_free_block -> next) ) {
        if (((uint64_t) cur_free_block) + ((uint64_t) cur_free_block -> block_size_alloc) ==
            (uint64_t) new_free_block) {
                coalesce(cur_free_block);
        } else {
            cur_free_block -> next = new_free_block;
            new_free_block -> next = NULL;
        }
   }
}

/*
removes the input free block from the free list
*/
void remove_from_free_list(memory_block_t* remove) {
    assert(!is_allocated(remove));
    assert(remove);

    //find previous free block
    memory_block_t* temp = free_head;
    while ((uint64_t) temp->next != (uint64_t) remove) temp = get_next(temp);
    //skip block to remove
    temp->next = remove->next;
    //null out next pointer of block to remove
    remove->next = NULL;
}