/*
 * Filename: myalloc.c
 * 
 * Description: Contains the functionality related to the custom memory allocator.
 * 
 * Date: July 26, 2024
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "myalloc.h"
#include "list.h"

struct Myalloc {
    enum allocation_algorithm aalgorithm;
    int size;
    void* memory;
    // Some other data members you want, 
    // such as lists to record allocated/free memory
    struct nodeStruct *free_list;
    struct nodeStruct *allocated_list;
    int available_memory;
    int used_memory;
    pthread_mutex_t lock;
};

struct Myalloc myalloc;

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
void initialize_allocator(int _size, enum allocation_algorithm _aalgorithm) {
    assert(_size > 0);
    myalloc.aalgorithm = _aalgorithm;

    // Intialize the mutex
    pthread_mutex_init(&myalloc.lock, NULL);

    // Calculate the rounded size (nearest 64-byte boundary)
    int rounded_size = ((_size + 63) / 64) * 64;
    myalloc.size = rounded_size;
    int total_size = rounded_size + HEADER_SIZE;
    // Allocate the memory chunk with space for the header
    void* ptr = malloc(total_size);
    if (ptr == NULL) {
        printf("Error: initialize_allocator malloc failed");
        exit(1);
    }
    // Pre-fault the memory chunk and initialize to 0
    memset(ptr, 0, total_size);
    // Points to the memory chunk after the header
    myalloc.memory = (void*)((char*)ptr + HEADER_SIZE);

    // Initialize allocated list and free list
    myalloc.allocated_list = NULL;
    myalloc.free_list = List_createNode(myalloc.memory);
    *((size_t*)((char*)myalloc.memory - HEADER_SIZE)) = (size_t)myalloc.size;  // update header with size information

    // Store statistics information
    myalloc.available_memory = myalloc.size;
    myalloc.used_memory = 0;
}

/**
 * Description: Similar to malloc call in C.
 *              Returns a pointer to the allocated block of size _size.
 *              If allocation cannot be satisfied, returns NULL
 */
