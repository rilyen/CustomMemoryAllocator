/*
 * Filename: list.c
 * 
 * Description: Implementation of a singly linked list.
 * 
 * Date: July 26, 2024
 */

#include "list.h"

/*
 * Description: Allocate memory for a node of type struct nodeStruct and initialize it with the block address. 
 *              Return a pointer to the new node.
*/
struct nodeStruct* List_createNode(void *block) {
    struct nodeStruct *newNode;                                         // declare the new node
    newNode = (struct nodeStruct*)malloc(sizeof(struct nodeStruct));    // allocate memory for the new node
    if (newNode == NULL) {
        printf("Error: List_createNode malloc failed");
        exit(1);
    }
    newNode->block = block;                                             // set block address
    newNode->next = NULL;
    return newNode;
}

/*
 * Description: Insert node at the head of the list.
 */
void List_insertHead(struct nodeStruct **headRef, struct nodeStruct *node) {
    if (node == NULL) {
        printf("Error: cannot insert NULL node\n");
        return;
    } else if(*headRef == NULL) {
        *headRef = node;
    } else if (List_findNode(*headRef,node->block)) { //node already in list
        printf("Error: cannot insert a node that is already in the list\n");
        return;
    } else {                // insert node at the head of the list
    node->next = *headRef;  // new node points to the head of the list
    *headRef = node;        // head now points to new node (new head of the list)
    }
}

/*
 * Description: Insert node after the tail of the list.
 */
void List_insertTail(struct nodeStruct **headRef, struct nodeStruct *node) {
    if (node == NULL) {     // NULL node can not be inserted
        printf("Error: cannot insert NULL node\n");
        return;
    } else if (*headRef == NULL){   // if list is empty then node becomes the head
        *headRef = node;
        return;
    } else if(List_findNode(*headRef,node->block)) {    // if node already in list can not insert
        printf("Error: cannot insert a node that is already in the list\n");
        return;
    } else {                            // else go to the end of list and append
        struct nodeStruct *curr;
        curr = *headRef;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = node;
    }
}

/*
 * Description: Count number of nodes in the list.
 *              Return 0 if the list is empty, i.e., head == NULL
 */
int List_countNodes(struct nodeStruct *head) {
    int count;
    struct nodeStruct *curr;
    count = 0;
    curr = head;
    if (head == NULL) {         // empty list return 0
        return count;
    } else {
        count++;                // non-empty list then there is at least one node
    }
    while(curr->next != NULL) { // if there is a succeeding node
        count++;                // count that node
        curr = curr->next;      // go to the next node
    }
    return count;
}

/*
 * Description: Return the first node holding the block address, return NULL if none found.
 */
struct nodeStruct* List_findNode(struct nodeStruct *head, void *block) {
    struct nodeStruct *curr = head;     // start at the head
    
    while (curr != NULL) {              // while at a node in the list        
        if (curr->block == block) { // if current node matches item return pointer to current node
            return curr;                
        }
        curr = curr->next;              // go to the next node
    }
    return NULL;                        // reached the end of the list without finding item, return NULL
}


/*
 * Description: Delete node from the list and free memory allocated to it.
 *              This function assuemes that node has been properly set to a valid node in the list. 
 *              For example, the client code may have found it by calling List_findNode(). 
 *              If the list contains only one node, the head of the list should be set to NULL.
 * 
 *              Assumes node exists in the list, so there is at least one node.
 *              Delete the first matching node found in the list (no duplicates).
 */
void List_deleteNode(struct nodeStruct **headRef, struct nodeStruct *node) {
    struct nodeStruct* curr = *headRef;
    // We guarantee that the node exists in the list, so we can delete this if there is only one node in the list
	if (List_countNodes(*headRef) == 1) {
		free(node);
		*headRef = NULL;
		return;
	}
    // the node to delete is at the head of the list
	if (curr == node) {
		*headRef = (*headRef)->next;
		free(node);
		return;
	}
    // traverse the rest of the list for the node we are looking for
	while(curr != node && curr->next != node && curr->next != NULL) {
		curr = curr->next;
	}
    // remove node from the linked list
	curr->next = node->next;
	free(node);
}

