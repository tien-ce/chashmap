#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chashmap.h"

/* 
 * Dummy callback simulating payload deallocation.
 * Assumes payload is a dynamically allocated string for this test.
 */
void free_payload_string(void *payload, void *user_data) {
    (void)user_data;
    if (payload) {
        printf("[Test] Freeing payload: %s\n", (char*)payload);
        free(payload);
    }
}

void *(cst_calloc) (size_t nmemb, size_t size)
{
    printf("custom alloc\n");
    return calloc(nmemb, size);
}

int main(void) {
    printf("=========================================\n");
    printf("   CHASHMAP COMPREHENSIVE TEST SCRIPT    \n");
    printf("=========================================\n");

    /* 1. Test chashmap_create */
    chashmap_t *map = chashmap_create(4, free_payload_string, NULL, cst_calloc, NULL);
    if (!map) {
        printf("Error: Failed to initialize hash map!\n");
        return 1;
    }
    printf("[Info] Hash map initialized with capacity 4.\n\n");

    /* 2. Test chashmap_set (and implicitly chashmap_rehashing) */
    printf("[Action] Inserting elements to trigger rehashing...\n");
    chashmap_set(map, "key1", strdup("Value 1"));
    chashmap_set(map, "key2", strdup("Value 2"));
    chashmap_set(map, "key3", strdup("Value 3")); /* Triggers rehash from 4 to 8 */
    chashmap_set(map, "key4", strdup("Value 4"));
    chashmap_set(map, "key5", strdup("Value 5"));
    printf("[Info] 5 Elements inserted successfully.\n\n");

    /* 3. Test chashmap_get */
    printf("[Action] Retrieving elements...\n");
    printf("key1: %s\n", (char*)chashmap_get(map, "key1"));
    printf("key3: %s\n", (char*)chashmap_get(map, "key3"));
    printf("invalid_key: %s\n", (char*)chashmap_get(map, "invalid_key"));
    printf("\n");

    /* 4. Test Update (overwriting existing key) */
    printf("[Action] Updating 'key2'...\n");
    chashmap_set(map, "key2", strdup("Updated Value 2"));
    printf("key2 (after update): %s\n\n", (char*)chashmap_get(map, "key2"));

    /* 5. Test chashmap_remove */
    printf("[Action] Removing 'key4'...\n");
    bool removed = chashmap_remove(map, "key4");
    printf("Remove 'key4' success: %s\n", removed ? "true" : "false");
    printf("key4 (after remove): %s\n", (char*)chashmap_get(map, "key4"));
    
    printf("\n[Action] Removing 'key_not_exist'...\n");
    removed = chashmap_remove(map, "key_not_exist");
    printf("Remove 'key_not_exist' success: %s\n\n", removed ? "true" : "false");

    /* 6. Test chashmap_destroy */
    printf("[Action] Destroying map and all remaining elements...\n");
    chashmap_destroy(map);
    printf("[Info] Map destroyed successfully. Run with Valgrind to ensure 0 leaks!\n");
    
    return 0;
}