void* allocate(int _size) {
    assert(_size > 0);
    void* ptr = NULL;

    // Allocate memory from myalloc.memory 
    // ptr = address of allocated memory

    // Lock the mutex before accesing shared data structures
    pthread_mutex_lock(&myalloc.lock);

    struct nodeStruct *curr = myalloc.free_list;
    if (curr == NULL) {
        // Lock the mutex before returning
        pthread_mutex_unlock(&myalloc.lock);    
        printf("The free list is empty. No room to allocate.\n");
        return NULL;
    }
    // Find the appropriate chunk from the free list to allocate
    switch(myalloc.aalgorithm) {
        // Use the first hole that is big enough
        case FIRST_FIT: {
            while (curr) {
                int curr_free_size = abs(*((char*)curr->block - HEADER_SIZE));
                if (curr_free_size >= _size) {
                    ptr = curr->block;
                    break;
                }
                curr = curr->next;
            }
            break;
        }
        // Use the smallest hole that is big enough (must traverse the entire free list)
        case BEST_FIT: {
            while (curr) {
                int curr_free_size = abs(*((char*)curr->block - HEADER_SIZE));
                if (curr_free_size >= _size) {                                  // the current chunk is large enough
                    if (ptr == NULL) {                                          // all the previous chunks were too small
                        ptr = curr->block;
                    } else if (curr_free_size < *((char*)ptr - HEADER_SIZE)) {  // this chunk is smaller than all the previous chunks
                        ptr = curr->block;
                    }
                }
                curr = curr->next;
            }
            break;
        }
        // Use the largest hole that is big enough (must traverse the entire free list)
        case WORST_FIT: {
            while (curr) {
                int curr_free_size = abs(*((char*)curr->block - HEADER_SIZE));
                if (curr_free_size >= _size) {                                  // the current chunk is large enough
                    if (ptr == NULL) {                                          // all the previous chunks were too small
                        ptr = curr->block;
                    } else if (curr_free_size > *((char*)ptr - HEADER_SIZE)) {  // this chunk is larger than all the previous chunks
                        ptr = curr->block;
                    }
                }
                curr = curr->next;
            }
            break;
        }
        default: {
            pthread_mutex_unlock(&myalloc.lock);
            printf("BRUH ERROR\n");
            exit(1);
            break;
        }
    }

    // If we can not find sufficient space, return NULL
    if (ptr == NULL) {
        pthread_mutex_unlock(&myalloc.lock);
        printf("There are no free blocks large enough to allocate the requested size.\n");
        return NULL;
    }

    if (ptr != NULL) {  // we found a sufficient chunk
        // Update the allocated list with the chosen ptr from the free list
        List_insertHead(&myalloc.allocated_list, List_createNode(ptr));
        // Update the free list
        //  - if there is space left in the free chunk, adjust the address and update the header
        //      - if there is 8 bytes or less then do not keep it in the free list (no point in having a free chunk that can hold 0 bytes)
        //  - if the whole chunk was used, remove the node from the free list
        int ptr_free_size = abs(*((char*)ptr - HEADER_SIZE));
        int leftover_free_space = ptr_free_size - _size;
        if (leftover_free_space > 8) {
            // adjust header for the allocated chunk
                // new_size = _size;
            *((char*)ptr - HEADER_SIZE) = _size;
            // adjust address of chunk in free_list that was used and update the header
                // new_address = old_address + _size + HEADER_SIZE
                // new_size = old_size - _size - HEADER_SIZE = leftover_free_space - HEADER_SIZE
            struct nodeStruct *chunk_free = List_findNode(myalloc.free_list, ptr);
            chunk_free->block = (void*)((char*)chunk_free->block + _size + HEADER_SIZE);  // adjust address
            *((char*)chunk_free->block - HEADER_SIZE) = leftover_free_space - HEADER_SIZE;   // update header
        } else {
            // remove the entire chunk from the free list
            // header stays the same since we are allocating the whole free chunk
            List_deleteNode(&myalloc.free_list, List_findNode(myalloc.free_list, ptr));
        }
    }

    // update statistics information: available_memory and used_memory
    // new_available_memory = old_available_memory - size_ - HEADER_SIZE
    myalloc.available_memory = available_memory();
    myalloc.used_memory = used_memory();

    pthread_mutex_unlock(&myalloc.lock);
    return ptr;
}

/**
 * Description: Similar to free call in C.
 *              Takes a pointer to a chunk of memory as the sole parameter and returns it back to the allocator.
 * Precondition: The pointer is a valid entry in memory and is in the allocated list
 */
