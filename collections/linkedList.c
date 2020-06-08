//
// Doubly-Linked List implementation


#include "linkedList.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Initializes a new instance of a linked list
 */
LinkedList* 
ll_init() {
	return (LinkedList *)calloc(1, sizeof(LinkedList));
}

/**
 * @brief Inserts the data element into the end of the linked list
 *
 * @params list LinkedList to insert into
 * @params data Data to insert
 */
void
ll_insert(LinkedList *list, void *data) {
	Node *node = (Node *)calloc(1, sizeof(Node));
	node->data = data;
	if(list->head == NULL) {
		list->head = node;
		list->tail = node;
		node->next = node->prev = NULL;
	} 
	else {
		list->tail->next = node;
		node->prev = list->tail;
		list->tail = node;
	}
	list->count++;
}

/**
 * @brief Finds the element in the linked list. Accepts a comparison delegate that expects
 * 0 if two elements are equal. NULL can be specified to use a direct-value comparison
 *
 * @params list LinkedList to search
 * @params elem Data to compare
 * @params comparer Comparison delegate. Can be null for direct value Comparison
 * @returns Node element if found, NULL if not
 */
Node *
ll_find(LinkedList *list, void *elem, int (*comparer)(void*,void*)) {
	Node *curr = list->head;

	if (comparer == NULL) {
		while (curr != NULL && *(char *)curr->data != *(char *)elem) {
			curr = curr->next;
		}
	}
	else {
		while (curr != NULL && comparer(curr->data, elem) != 0) {
			curr = curr->next;
		}
	}

	return curr;
}

/**
 * @brief Removes the node from the linked list
 *
 * @params list LinkedList to search
 * @params node Node to remove
 */
void
ll_remove(LinkedList *list, Node *node) {
	if (node->next == NULL) {
		list->tail = node->prev;
		
		if(node->prev != NULL) {
			node->prev->next = NULL;
		}
	}
	else {
		node->next->prev = node->prev;
	}

	if (node->prev == NULL) {
		list->head = node->next;

		if(node->next != NULL) {
			node->next->prev = NULL;
		}
	}
	else {
		node->prev->next = node->next;
	}

	list->count--;
}

/**
 * @brief Frees the linked list
 *
 * @params list LinkedList to free
 */
void
ll_free(LinkedList *list) {
	// TODO: Possibly accept a free delegate instead? Data may not be malloc-ed memory
	Node *curr = list->head;
	while (curr != NULL) {
		Node *temp = curr->next;
		free(curr->data);
		free(curr);
		list->count--;
		curr = temp;
	}
}