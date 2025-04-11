#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)     ((b) + 1)
#define BLOCK_HEADER(ptr) ((struct _block *)(ptr) - 1)

static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;
static int current_heap_size = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes     */
   struct _block *next;  /* Pointer to the next _block of allocated memory      */
   bool   free;          /* Is this _block free?                                */
   char   padding[3];    /* Padding: IENTRTMzMjAgU3jMDEED                       */
};


struct _block *heapList = NULL; /* Free list to track the _blocks available */



/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = heapList;

#if defined FIT && FIT == 0
   /* First fit */
   //
   // While we haven't run off the end of the linked list and
   // while the current node we point to isn't free or isn't big enough
   // then continue to iterate over the list.  This loop ends either
   // with curr pointing to NULL, meaning we've run to the end of the list
   // without finding a node or it ends pointing to a free node that has enough
   // space for the request.
   // 
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

// \TODO Put your Best Fit code in this #ifdef block
#if defined BEST && BEST == 0
   ///** \TODO Implement best fit here **/
    struct _block *best = NULL;
    struct _block *prev_best = NULL;  // To track the previous block 
   
    while (curr != NULL) 
    {
        // First, let's check if the block is free and large enough to fit
        if (curr->free && curr->size >= size) 
        {
            // If no best block has been found yet, 
            // or the current block is the smallest fitting block
            if (!best || curr->size < best->size) 
            {
                best = curr;   // Update the best fit block
                prev_best = *last;  // Update the previous block to current last block
            }
        }

        *last = curr;   
        curr = curr->next;  // Move to the next block
    }

#endif

// \TODO Put your Worst Fit code in this #ifdef block
#if defined WORST && WORST == 0
   /** \TODO Implement worst fit here */
    struct _block *worst = NULL;
    struct _block *prev_worst = NULL;
   
    while (curr != NULL) 
    {
        //  check if the block is free and big enough 
        if (curr->free && curr->size >= size) 
        {
            // If no worst block has been found yet, 
            // or the current block is the largest fitting block
            if (!worst || curr->size > worst->size) 
            {
                worst = curr;   // Update the worst fit block
                prev_worst = *last;  
            }
        }

        *last = curr;          
        curr = curr->next; 
    }
#endif

// TODO Put your Next Fit code in this #ifdef block
#if defined NEXT && NEXT == 0
   /** \TODO Implement next fit here */
   struct _block *last_allocated = NULL;
   // Check if we need to start from the beginning of the heap
   // we need to start from the beginning if nothing has been allocated yet

   if (!last_allocated) 
   {
      last_allocated = heapList; 
   }

   struct _block *start = last_allocated; 
   
   // if somethin is allocated,
   // loop through the blocks starting from last_allocated
   do 
   {
      // if the block is free 
      // and large enough to fit the size
      if (last_allocated->free && last_allocated->size >= size) 
      {
         curr = last_allocated; // We've found a block that fits
         break; // so we don't need to keep searching
      }

      // Move last_allocated to the next block
      *last = last_allocated; 

      // If we reach the end of the list, 
      // go back to the start 
      // do the loop again and again until we find a fit
      if (last_allocated->next) 
      {
         last_allocated = last_allocated->next;
      } else 
      {
         last_allocated = heapList;
      }

    } 
    // Continue looping until we come back to the starting point
    while (last_allocated != start); 

