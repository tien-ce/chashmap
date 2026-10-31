#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "chashmap.h"

/* -------------------- Internal Utilities Declaration -------------------- */

/* Define the logger macro based on whether the log flag is active */
#ifdef CHASHMAP_LOG_ENABLED
    /* The macro accepts a format string and any number of variables */
    #define CHASHMAP_LOG(format, ...) printf(format, ##__VA_ARGS__)
#else
    /* If disabled, the macro evaluates to nothing */
    #define CHASHMAP_LOG(format, ...) 
#endif

/* Define the debug macro based on whether the debug flag is active */
#ifdef CHASHMAP_DEBUG_ENABLED
    /* The macro accepts a format string and any number of variables */
    #define CHASHMAP_DEBUG(format, ...) printf(format, ##__VA_ARGS__)
#else
    /* If disabled, the macro evaluates to nothing */
    #define CHASHMAP_DEBUG(format, ...) 
#endif
/**
 * @brief DJB2 hash algorithm.
 * Generates a uniform hash code for the given string key.
 */
static size_t hash_function(const char *str, size_t modulus);
static bool chashmap_rehashing(chashmap_t *map);
/**
 * @brief Duplicate a string using the map's allocator instead of strdup.
 */
static char *chashmap_strdup(chashmap_t *map, const char *str);
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
    void *free_user_data;          /* User context forwarded to the destructor callback */
    chashmap_calloc_fn calloc;     /* Custom calloc function defined by user */
    chashmap_free_fn free;         /* Custom free function defined by user */
    chashmap_entry_t **buckets;    /* Dynamic array of pointers serving as the hash table buckets */
};

/* -------------------- Public API Implementations -------------------- */
/**
 * @brief Allocate and initialize a new Hash Map on the heap.
 */
chashmap_t *chashmap_create(size_t initial_capacity, chashmap_free_cb free_cb, void *user_data, chashmap_calloc_fn mem_calloc, chashmap_free_fn mem_free) {
    if (initial_capacity <= 0)
    {
        initial_capacity = MIN_HASH_SIZE;
    }
    chashmap_calloc_fn do_calloc = mem_calloc ? mem_calloc : calloc;
    chashmap_free_fn   do_free   = mem_free   ? mem_free   : free;
    /* Allocate wrapper struct */
    struct CHASHMAP_STRUCT *map = do_calloc(1, sizeof(struct CHASHMAP_STRUCT));
    if (map == NULL)
    {
        return NULL;
    }
    map->calloc = do_calloc;
    map->free   = do_free;
    /* Allocate the buckets array. Use calloc to implicitly initialize all pointers to NULL. */
    map->buckets = map->calloc(initial_capacity, sizeof(chashmap_entry_t*));
    if (map->buckets == NULL)
    {
        map->free(map);
        return NULL;
    }

    /* Write initialized metadata */
    map->size = 0;
    map->capacity = initial_capacity;
    map->free_func = free_cb;
    map->free_user_data = user_data;
    return map;
}

/**
 * @brief Insert or update a key-value pair in the hash map
 * If the function return false, the caller reclaim the own of value.
 */
bool chashmap_set(chashmap_t *map, const char *key, void *value) {
    /* Check safety */
    if (map == NULL || key == NULL)
    {
        CHASHMAP_LOG("Invalid parameter, map table and key can't be NULL\n");
        return false;
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
            return false;
        }
    }
    /* calculate the hash code and apply modulo to get the bucket index */
    size_t bucket_index = hash_function(key, map->capacity);
    CHASHMAP_DEBUG("[SET] Key '%s' hashed to bucket %zu\n", key, bucket_index);
    
    /* get the bucket (first entry in chain) in hash table corresponding to key */
    chashmap_entry_t *bucket = map->buckets[bucket_index];
    
    /* look through the list in collision chain to find the target entry */
    while(bucket != NULL)
    {
        /* check if the entry matches the key */
        if(strcmp(key, bucket->key) == 0)
        {
            CHASHMAP_DEBUG("      -> Key '%s' exists, updating value.\n", key);
            /* Free old value by user destructor define */
            if (map->free_func) {
                map->free_func(bucket->value, map->free_user_data);
            }
            /* Assign new value */
            bucket->value = value;
            return true;
        }
        CHASHMAP_DEBUG("      -> Collision detected, moving to next entry in bucket %zu.\n", bucket_index);
        /* move to next entry */
        bucket = bucket->next;
    }
    
    /* Insertion logic: key not found, create new node */
    chashmap_entry_t *new_entry = map->calloc(1, sizeof(chashmap_entry_t));
    if (new_entry == NULL) {
        CHASHMAP_LOG("Can't allocate memory for new entry\n");
        return false;
    }
    new_entry->key = chashmap_strdup(map, key);
    if (new_entry->key == NULL)
    {
        CHASHMAP_LOG("Can't allocate memory for new entry key\n");
        map->free(new_entry);
        return false;
    }
    new_entry->value = value;
    
    /* Insert at head of the bucket */
    new_entry->next = map->buckets[bucket_index];
    map->buckets[bucket_index] = new_entry;
    map->size++;
    CHASHMAP_DEBUG("      -> Inserted new node '%s' at head of bucket %zu.\n", key, bucket_index);
    return true;
}

/**
 * @brief Retrieve payload data based on the key.
 */
