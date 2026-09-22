#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "chashmap.h"

/* -------------------- Internal ultilities decleration-------------------- */

/* Define the macro based on whether the setting above is active */
#ifdef CHASHMAP_LOG_ENABLED
    // The macro accepts a format string and any number of variables
    #define CHASHMAP_LOG(format, ...) printf(format, ##__VA_ARGS__)
#else
    // If disabled, the macro evaluates to nothing (takes 0 bytes of flash memory)
    #define CHASHMAP_LOG(format, ...) 
#endif

/**
 * @brief DJB2 hash algorithm.
 * Generates a uniform hash code for the given string key.
 */
static size_t hash_function(const char *str, size_t modulus);
static bool chashmap_rehashing(chashmap_t *map);
/* -------------------- Internal Data Structures -------------------- */

/**
 * @brief Node structure representing a single key-value pair in the hash map.
 * Utilized as a linked list node to resolve hash collisions through separate chaining.
 */
typedef struct CHASHMAP_ENTRY{
    char *key;                      /* Dynamically allocated string holding the unique identifier */
    void *value;                    /* Opaque pointer holding the payload data */
    struct CHASHMAP_ENTRY *next;  /* Pointer to the next node in the bucket's collision chain */
} chashmap_entry_t;

/**
 * @brief Core structure defining the state and storage of the hash map.
 * Provides the backing dynamic array and memory management metadata.
 */
struct CHASHMAP_STRUCT {
    size_t size;                   /* Current count of active entries residing in the map */
    size_t capacity;               /* Total allocated length of the buckets array for modulo arithmetic */
    chashmap_free_cb free_func;    /* Destructor callback invoked for payload cleanup */
    chashmap_entry_t **buckets;    /* Dynamic array of pointers serving as the hash table buckets */
};

/* -------------------- Public API Implementations (Stubs) -------------------- */
/**
 * @brief Allocate and initialize a new Hash Map on the heap. */

chashmap_t *chashmap_create(size_t initial_capacity, chashmap_free_cb free_func) {
    if (initial_capacity <= 0)
    {
        initial_capacity = MIN_HASH_SIZE;
    }
    /* Allocate wrapper struct */
    struct CHASHMAP_STRUCT *map = calloc(1,sizeof(struct CHASHMAP_STRUCT));
    if (map == NULL)
    {
        return NULL;
    }

    /* Allocate the buckets, each bucket is one pointer to the entry that contains user pointer */
    map->buckets = calloc(initial_capacity, sizeof(chashmap_entry_t*)); // Use calloc to implicitly point to NULL each bucket
    if (map->buckets == NULL)
    {
        free(map);
        return NULL;
    }

    /* Write initialized metadata */
    map->size = 0;
    map->capacity = initial_capacity;
    map->free_func = free_func;
    return map;
}

void chashmap_set(chashmap_t *map, const char *key, void *value) {
    /* Check safety */
    if (map == NULL || key == NULL)
    {
        CHASHMAP_LOG("Invalid parameter, map table and key can't be NULL\n");
        return;
    }
    /* Check load factor */
    if (map->size * 4 >= map->capacity * 3)
    {
        /* Rehashing table */
        bool ret = chashmap_rehashing(map);
        if (!ret)
        {
            /* If can't rehashing, abort the request */
            CHASHMAP_LOG("Can't rehashing table, abort setting value\n");
            return;
        }
    }
    /* Insert logic */
    /* calculate the hash code and apply modulo to get the bucket index */
    size_t bucket_index = hash_function(key, map->capacity);
    
    /* get the bucket (first entry in chain) in hash table corresponding to key */
    chashmap_entry_t *bucket = map->buckets[bucket_index];
    
    /* look through the list in collision chain to find the target entry */
    while(bucket != NULL)
    {
        /* check if the entry matches the key */
        if(strcmp(key, bucket->key) == 0)
        {
            /* Free old value by user destructor define */
            if (map->free_func) {
                map->free_func(bucket->value);
            }
            /* Assign new value */
            bucket->value = value;
            return;
        }
        /* move to next entry */
        bucket = bucket->next;
    }
    
    /* Insertion logic: key not found, create new node */
    chashmap_entry_t *new_entry = calloc(1, sizeof(chashmap_entry_t));
    if (new_entry == NULL) {
        CHASHMAP_LOG("Can't allocate memory for new entry\n");
        return;
    }
    new_entry->key = strdup(key);
    new_entry->value = value;
    
    /* Insert at head of the bucket */
    new_entry->next = map->buckets[bucket_index];
    map->buckets[bucket_index] = new_entry;
    map->size++;
}

/**
 * @brief Retrieve payload data based on the key 
 */
void *chashmap_get(chashmap_t *map, const char *key) {
    if (map == NULL || key == NULL) {
        return NULL;
    }

    /* calculate the hash code and apply modulo to get the bucket index */
    size_t bucket_index = hash_function(key, map->capacity);
    
    /* get the bucket (first entry in chain) in hash table corresponding to key */
    chashmap_entry_t *bucket = map->buckets[bucket_index];
    
    /* look through the list in collision chain to find the target entry */
    while(bucket != NULL)
    {
        /* check if the entry matches the key */
        if(strcmp(key, bucket->key) == 0)
        {
            return bucket->value;
        }
        /* move to next entry */
        bucket = bucket->next;
    }
    /* If can't find, return NULL */
    return NULL;
}

bool chashmap_remove(chashmap_t *map, const char *key) {
    /* Implementation pending */
    (void)map;
    (void)key;
    return false;
}

void chashmap_destroy(chashmap_t *map) {
    /* Implementation pending */
    (void)map;
    return;
}

/* -------------------- Internal ultilities definition -------------------- */

static size_t hash_function(const char *str, size_t modulus) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash / modulus;
}

static bool chashmap_rehashing(chashmap_t *map)
{
    /* Try to allocate more memory for buckets with new capacity */
    size_t new_capacity = map->capacity * 2;
    chashmap_entry_t **new_buckets = calloc(new_capacity, sizeof(struct CHASHMAP_ENTRY*));
    if (new_buckets == NULL)
    {
        return false;
    }
    /* Reorder hashmap entries with new cap */
    for (size_t i = 0; i < map->capacity; i++)
    {
        /* Get the first entry at each pocket */
        chashmap_entry_t *bucket = map->buckets[i];
        /* look through the list in collision chain to get all entry */
        while(bucket != NULL)
        {
            /* Save next node */
            chashmap_entry_t *next_node = bucket->next;

            /* recalculate the new index of entry node */
            size_t new_index = hash_function(bucket->key, new_capacity);

            /* Add the current node into the head of new array */ 
            bucket->next = new_buckets[new_index];
            new_buckets[new_index] = bucket;
            /* Go to next entry node */
            bucket = next_node;
        }
    }
    /* Free the old bucket lists */
    free(map->buckets);
    /* Assign new buckets and new cap */
    map->buckets = new_buckets;
    map->capacity = new_capacity;
    return true; 
}
