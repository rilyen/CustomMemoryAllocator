/*
 * Filename: main.c
 * 
 * Description: Main program for a custom memory allocator.
 * 
 * Date: July 26, 2024
 */

#include <stdio.h>
#include "myalloc.h"
#include "list.h"

int main(int argc, char* argv[]) {
    //initialize_allocator(100, FIRST_FIT);
    initialize_allocator(100, BEST_FIT);
    // initialize_allocator(100, WORST_FIT);
    //printf("Using first fit algorithm on memory size 100\n");
    printf("Using best fit algorithm on memory size 100\n");

    int* p[50] = {NULL};
    for(int i=0; i<10; ++i) {
        p[i] = allocate(sizeof(int));
        if(p[i] == NULL) {
            printf("Allocation failed\n");
            continue;
        }
        *(p[i]) = i;
        printf("p[%d] = %p ; *p[%d] = %d\n", i, p[i], i, *(p[i]));
    }

    print_statistics();

    for (int i = 0; i < 10; ++i) {
        if (i % 2 == 0)
            continue;

        printf("Freeing p[%d]\n", i);
        deallocate(p[i]);
        p[i] = NULL;
    }

    printf("available_memory %d\n", available_memory());

    void* before[100] = {NULL};
    void* after[100] = {NULL};
    deallocate(p[0]);
    int count = compact_allocation(before, after);
    
    // show pointers
    for (int i=0; i < count; i++) {
        printf("_before[%d]: %p\t", i, before[i]);
        printf("_after[%d]: %p\n", i, after[i]);
    }

    print_statistics();

    // You can assume that the destroy_allocator will always be the 
    // last funciton call of main function to avoid memory leak 
    // before exit

    destroy_allocator();

    return 0;
}
