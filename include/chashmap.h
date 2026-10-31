#ifndef CHASHMAP_H
#define CHASHMAP_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MIN_HASH_SIZE 16
/* -------------------- Opaque Types & Callbacks -------------------- */

/**
 * @brief Opaque pointer representing the Hash Map structure.
 * Memory details are hidden inside the implementation file to ensure encapsulation.
 */
typedef struct CHASHMAP_STRUCT chashmap_t;

/**
 * @brief Destructor callback function pointer.
 * Automatically invoked by the library on the payload parameter when an element
 * is overwritten, removed, or when the entire map is destroyed.
 */
typedef void (*chashmap_free_cb)(void *payload);

/**
 * @brief Memory allocator passed from outside.
 * Used if the caller want to use custom allocator instead of C standard.
 */
typedef void* (*chashmap_calloc_fn)(size_t nmemb, size_t size);
typedef void  (*chashmap_free_fn)(void *ptr);
/**
 * @brief Callback function type for iterating over hash map entries.
 * @param key The key string of the current entry (read-only).
 * @param value The exact opaque payload pointer stored in the hash map.
 * @param user_data Opaque pointer passed through from chashmap_foreach.
 * @return true to continue iteration, false to break/stop early.
 */
typedef bool (*chashmap_foreach_cb)(const char *key, void *value, void *user_data);
/* -------------------- Public API -------------------- */

/**
 * @brief Allocate and initialize a new Hash Map on the heap.
 * @param initial_capacity Initial capacity of the buckets array.
 * @param free_func Callback for payload deallocation (can be NULL).
 * @param mem_calloc Custom calloc function defined by user (NULL to use stdlib calloc).
 * @param mem_free Custom free function defined by user (NULL to use stdlib free).
 * @return Pointer to the initialized hash map.
 */
chashmap_t *chashmap_create(size_t initial_capacity, chashmap_free_cb free_func, chashmap_calloc_fn mem_calloc, chashmap_free_fn mem_free);

/**
 * @brief Insert or update a key-value pair, if return false, the value belong to caller.
 * The key string is automatically duplicated to ensure independent ownership.
 * @param map Pointer to the hash map.
 * @param key Identifier string.
 * @param value Void pointer carrying the payload data.
 * @return true if set successfully
 */
bool chashmap_set(chashmap_t *map, const char *key, void *value);

/**
 * @brief Retrieve payload data based on the key.
 * @param map Pointer to the hash map.
 * @param key Identifier string to look up.
 * @return Payload pointer if found, or NULL if not present.
 */
void *chashmap_get(chashmap_t *map, const char *key);

/**
 * @brief Remove an element from the hash map.
 * @param map Pointer to the hash map.
 * @param key Identifier string of the element to remove.
 * @return true if successfully removed, false if key not found.
 */
bool chashmap_remove(chashmap_t *map, const char *key);

/**
 * @brief Clean up all data and destroy the hash map structure.
 * @param map Pointer to the hash map.
 */
void chashmap_destroy(chashmap_t *map);

/**
 * @brief Iterate over all key-value pairs inside the hash map.
 * @param map Pointer to the hash map.
 * @param callback Function to call for each entry.
 * @param user_context Opaque pointer to pass external state to the callback.
 */
void chashmap_foreach(chashmap_t *map, chashmap_foreach_cb callback, void *user_context);
#ifdef __cplusplus
}
#endif

#endif /* CHASHMAP_H */