void deallocate(void* _ptr) {
    assert(_ptr != NULL);

    // Free allocated memory
    // Note: _ptr points to the user-visible memory. The size information is
    // stored at (char*)_ptr - 8.

    pthread_mutex_lock(&myalloc.lock);

    // Iterate through the free_list and merge with adjacent chunks
    // If found, we do not need to continue iterating through the list since we assume there are no adjacent chunks in the starting free list
    int _ptr_size = abs(*((char*)_ptr - HEADER_SIZE));
    struct nodeStruct *curr = myalloc.free_list;
    if (curr == NULL) {
        pthread_mutex_unlock(&myalloc.lock);
        printf("Given NULL, unable to deallocate.\n");
        return;
    }
    while (curr) {
        // found a chunk directly right to _ptr 
        //      - i.e. its address is equal to (char*)_ptr + size(_ptr) + HEADER_SIZE 
        if (curr->block == (char*)_ptr + _ptr_size + HEADER_SIZE) {
            // _ptr is the new address
            // adjust the header size information: new_size = _ptr_size + HEADER_SIZE + curr_size
            // remove curr from the free list and insert ptr at the head of the free list
            *((char*)_ptr - HEADER_SIZE) = _ptr_size + HEADER_SIZE + abs(*((char*)curr->block - HEADER_SIZE));
            List_deleteNode(&myalloc.free_list, curr);
            List_insertHead(&myalloc.free_list, List_createNode(_ptr));
            _ptr_size = abs(*((char*)_ptr - HEADER_SIZE));
            // Update statistics: myalloc.available_memory and myalloc.used_memory
            myalloc.available_memory = available_memory();
            myalloc.used_memory = used_memory();
            pthread_mutex_unlock(&myalloc.lock);
            return;
        }
        curr = curr->next;
    }
    while (curr) {
        // found a chunk directly left to _ptr
        //      - i.e. _ptr address is equal to (char*)curr->block + size(curr) + HEADER_SIZE
        int curr_size = abs(*((char*)curr->block - HEADER_SIZE));
        if (_ptr == (char*)curr->block + curr_size + HEADER_SIZE) {
            // curr->block is the new address
            // adjust the header size information: new_size = curr_size + HEADER_SIZE + _ptr_size
            // remove curr from the free list and insert curr at the head of the free list 
            //      - lecture notes indicate to always insert at list head when freeing allocated chunks)
            *((char*)curr->block - HEADER_SIZE) = curr_size + HEADER_SIZE + abs(*((char*)_ptr - HEADER_SIZE));
            List_deleteNode(&myalloc.free_list, curr);
            List_insertHead(&myalloc.free_list, List_createNode(curr->block));
            // Update statistics: myalloc.available_memory and myalloc.used_memory
            myalloc.available_memory = available_memory();
            myalloc.used_memory = used_memory();
            pthread_mutex_unlock(&myalloc.lock);
            return;
        }
        curr = curr->next;
    }

    // insert the chunk into the free list
    List_deleteNode(&myalloc.allocated_list, List_findNode(myalloc.allocated_list, _ptr));
    List_insertHead(&myalloc.free_list, List_createNode(_ptr));

    // Update statistics: myalloc.available_memory and myalloc.used_memory
    myalloc.available_memory = available_memory();
    myalloc.used_memory = used_memory();

    pthread_mutex_unlock(&myalloc.lock);
}

/**
 * Description: Returns the available memory (bytes) as an integer
 */
int available_memory() {
    int available_memory_size = 0;
    // Calculate available memory size
    struct nodeStruct *curr = myalloc.free_list;
    while (curr) {
        int curr_size = abs(*((char*)curr->block - HEADER_SIZE));
        available_memory_size += curr_size;
        curr = curr->next;
    }
    return available_memory_size;
}

/**
 * Description: Returns the used memory (bytes) as an integer
 */
int used_memory() {
    int used_memory_size = 0;
    // Calculate used (allocated) memory size
    struct nodeStruct *curr = myalloc.allocated_list;
    while (curr) {
        int curr_size = abs(*((char*)curr->block - HEADER_SIZE));
        used_memory_size += curr_size;
        curr = curr->next;
    }
    return used_memory_size;
}

/**
 * Description: Returns true if there is fragmentation. False if there is no fragmentation.
 *              Combine adjacent free chunks.
 */
