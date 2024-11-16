/*
 * Filename: list.h
 * 
 * Description: Implementation of a singly linked list.
 * 
 * Date: July 26, 2024
 */

#ifndef LIST_H  
#define LIST_H
#define HEADER_SIZE 8

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

struct nodeStruct {
    void* block;                // pointer to the address of the block in memory
    struct nodeStruct *next;    // pointer to the next node in the list
};

/*
 * Description: Allocate memory for a node of type struct nodeStruct and initialize
 *              it with the value item. Return a pointer to the new node.
*/
struct nodeStruct* List_createNode(void *block);

/*
 * Description: Insert node at the head of the list.
 */
void List_insertHead(struct nodeStruct **headRef, struct nodeStruct *node);

/*
 * Description: Insert node after the tail of the list.
 */
void List_insertTail(struct nodeStruct **headRef, struct nodeStruct *node);

/*
 * Description: Count number of nodes in the list.
 *              Return 0 if the list is empty, i.e., head == NULL
 */
int List_countNodes(struct nodeStruct *head);

/*
 * Description: Return the first node holding the block address, return NULL if none found.
 */
struct nodeStruct* List_findNode(struct nodeStruct *head, void *block);

/*
 * Description: Delete node from the list and free memory allocated to it.
 *              This function assuemes that node has been properly set to a valid node in the list. 
 *              For example, the client code may have found it by calling List_findNode(). 
 *              If the list contains only one node, the head of the list should be set to NULL.
 */
void List_deleteNode(struct nodeStruct **headRef, struct nodeStruct *node);

/*
 * Description: Sort the list in ascending ASCII order based on the block address field.
 *              Any sorting algorithm is fine.
 */
void List_sort(struct nodeStruct **headRef);

/*
 * Description: Return the min node in the list 
 */
struct nodeStruct* List_minNodePrev(struct nodeStruct **headRef); 

/**
 * Description: Returns the size of the smallest chunk in the list
 */
int List_smallest_chunk(struct nodeStruct **headRef);

/**
 * Description: Returns the size of the largest chunk in the list
 */
int List_largest_chunk(struct nodeStruct **headRef);

/*
 * Description: Swap head with minNodePrev->next. Assume node is valid
 */
void List_swapNode(struct nodeStruct **headRef, struct nodeStruct *minNodePrev);

/*
 * Description: Print the node from head to end of the list 
 */
void List_display(struct nodeStruct *head);

#endif