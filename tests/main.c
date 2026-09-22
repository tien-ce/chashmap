#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chashmap.h"

/* 
 * Dummy callback simulating payload deallocation.
 * Assumes payload is a dynamically allocated string for this test.
 */
void free_payload_string(void *payload) {
    if (payload) {
        printf("[Test] Freeing payload: %s\n", (char*)payload);
        free(payload);
    }
}

int main(void) {
    printf("=========================================\n");
    printf("   CHASHMAP TEST ENVIRONMENT STARTING    \n");
    printf("=========================================\n");

    /* 
     * Initialize hash map
     * (Will cause a linker error currently since chashmap_create is not implemented)
     */
     
    /*
    chashmap_t *map = chashmap_create(16, free_payload_string);
    if (!map) {
        printf("Error: Failed to initialize hash map!\n");
        return 1;
    }
    printf("Hash map initialized successfully.\n");
    chashmap_destroy(map);
    */

    printf("Header and Makefile configured. Implement src/chashmap.c to proceed!\n");
    return 0;
}
