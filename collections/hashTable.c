//
// HashTable with LinkedList implementation

#include "hashTable.h"
#include <stdlib.h>
#include <string.h>

const unsigned int fnv_prime = 0x01000193;
const unsigned int fnv_seed = 0x811C9DC5;

/**
 * @brief Calculates the hash of the input key using the FNV-1a hash algorithm
 *
 * @params input Key to hash
 * @returns uint32_t of the hash
 */
unsigned int 
hash(char *input) {
	char *ptr = input;
	unsigned int hash = fnv_seed;
	while (*ptr) {
		hash = (*ptr++^fnv_seed) * fnv_prime;
	}
	return hash;
}

/**
 * @brief Initializes a hash table with the specified capacity
 *
 * @params bucketCount Specified number of buckets for the hash table
 * @returns Pointer to an initialized HashTable
 */
HashTable *
ht_init(int bucketCount) {
	HashTable *table = (HashTable *)calloc(1, sizeof(HashTable));
	table->table = (HashEntry **)calloc(bucketCount, sizeof(HashEntry *));
	table->limit = bucketCount;

	return table;
}

/**
 * @brief Inserts the data into the hash table with the given key
 *
 * @params table HashTable to insert into
 * @params key Key of the element to insert
 * @params data Data to insert
 */
void 
ht_insert(HashTable *table, char *key, void *data) {
	HashEntry *entry = (HashEntry *)calloc(1, sizeof(HashEntry));
	entry->key = key;
	entry->data = data;

	// Calculate the hash
	unsigned int hashedKey = hash(key);

	// First insert into the end of the linked list
	if (table->head == NULL) {
		table->head = table->tail = entry;
	}
	else {
		table->tail->next = entry;
		entry->prev = table->tail;
		table->tail = entry;
	}

	// Determine the position in the hash table
	unsigned int index = hashedKey%table->limit;
	// If the bucket is empty, put it as the head
	if (table->table[index] == NULL) {
		table->table[index] = entry;
	}
	else {
		// Otherwise traverse until the end
		HashEntry *bucket = table->table[index];
		while (bucket->bucket_next != NULL) {
			bucket = bucket->bucket_next;
		}
		bucket->bucket_next = entry;
	}
	// Increment the element count
	table->elements++;
}

/**
 * @brief Searches the hash table for the element with the key
 *
 * @params table HashTable to search
 * @params key Key of the element to find
 * @returns Data element found
 */
void* 
ht_find(HashTable *table, char *key) {
	if (key == NULL)
	    return NULL;
	// Calculate the hash
	unsigned int hashedKey = hash(key);

	// Determine the position in the hash table
	unsigned int index = hashedKey%table->limit;
	// If the bucket is empty, put it as the head
	if (table->table[index] == NULL) {
		return NULL;
	}
	else {
		// Otherwise traverse until found or null
		HashEntry *bucket = table->table[index];
		while (bucket != NULL && strcmp(bucket->key, key) != 0) {
			bucket = bucket->bucket_next;
		}

		if (bucket == NULL) return NULL;
		return bucket->data;
	}
}

/**
 * @brief Removes the specified key from the hash table
 *
 * @params table HashTable to search
 * @params key Key of element to remove
 */
void 
ht_remove(HashTable *table, char *key) {
	if (key == NULL)
		return;

	// Calculate hash
	unsigned int hashedKey = hash(key);
	// Get index, bounded by table limit
	unsigned int index = hashedKey % table->limit;

	// Element at index empty, does not exist
	if(table->table[index] == NULL) return;
	else {
		// Traverse the bucket
		HashEntry *bucket = table->table[index];
		
		while(bucket != NULL && strcmp(bucket->key, key) != 0) {
			bucket = bucket->bucket_next;
		}

		// Swap both the bucket pointers, and the linked list pointers
		if (bucket != NULL) {
			if (bucket->bucket_prev != NULL) {
				bucket->bucket_prev->bucket_next = bucket->next;
			}
			else {
				table->table[index] = NULL;
			}
			if (bucket->bucket_next != NULL) {
				bucket->bucket_next->bucket_prev = bucket->prev;
			}
			if (bucket->next != NULL) {
				bucket->next->prev = bucket->prev;
			}
			if (bucket->prev != NULL) {
				bucket->prev->next = bucket->next;
			}
			if (table->head == bucket) {
				table->head = bucket->next;
			}
			if (table->tail == bucket) {
				table->tail = bucket->prev;
			}
			free(bucket);
			return;
		}
	}
}

/**
 * @brief Frees all elements in the hash table
 *
 * @params table HashTable to free
 */
void 
ht_free(HashTable *table) {
	//TODO: Maybe have a free delegate since the void * data may not be pointing to malloced memory
    HashEntry *node = table->head;

	while(node != NULL) {
	    HashEntry *curr = node;
	    node = curr->next;

	    free(curr);
	}
	free(table);
}