void *chashmap_get(chashmap_t *map, const char *key) {
    if (map == NULL || key == NULL) {
        return NULL;
    }

    /* calculate the hash code and apply modulo to get the bucket index */
    size_t bucket_index = hash_function(key, map->capacity);
    CHASHMAP_DEBUG("[GET] Looking for '%s', mapped to bucket %zu\n", key, bucket_index);
    
    /* get the bucket (first entry in chain) in hash table corresponding to key */
    chashmap_entry_t *bucket = map->buckets[bucket_index];
    
    /* look through the list in collision chain to find the target entry */
    while(bucket != NULL)
    {
        /* check if the entry matches the key */
        if(strcmp(key, bucket->key) == 0)
        {
            CHASHMAP_DEBUG("      -> Found '%s' in bucket %zu!\n", key, bucket_index);
            return bucket->value;
        }
        /* move to next entry */
        bucket = bucket->next;
    }
    /* If can't find, return NULL */
    return NULL;
}

/**
 * @brief Remove an element from the hash map and free its resources.
 */
bool chashmap_remove(chashmap_t *map, const char *key) {
    /* Check safety */
    if(map == NULL || key == NULL)
    {
        CHASHMAP_LOG("Invalid arguments, map and key can't be NULL");
        return false;
    }
    size_t bucket_index = hash_function(key, map->capacity);
    chashmap_entry_t *bucket = map->buckets[bucket_index]; /* First entry of the linked list */
    if (bucket == NULL)
    {
        return false;
    }
    /* Handle head element deletion */
    if (strcmp(key, bucket->key) == 0)
    {
        /* Link the head to the next node */
        map->buckets[bucket_index] = bucket->next;
        /* Remove the node including payload, key and entry wrapper */
          if (map->free_func)
          {
              map->free_func(bucket->value, map->free_user_data); // Payload
          }
          map->free(bucket->key);
          map->free(bucket);
          bucket = NULL;
          /* Reduce the size */
          map->size --;
          return true;
    }
    chashmap_entry_t *next = bucket->next;
    while(next != NULL)
    {
        /* Find target node using the trailing pointer technique */
        if (strcmp(key, next->key) == 0)
        {
            /* Link the current node directly to the node after the target */
            bucket->next = next->next;
            /* Free the target node including payload, key, and entry wrapper */
            if (map->free_func)
            {
                map->free_func(next->value, map->free_user_data); // Payload
            }
            map->free(next->key);
            map->free(next);
            next = NULL;
            map->size --;
            return true;
        }
        /* Go to next node */
        bucket = next;
        next = next->next;
    }
    /* Can't find the key */
    return false;
}

/**
 * @brief Clean up all data and destroy the entire hash map structure.
 */
void chashmap_destroy(chashmap_t *map) {
    if (map == NULL)
    {
        return;
    }
    /* Destroy buckets */
    for (size_t i = 0; i < map->capacity; i++)
    {
        chashmap_entry_t *bucket = map->buckets[i];
        while(bucket != NULL)
        {
            /* Keep pointer to next befor free */
            chashmap_entry_t *next = bucket->next;
            /* Destroy entry node */
            if(map->free_func)
            {
                map->free_func(bucket->value, map->free_user_data);
            }
            map->free(bucket->key);
            map->free(bucket);
            bucket = next;
        }
        map->buckets[i] = NULL; /* Set null for convention despite the buckets also be free */ 
    }
    map->free(map->buckets);
    map->buckets = NULL;
    map->free(map);
    map = NULL;
    return;
}

/**
 * @brief Iterate over all key-value pairs inside the hash map.
 */
void chashmap_foreach(chashmap_t *map, chashmap_foreach_cb callback, void *user_data)
{
    /* Prevent execution if map is invalid or callback is missing */
    if (map == NULL || map->buckets == NULL || callback == NULL)
    {
        return;
    }

    /* Iterate over the entire allocated buckets array (capacity, not size) */
    for (size_t i = 0; i < map->capacity; i++)
    {
        chashmap_entry_t *bucket = map->buckets[i];
        
        /* Traverse the linked list (collision chain) at this bucket */
        while(bucket != NULL)
        {
            const char *key = bucket->key;
            void *value = bucket->value;

            /* Execute the user's callback function */
            bool is_continue = callback(key, value, user_data);
            
            /* Break out early if the callback returns false */
            if (!is_continue)
            {
                return;
            }
            
            /* Go to the next entry */
            bucket = bucket->next;
        }
    }
}
/* -------------------- Internal ultilities definition -------------------- */

static size_t hash_function(const char *str, size_t modulus) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash % modulus;
}

static bool chashmap_rehashing(chashmap_t *map)
{
    /* Try to allocate more memory for buckets with new capacity */
    size_t new_capacity = map->capacity * 2;
    CHASHMAP_DEBUG("\n[REHASH] Triggered! Expanding capacity from %zu to %zu\n", map->capacity, new_capacity);
    
    chashmap_entry_t **new_buckets = map->calloc(new_capacity, sizeof(struct CHASHMAP_ENTRY*));
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
            CHASHMAP_DEBUG("         Moving '%s' from bucket %zu to %zu\n", bucket->key, i, new_index);

            /* Add the current node into the head of new array */ 
            bucket->next = new_buckets[new_index];
            new_buckets[new_index] = bucket;
            /* Go to next entry node */
            bucket = next_node;
        }
    }
    /* Free the old bucket lists */
    map->free(map->buckets);
    /* Assign new buckets and new cap */
    map->buckets = new_buckets;
    map->capacity = new_capacity;
    return true;
}

static char *chashmap_strdup(chashmap_t *map, const char *str)
{
    /* Include the null terminator */
    size_t len = strlen(str) + 1;
    char *copy = map->calloc(len, sizeof(char));
    if (copy == NULL)
    {
        return NULL;
    }
    memcpy(copy, str, len);
    return copy;
}
