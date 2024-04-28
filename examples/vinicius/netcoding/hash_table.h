#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include "string.h"

typedef struct hash_table_t {
    int capacity;
    int size;

    size_t* keys;
    void** items;

    int (*collision_detector)(void*, void*);
    size_t (*hash_calculator)(void*);
    void (*item_copier)(void*, void**);
    void** (*vector_alocator)(int);
} hash_table;

static hash_table create_hash_table(int capacity,
                                    int (*collision_detector)(void*, void*),
                                    void** (*vector_alocator)(int),
                                    size_t (*hash_calculator)(void*),
                                    void (*item_copier)(void*, void**)) {
    hash_table map;

    map.capacity = capacity;
    map.size = 0;
    map.keys = (size_t*)malloc(capacity * sizeof(size_t));
    for(int i = 0; i < capacity; i++) map.keys[i] = 0;
    map.items = vector_alocator(capacity);
    map.hash_calculator = hash_calculator;
    map.collision_detector = collision_detector;
    map.item_copier = item_copier;
    map.vector_alocator = vector_alocator;

    return map;
}

static int insert_item_without_resize(hash_table* hash_table, void* item) {
    size_t hash = hash_table->hash_calculator(item);
    int index = hash % hash_table->capacity;

    for(int i = 0; i < hash_table->capacity; i++) {
        int next_index = (index + i) % hash_table->capacity;

        // Empty slot
        if(hash_table->keys[next_index] == 0) {
            hash_table->keys[next_index] = hash;
            hash_table->item_copier(item, &hash_table->items[next_index]);
            hash_table->size++;
            return 1;
        }
        // Verify if both items are equal
        if(hash_table->collision_detector(item, hash_table->items[next_index]))
            return 2;
    }

    return 0;
}

static void resize_hash_table(hash_table* hash_table,
                              int new_size,
                              int old_size) {
    size_t *old_keys = hash_table->keys,
           *new_keys = (size_t*)malloc(new_size * sizeof(size_t));
    for(int i = 0; i < new_size; i++) new_keys[i] = 0;
    void **old_items = hash_table->items,
         **new_items = hash_table->vector_alocator(new_size);

    hash_table->keys = new_keys;
    hash_table->items = new_items;
    hash_table->size = 0;
    hash_table->capacity = new_size;

    for(int i = 0; i < old_size; i++) {
        if(old_keys[i]) {
            insert_item_without_resize(hash_table, old_items[i]);
        }
    }

    free(old_items);
    free(old_keys);
}

/**
 * @brief Tries to insert a item into the hash table.
 *
 * @param hash_table
 * @param item
 * @return int 0 if didn't insert the item, 1 if inserted and 2 if the item was
 * already there.
 */
static int try_insert_item(hash_table* hash_table, void* item) {
    if(hash_table->size + 1 > hash_table->capacity / 2)
        resize_hash_table(
            hash_table, hash_table->capacity * 2, hash_table->capacity);

    size_t hash = hash_table->hash_calculator(item);
    int index = hash % hash_table->capacity;

    for(int i = 0; i < hash_table->capacity; i++) {
        int next_index = (index + i) % hash_table->capacity;

        // Empty slot
        if(hash_table->keys[next_index] == 0) {
            hash_table->keys[next_index] = hash;
            hash_table->item_copier(item, &hash_table->items[next_index]);
            hash_table->size++;
            return 1;
        }
        // Verify if both items are equal
        if(hash_table->collision_detector(item, hash_table->items[next_index]))
            return 2;
    }

    return 0;
}

/**
 * @brief
 *
 * @param hash_table
 * @param item
 * @return void* NULL if not found, the pointer to the data otherwise
 */
static void* find_item(hash_table* hash_table, void* item) {
    size_t hash = hash_table->hash_calculator(item);
    int index = hash % hash_table->capacity;

    for(int i = 0; i < hash_table->capacity; i++) {
        int next_index = (index + i) % hash_table->capacity;

        // Didn't find until now, then it is not here
        if(hash_table->keys[next_index] == 0) return NULL;
        if(hash_table->keys[next_index] == hash
           && hash_table->collision_detector(hash_table->items[next_index],
                                             item))
            return hash_table->items[next_index];
    }

    return NULL;
}

static void clean_hash_table(hash_table* hash_table) {
    free(hash_table->keys);
    free(hash_table->items);
}

static void print_hash_table(hash_table* hash_table) {
    printf("HashTable: {\n");
    for(int i = 0; i < hash_table->capacity; i++) {
        if(hash_table->keys[i]) {
            printf("\t[%lu] -> ", hash_table->keys[i]);
            netcoding_packet* packet =
                ((netcoding_packet**)hash_table->items)[i];
            print_packet_str(packet);
            printf("\n");
        }
        else {
            printf("\t[0] -> NULL\n");
        }
    }
    printf("}\n");
}

#endif /* HASH_TABLE_H_ */