bool is_fragmented() {
    struct nodeStruct* curr_free = myalloc.free_list;
    struct nodeStruct* curr_alloc = myalloc.allocated_list;
    int num_free_nodes = List_countNodes(curr_free);
    // No free space, not fragmented. Return false
    if (num_free_nodes == 0) {
        return false;
    }
    // More than one free node, is fragmented. Return true
    // Check if adjacent free chunks to be combined
    if (num_free_nodes > 1) {
        curr_free = myalloc.free_list;
        struct nodeStruct* current = NULL;
        int has_adj = 0;    // 0 = none, 1 = left, 2 = right
        struct nodeStruct* adjacent = NULL;
        while (curr_free && (has_adj==0)) {
            struct nodeStruct* curr_free_next = curr_free->next;  
            int size_curr_free = abs(*((char*)curr_free->block - HEADER_SIZE));
            while (curr_free_next) {
                // if next address is to the right of curr: next_address == current_address + size(curr) + HEADER_SIZE
                if (curr_free_next->block == (char*)curr_free->block + size_curr_free + HEADER_SIZE) {
                    current = curr_free;
                    adjacent = curr_free_next;
                    has_adj = 2;
                }
                // if next address is to the left of curr: next_address + size(next) + HEADER_SIZE == current_address
                int size_next_free = abs(*((char*)curr_free_next->block - HEADER_SIZE));
                if (curr_free->block == (char*)curr_free_next->block + size_next_free + HEADER_SIZE) {
                    current = curr_free;
                    adjacent = curr_free_next;
                    has_adj = 1;
                }
                curr_free_next = curr_free_next->next;
            }
            curr_free = curr_free->next;
        }
        // if there is an adjacent chunk to the right of the current chunk, address stays the same, update header with new size
        // remove adjacent from the free list
        if (has_adj == 2) {
            int size_adj = abs(*((char*)adjacent->block - HEADER_SIZE));
            int size_current = abs(*((char*)current->block - HEADER_SIZE));
            int new_size = size_current + HEADER_SIZE + size_adj;
            *((char*)current->block - HEADER_SIZE) = new_size;
            List_deleteNode(&myalloc.free_list, adjacent);
        }
        // if there is an adjacent chunk to the left of the current, address becomes adjacent, update header with new size
        // remove current from the free list
        if (has_adj == 1) {
            int size_adj = abs(*((char*)adjacent->block - HEADER_SIZE));
            int size_current = abs(*((char*)current->block - HEADER_SIZE));
            int new_size = size_adj + HEADER_SIZE + size_current;
            *((char*)adjacent->block - HEADER_SIZE) = new_size;
            List_deleteNode(&myalloc.free_list, current);
        }
        return true;
    }
    // If there is only one free node, we must check if it there is fragmentation
    // Fragmented if address of the free node (there is only one) is smaller than any of the addresses of the allocated nodes
    curr_free = myalloc.free_list;
    while (curr_alloc) {
        if (curr_free->block < curr_alloc->block) {
            return true;
        }
        curr_alloc = curr_alloc->next;
    }
    return false;
}


/**
 * Description: Compaction will be performed by grouping the allocated memory blocks in the beginning of the memory
 *              chunk and combining the free memory at the end of the memory chunk.
 *              The return value is an integer which is the total number of pointers inserted in the _before/_after array.
 */