/*
 * Description: Sort the list in ascending ASCII order based on the block address field.
 *              Any sorting algorithm is fine.
 */
void List_sort(struct nodeStruct **headRef) {
    //if (*headRef == NULL || (*headRef)->next == NULL) { // if list is size 0 or 1 then already in order
    if (*headRef == NULL) {
        return;
    } else {
        // find min node
        struct nodeStruct* minNodePrev = List_minNodePrev(headRef);
        // if min node is the head then continue along the list
        if (minNodePrev == NULL) {
            List_sort(&(*headRef)->next);
        } else {
            List_swapNode(headRef,minNodePrev);
        }
        // continue along the list
        List_sort(&(*headRef)->next);
    }
}

/*
 * Description: Return the min (address) node in the list 
 */
struct nodeStruct* List_minNodePrev(struct nodeStruct **headRef) {
    struct nodeStruct *curr;
    struct nodeStruct *currPrev;
    struct nodeStruct *min;
    struct nodeStruct *minPrev;
    curr = *headRef;
    currPrev = NULL;
    min = *headRef;
    minPrev = NULL;
    while (curr != NULL) {                  // while at a node in the list        
        if (curr->block < min->block) { // if current node less than min node
            min = curr;                     // set min node
            minPrev = currPrev;
        }
        currPrev = curr;
        curr = curr->next;                  // go to the next node
    }
    return minPrev;
}

/**
 * Description: Returns the size of the smallest chunk in the list
 */
int List_smallest_chunk(struct nodeStruct **headRef) {
    struct nodeStruct *curr;
    struct nodeStruct *min;
    int min_size;
    curr = *headRef;
    min = *headRef;
    if (curr == NULL) {
        printf("The list is empty.\n");
        return 0;
    }
    min_size = *((char*)curr->block - HEADER_SIZE); // list is non-empty so set min_size to the first one we see
    curr = curr->next;
    while (curr != NULL) {                  // while at a node in the list        
        if (*((char*)curr->block - HEADER_SIZE) < *((char*)min->block - HEADER_SIZE)) { // if current node less than min node
            min = curr;                     // set min node
            min_size = *((char*)curr->block - HEADER_SIZE);
        }
        curr = curr->next;                  // go to the next node
    }
    return min_size;
}

/**
 * Description: Returns the size of the largest chunk in the list
 */
int List_largest_chunk(struct nodeStruct **headRef) {
    struct nodeStruct *curr;
    struct nodeStruct *max;
    int max_size;
    curr = *headRef;
    max = *headRef;
    if (curr == NULL) {
        printf("The list is empty.\n");
        return 0;
    }
    max_size = *((char*)curr->block - HEADER_SIZE); // list is non-empty so set max_size to the first one we see
    curr = curr->next;
    while (curr != NULL) {                  // while at a node in the list        
        if (*((char*)curr->block - HEADER_SIZE) > *((char*)max->block - HEADER_SIZE)) { // if current node less than min node
            max = curr;                     // set min node
            max_size = *((char*)curr->block - HEADER_SIZE);
        }
        curr = curr->next;                  // go to the next node
    }
    return max_size;
}

/*
 * Description: Swap head with minNodePrev->next. Assume node is valid
 */
void List_swapNode(struct nodeStruct **headRef, struct nodeStruct *minNodePrev) {
    //if (minNodePrev == NULL) {  // if head is the in node don't swap
    //    return;
    //}
    struct nodeStruct *head;
    struct nodeStruct *min;
    head = *headRef;
    min = minNodePrev->next;

    if (head == min) {  // if nodes are equal then don't need to swap
        return;
    }
    
    minNodePrev->next = head;
    struct nodeStruct *tmp;
    tmp = head->next;
    head->next = min->next;
    min->next = tmp;
    *headRef = min;
}

/*
 * Description: Print the node from head to end of the list 
 */
void List_display(struct nodeStruct *head) {
    //printf("Start of List_display\n");
    struct nodeStruct *curr;
    curr = head;
    if (curr == NULL) {
        printf("List is empty.\n");
    }
    while(curr) {
        printf("block: %p\n",curr->block);
        curr = curr->next;
    }
    return;
}