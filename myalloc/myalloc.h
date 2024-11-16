/*
 * Filename: myalloc.c
 * 
 * Description: Contains the functionality related to the custom memory allocator.
 * 
 * Date: July 26, 2024
 */

#ifndef __MYALLOC_H__
#define __MYALLOC_H__
#include <stdbool.h>

enum allocation_algorithm {FIRST_FIT, BEST_FIT, WORST_FIT};

/**
 * Description: Initialize the memory allocator. 
 *              _size indicates the contiguous memory chunk size that is assumed for the rest of the program:
 *                  - it should align to the nearest next 64-byte boundary (round up)
 *                  - Ex) If the requested size is 120 bytes, internally you should round it up to 128 bytes
 *                  - stats information should base on the actual chunk size
 *                  - any requests for allocation and deallocation requests will be served from this contiguous chunk
 *              You must allocate the memory chunk using malloc.
 *              The memory chunk must be pre-faulted.
 *              Its content initialized to 0 (using memset).
 */
void initialize_allocator(int _size, enum allocation_algorithm _aalgorithm);

/**
 * Description: Similar to malloc call in C.
 *              Returns a pointer to the allocated block of size _size.
 *              If allocation cannot be satisfied, returns NULL.
 */
void* allocate(int _size);

/**
 * Description: Similar to free call in C.
 *              Takes a pointer to a chunk of memory as the sole parameter and returns it back to the allocator.
 * Precondition: The pointer is a valid entry in memory and the allocated lists.
 */
void deallocate(void* _ptr);

/**
 * Description: Returns the available memory (bytes) as an integer.
 */
int available_memory();

/**
 * Description: Returns the used memory (bytes) as an integer.
 */
int used_memory();

/**
 * Description: Prints the statistics of the memory allocator.
 */
void print_statistics();

/**
 * Description: Returns true if there is fragmentation. False if there is no fragmentation.
 */
bool is_fragmented();

/**
 * Description: Compaction will be performed by grouping the allocated memory blocks in the beginning of the memory
 *              chunk and combining the free memory at the end of the memory chunk.
 */
int compact_allocation(void** _before, void** _after);

/**
 * Description: Releases any dynamically allocated memory in your contiguous allocator.
 */
void destroy_allocator();

#endif