#endif

   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size) 
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1) 
   {
      return NULL;
   }

   /* Update heapList if not set */
   if (heapList == NULL) 
   {
      heapList = curr;
   }

   /* Attach new _block to previous _block */
   if (last) 
   {
      last->next = curr;
   }

   /* Update _block metadata:
      Set the size of the new block and initialize the new block to "free".
      Set its next pointer to NULL since it's now the tail of the linked list.
   */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;\

   current_heap_size += size + sizeof(struct _block);
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size) 
{
	printf("abc");
   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0) 
   {
      return NULL;
   }

   /* Look for free _block.  If a free block isn't found then we need to grow our heap. */

   struct _block *last = heapList;
   struct _block *next = findFreeBlock(&last, size);

   /* TODO: If the block found by findFreeBlock is larger than we need then:
            If the leftover space in the new block is greater than the sizeof(_block)+4 then
            split the block.
            If the leftover space in the new block is less than the sizeof(_block)+4 then
            don't split the block.
   */
   
   // Track the total requested memory size
    num_requested += size;

    /* If the free block found by findFreeBlock */
    if (next != NULL) 
    {
        num_reuses++;  
        
      // if the block is larger than the sizeof(_block)+4 
      // then split the block. 
      if (next->size >= size + sizeof(struct _block) + 4) 
      {
         // allocated and free
         // size of the remaining free space after allocating 'size'
         struct _block *new_block = (struct _block *)((char *)next + size + sizeof(struct _block));
         
         new_block->size = next->size - size - sizeof(struct _block);  // left space
         new_block->free = true;  // then new block will be free
         new_block->next = next->next;  // link to the next block (same as next's next)
         
         // adjusting the size of the allocated block
         next->size = size;  
         next->next = new_block; 
         
         num_splits++;
      }

      // the block is in use
      next->free = false;
      num_mallocs++;
      
      return BLOCK_DATA(next);
   }

   /* Could not find free _block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
      num_grows++; 
   }
   
    // Update max_heap if current_heap_size exceeds max_heap
    if (current_heap_size > max_heap)
    {
        max_heap = current_heap_size;
    }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }
   
   /* Mark _block as in use */
   next->free = false;
   num_blocks++;

   /* Return data address associated with _block to the user */
   return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr) 
{
   if (ptr == NULL) 
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;

   /* TODO: Coalesce free _blocks.  If the next block or previous block 
            are free then combine them with this block being freed.
   */
   
   if (curr->next && curr->next->free) 
   {
       // combine with the next block
       // and then skip over the next block
      curr->size = curr->size + sizeof(struct _block) + curr->next->size;
      curr->next = curr->next->next;
      
      num_coalesces++;  // Increment coalesce counter
   }

   // Combine with the previous block if it's free
   // amking sure we're not looking for the previous block 
   // when we're at the first block
   if (curr != heapList)
   {
      struct _block *previous = heapList;
      while (previous && previous->next != curr) 
      {
         // Find the previous block by traversing the list
         previous = previous->next;
      }
      
      if (previous && previous->free) 
      {
         // Combine with the previous block and skip over the current block
         previous->size = previous->size + sizeof(struct _block) + curr->size;
         previous->next = curr->next;

         num_coalesces++;  // Increment coalesce counter for merging with the previous block
      }
   }
   
   num_blocks--;  // Since we've freed a block

   num_frees++;  // Increment free counter
    
   return;

}

void *calloc( size_t nmemb, size_t size )
{
   // \TODO Implement calloc
   // the total size required
    size_t T_size = nmemb * size;

    // Use malloc to allocate memory
    void *ptr = malloc(T_size);

    // If malloc fails, return NULL
    if (ptr == NULL) 
    {
        return NULL;
    }

    // Zero out the allocated memory
    memset(ptr, 0, T_size);
    
    return ptr;
}

void *realloc( void *ptr, size_t size )
{
   // \TODO Implement realloc
   // If ptr == NULL, using realloc as malloc
    if (ptr == NULL) 
    {
        return malloc(size);
    }

    // If size == 0, use realloc as free
    if (size == 0) 
    {
        free(ptr);
        return NULL;
    }

    // get the block header for the current block
    struct _block *current_block = BLOCK_HEADER(ptr);

    // if the new size is smaller, return same pointer
    if (current_block->size >= size) 
    {
        return ptr;
    }

    // allocate new memory of the requested size
    void *new_ptr = malloc(size);
    if (new_ptr == NULL) 
    {
        return NULL;
    }

    // Copy old data into the new memory
    memcpy(new_ptr, ptr, current_block->size);

    // Free the old block
    free(ptr);

    return new_ptr;
}



/* vim: IENTRTMzMjAgU3ByaW5nIDIwM002= ----------------------------------------*/
/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/