#include <stdlib.h>
#include <stdbool.h>

#define ALIGNMENT 16 /* The alignment of all payloads returned by umalloc */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/*
 * memory_block_t - Represents a block of memory managed by the heap. The 
 * struct can be left as is, or modified for your design.
 * In the current design bit0 is the allocated bit
 * bits 1-3 are unused.
 * and the remaining 60 bit represent the size.
 */
typedef struct memory_block_struct {
    size_t block_size_alloc;
    struct memory_block_struct *next;
} memory_block_t;

// Helper Functions, this may be editted if you change the signature in umalloc.c

/*
*  STUDENT TODO:
*      Write 1-2 sentences for each function explaining what it does.
*      Don't just repeat the name of the function back to us.
*/

//returns true if memory block has been allocated
bool is_allocated(memory_block_t *block);
//changes header of memory block to match that of an allocated memory block
void allocate(memory_block_t *block);
//changes header ofmemory block to match that of an unallocated memory block
//also changes free lsit
void deallocate(memory_block_t *block);
//returns size of memory block
size_t get_size(memory_block_t *block);
//returns a pointer to the memory block pointed to by the input memory block
memory_block_t *get_next(memory_block_t *block);
//updates the header of the input block with the given stae
void put_block(memory_block_t *block, size_t size, bool alloc);
//returns the data stored in the input memory block
void *get_payload(memory_block_t *block);
//returns the memory block with the data inputted
memory_block_t *get_block(void *payload);

//finds a memory block of at least the input ize
memory_block_t *find(size_t size);
//extends the current heap if more memory is requested
memory_block_t *extend(size_t size);
//splits the input memory block, with one block being at least the input size
memory_block_t *split(memory_block_t *block, size_t size);
//combines the input memory block with a neighboring memory block
memory_block_t *coalesce(memory_block_t *block);


// Portion that may not be edited
int uinit();
void *umalloc(size_t size);
void ufree(void *ptr);