int compact_allocation(void** _before, void** _after) {
    pthread_mutex_lock(&myalloc.lock);
    int compacted_size = 0;
    if (myalloc.used_memory == 0) {
        pthread_mutex_unlock(&myalloc.lock);
        return 0;
    }
    if (myalloc.available_memory == 0) {
        pthread_mutex_unlock(&myalloc.lock);
        return 0;
    }
    // compact allocated memory
    // update _before, _after and compacted_size

    struct nodeStruct *curr = myalloc.allocated_list;
    if (curr == NULL) {
        pthread_mutex_unlock(&myalloc.lock);
        return 0;
    }
    // while (curr) {
    //     _before[compacted_size] = curr->block;
    //     _after[compacted_size] = curr->block;
    //     compacted_size++;
    //     curr = curr->next;
    // }
    // Go through the free list to search for places to compact
    curr = myalloc.free_list;
    if (curr == NULL) {
        pthread_mutex_unlock(&myalloc.lock);
        return 0;
    }


    void* dest;
    while (is_fragmented()) {
        struct nodeStruct* leftmost_free = myalloc.free_list;
        // struct nodeStruct* curr_alloc = myalloc.allocated_list;
        struct nodeStruct* curr_free = myalloc.free_list;
        // find the leftmost_free node
        curr_free = curr_free->next;
        while (curr_free) {
            if (curr_free->block < leftmost_free->block) {
                leftmost_free = curr_free;
            }
            curr_free = curr_free->next;
        }
        // find the adjacent allocatedchunk (directly to the right of the leftmost_free chunk)
            // adjacent = left + size(left) + header
        //int size_leftmost = abs(*((char*)leftmost_free->block - HEADER_SIZE));
        //void* adj = (char*)leftmost_free->block + size_leftmost + HEADER_SIZE;
        
        // struct nodeStruct* adjacent = List_findNode(myalloc.allocated_list, adj);
        struct nodeStruct* adjacent = NULL;
        struct nodeStruct* alloc_adj = myalloc.allocated_list;
        if (alloc_adj) {
            if (alloc_adj->block > leftmost_free->block) {
                if (adjacent == NULL) {
                    adjacent = alloc_adj;
                } else if (alloc_adj->block < adjacent->block) {
                    adjacent = alloc_adj;
                }
            }
            alloc_adj = alloc_adj->next;
        }
        
        if (adjacent != NULL) {
            // move the adjacent chunk over to where the free chunk is via memmove
            dest = (char*)leftmost_free->block - HEADER_SIZE;
            void* src = (char*)adjacent->block - HEADER_SIZE;
            int size_dest = abs(*(char*)dest);
            int size_src = abs(*(char*)src);
            // a new free chunk will be created when src is moved to dest by the size of src (want to copy all of src)
            // we also delete the free chunk we copied into
            memmove(dest, src, size_src);
            void* end_where_new_free_starts = (char*)dest + size_src + HEADER_SIZE;
            List_deleteNode(&myalloc.free_list, leftmost_free);
            if (List_findNode(myalloc.free_list, end_where_new_free_starts) == NULL) {
                List_insertHead(&myalloc.free_list, List_createNode(end_where_new_free_starts));
                // update header of new free: how many bytes we moved src by (size of dest)
                int size_new_free = size_dest;
                *((char*)end_where_new_free_starts - HEADER_SIZE) = size_new_free;
            }
        }
    }
    // update _after array and myalloc.allocated_list
    struct nodeStruct* update_alloc = myalloc.allocated_list;
    while (update_alloc != NULL) {
        _before[compacted_size] = update_alloc->block;
        update_alloc->block = dest + HEADER_SIZE;
        _after[compacted_size] = update_alloc->block;
        if (_before[compacted_size] != _after[compacted_size]) {
            compacted_size++;
        }
        update_alloc = update_alloc->next;
    }
    myalloc.available_memory = available_memory();
    myalloc.used_memory = used_memory();

    pthread_mutex_unlock(&myalloc.lock);
    return compacted_size;
}

/**
 * Description: Prints the statistics of the memory allocator
 */
void print_statistics() {
    int allocated_size = myalloc.size;
    int allocated_chunks = List_countNodes(myalloc.allocated_list);
    int free_size = myalloc.available_memory;
    int free_chunks = List_countNodes(myalloc.free_list);
    int smallest_free_chunk_size = List_smallest_chunk(&myalloc.free_list);
    int largest_free_chunk_size = List_largest_chunk(&myalloc.free_list);

    // Calculate the statistics

    printf("Allocated size = %d\n", allocated_size);
    printf("Allocated chunks = %d\n", allocated_chunks);
    printf("Free size = %d\n", free_size);
    printf("Free chunks = %d\n", free_chunks);
    printf("Largest free chunk size = %d\n", largest_free_chunk_size);
    printf("Smallest free chunk size = %d\n", smallest_free_chunk_size);
}

/**
 * Description: Releases any dynamically allocated memory in your contiguous allocator.
 */
void destroy_allocator() {
    free((char*)myalloc.memory - HEADER_SIZE);
    pthread_mutex_destroy(&myalloc.lock);

    // free other dynamic allocated memory to avoid memory leak
    struct nodeStruct* curr1 = myalloc.allocated_list;
	struct nodeStruct* curr2 = myalloc.free_list;
	struct nodeStruct* prev1 = NULL;
	struct nodeStruct* prev2 = NULL;
	while(curr1 != NULL)
	{
		prev1 = curr1;
		curr1 = curr1->next;
		List_deleteNode(&myalloc.allocated_list, prev1);
		
	}
	while(curr2 != NULL)
	{   
		prev2 = curr2;
		curr2 = curr2->next;
		List_deleteNode(&myalloc.free_list, prev2);    
	}
}

