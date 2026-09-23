# CHASHMAP Library Integration Guide

The `chashmap` library is a highly generic, standalone C hash map implementation. It relies on opaque pointers and `void*` payloads, allowing you to store any arbitrary data type (primitives, custom structs, function pointers) securely.

## 1. Directory Structure & Linking

To use `chashmap` in your external projects:
1. Compile the library to generate the static archive (`libchashmap.a`) using `make`.
2. Include the header file `chashmap.h` in your source files.
3. Link the static library during your compilation phase.

```bash
# Example compilation command for an external project:
gcc -Wall -Wextra main.c -I/path/to/chashmap/include -L/path/to/chashmap -lchashmap -o main_app
```

## 2. Debug & Trace Flags

The library provides internal tracing mechanisms to observe hash distribution, collisions, and pointer re-linking during rehashing. These are disabled by default to save flash/memory space, but can be enabled via compiler flags (e.g., in your `Makefile`'s `CFLAGS`).

*   `-DCHASHMAP_LOG_ENABLED`: Enables critical operation logging (e.g., out of memory, invalid arguments).
*   `-DCHASHMAP_DEBUG_ENABLED`: Enables verbose algorithmic tracing (displays exact bucket indices, linked-list traversal, and capacity expansions).

```bash
# Compiling with debug flags enabled:
gcc -Wall -Wextra -DCHASHMAP_LOG_ENABLED -DCHASHMAP_DEBUG_ENABLED -c src/chashmap.c -o obj/chashmap.o
```

## 3. Usage & Memory Lifecycle

Because the library utilizes `void*` for payloads, it assumes **no ownership** of the data unless explicitly instructed via the `chashmap_free_cb` destructor callback. 
Always define a custom destructor if your payloads involve dynamic memory (Heap allocations).

### End-to-End Example

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chashmap.h"

/* 1. Define your custom payload destructor */
void my_payload_destructor(void *payload) {
    if (payload != NULL) {
        /* Cast back to original type and free */
        free((char *)payload); 
    }
}

int main(void) {
    /* 2. Create the hashmap with an initial capacity of 16 */
    chashmap_t *map = chashmap_create(16, my_payload_destructor);
    if (!map) return 1;

    /* 3. Set values (Payloads must be pointers, e.g., strdup) */
    chashmap_set(map, "username", strdup("admin"));
    chashmap_set(map, "session", strdup("token_123xyz"));

    /* 4. Get values */
    char *user = (char *)chashmap_get(map, "username");
    if (user != NULL) {
        printf("User: %s\n", user);
    }

    /* 5. Update values (The old payload is automatically sent to my_payload_destructor) */
    chashmap_set(map, "session", strdup("token_456abc"));

    /* 6. Remove values (Payload is automatically freed) */
    chashmap_remove(map, "username");

    /* 7. Destroy the map (Frees all remaining nodes, keys, and payloads) */
    chashmap_destroy(map);

    return 0;
}
```

## 4. Algorithmic Complexity
*   **Insertion (Set)**: O(1) amortized. Degrades to O(N) if extreme collisions occur.
*   **Retrieval (Get)**: O(1) average case.
*   **Deletion (Remove)**: O(1) average case.
*   **Rehashing**: O(N), triggered automatically when the load factor reaches 75%.
