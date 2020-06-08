//
// HashTable with LinkedList header


#pragma once
#ifndef HASHTABLE_H_
#define HASHTABLE_H_

typedef struct _HashEntry {
	char *key;
	void *data;
	struct _HashEntry *next;
	struct _HashEntry *prev;
	struct _HashEntry *bucket_next;
	struct _HashEntry *bucket_prev;
} HashEntry;

typedef struct _HashTable {
	HashEntry **table;
	HashEntry *head;
	HashEntry *tail;
	int elements;
	int limit;
} HashTable;

/**
 * @brief Calculates the hash of the input key using the FNV-1a hash algorithm
 *
 * @params input Key to hash
 * @returns uint32_t of the hash
 */
unsigned int 
hash(char *input);

/**
 * @brief Initializes a hash table with the specified capacity
 *
 * @params bucketCount Specified number of buckets for the hash table
 * @returns Pointer to an initialized HashTable
 */
HashTable *
ht_init(int bucketCount);

/**
 * @brief Inserts the data into the hash table with the given key
 *
 * @params table HashTable to insert into
 * @params key Key of the element to insert
 * @params data Data to insert
 */
void 
ht_insert(HashTable *table, char *key, void *data);

/**
 * @brief Searches the hash table for the element with the key
 *
 * @params table HashTable to search
 * @params key Key of the element to find
 * @returns Data element found
 */
void* 
ht_find(HashTable *table, char *key);

/**
 * @brief Removes the specified key from the hash table
 *
 * @params table HashTable to search
 * @params key Key of element to remove
 */
void 
ht_remove(HashTable *table, char *key);

/**
 * @brief Frees all elements in the hash table
 *
 * @params table HashTable to free
 */
void 
ht_free(HashTable *table);

#endif