//
// Doubly-Linked List header


#pragma once
#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

typedef struct _Node
{
	struct _Node* next;
	struct _Node* prev;
	void *data;
} Node;

typedef struct _LinkedList
{
	Node* head;
	Node* tail;
	int count;
} LinkedList;

/**
 * @brief Initializes a new instance of a linked list
 */
LinkedList* 
ll_init();

/**
 * @brief Inserts the data element into the end of the linked list
 *
 * @params list LinkedList to insert into
 * @params data Data to insert
 */
void
ll_insert(LinkedList *list, void *data);

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
ll_find(LinkedList *list, void *elem, int (*comparer)(void*,void*));

/**
 * @brief Removes the node from the linked list
 *
 * @params list LinkedList to search
 * @params node Node to remove
 */
void
ll_remove(LinkedList *list, Node *node);

/**
 * @brief Frees the linked list
 *
 * @params list LinkedList to free
 */
void
ll_free(LinkedList *list);

#endif