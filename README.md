# C Hashmap

This library provides a generic C hashmap implementation for other projects to use.

## Usage

```c
#include <stdio.h>
#include "hashmap.h"

// Dummy string payload destructor
void string_destructor(void *payload) {
    // Cast and free if dynamically allocated
    // free(payload);
}

int main(void) {
    // Create a map with the destructor
    hashmap_t *map = hashmap_create(string_destructor);

    // Set a value
    hashmap_set(map, "key1", "value1");

    // Get a value
    char *val = (char *)hashmap_get(map, "key1");
    if (val != NULL) {
        printf("key1: %s\n", val);
    }

    // Remove a value
    hashmap_remove(map, "key1");

    // Destroy the map
    hashmap_destroy(map);

    return 0;
}
```
