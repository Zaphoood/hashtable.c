#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16
#define MIN_CAPACITY 16

typedef struct {
  size_t key;
  void *value;
} HashItem;

typedef struct {
  HashItem *items;
  size_t capacity;
  size_t count;
} HashTable;

size_t hash_fn(size_t key) { return key; }

HashTable hash_table_new(size_t capacity) {
  HashTable hash_table;
  hash_table.items = malloc(capacity * sizeof(HashItem));
  memset(hash_table.items, 0, capacity * sizeof(HashItem));
  hash_table.capacity = capacity;
  hash_table.count = 0;

  return hash_table;
}

int mod(int a, int b) { return (a % b + b) % b; }

void hash_table_free(const HashTable *hash_table) { free(hash_table->items); }

HashTable hash_table_resize(const HashTable *hash_table, size_t new_capacity);

void hash_table_insert_unbalanced(HashTable *hash_table, size_t key,
                                  void *value) {
  assert(hash_table->count < hash_table->capacity);

  size_t cursor = hash_fn(key) % hash_table->capacity;
  size_t init_cursor = cursor;
  while (1) {
    if (hash_table->items[cursor].key == key &&
        hash_table->items[cursor].value != NULL) {
      // Existing item, overwrite
      hash_table->items[cursor].value = value;
      return;
    }
    if (hash_table->items[cursor].value == NULL) {
      // Empty slot
      // printf("Inserted item after %d collisions\n",
      //       mod(cursor - init_cursor, hash_table->capacity));
      hash_table->items[cursor].key = key;
      hash_table->items[cursor].value = value;
      hash_table->count++;
      return;
    }
    cursor = (cursor + 1) % hash_table->capacity;
    if (cursor == init_cursor) {
      assert(0 && "Unreachable");
    }
  }
}

HashTable hash_table_resize(const HashTable *hash_table, size_t new_capacity) {
  assert(hash_table->count <= new_capacity);
  HashTable new_hash_table = hash_table_new(new_capacity);
  printf("Resizing from capacity %zu to %zu (count is %zu)\n",
         hash_table->capacity, new_hash_table.capacity, hash_table->count);
  for (size_t i = 0; i < hash_table->capacity; i++) {
    if (hash_table->items[i].value != NULL) {
      hash_table_insert_unbalanced(&new_hash_table, hash_table->items[i].key,
                                   hash_table->items[i].value);
    }
  }
  hash_table_free(hash_table);
  return new_hash_table;
}

void hash_table_balance_size(HashTable *hash_table) {
  if (hash_table->count > hash_table->capacity / 2) {
    *hash_table = hash_table_resize(hash_table, hash_table->capacity * 2);
  } else if (hash_table->count < hash_table->capacity / 4 &&
             hash_table->capacity > MIN_CAPACITY * 2) {
    *hash_table = hash_table_resize(hash_table, hash_table->capacity / 2);
  }
}

void hash_table_insert(HashTable *hash_table, size_t key, void *value) {
  hash_table_balance_size(hash_table);
  hash_table_insert_unbalanced(hash_table, key, value);
}

void hash_table_delete_unbalanced(HashTable *hash_table, size_t key) {
  size_t cursor = hash_fn(key) % hash_table->capacity;
  size_t init_cursor = cursor;
  while (1) {
    if (hash_table->items[cursor].key == key &&
        hash_table->items[cursor].value != NULL) {
      hash_table->items[cursor].key = 0;
      hash_table->items[cursor].value = NULL;
      hash_table->count--;
      return;
    }
    // if (hash_table->items[cursor].value == NULL) {
    //   // Hit empty slot, therefore the key doesn't exist
    //   return;
    // }
    cursor = (cursor + 1) % hash_table->capacity;
    if (cursor == init_cursor) {
      return;
    }
  }
}

void hash_table_delete(HashTable *hash_table, size_t key) {
  hash_table_delete_unbalanced(hash_table, key);
  hash_table_balance_size(hash_table);
}

int hash_table_get(const HashTable *hash_table, size_t key, void **result) {
  size_t cursor = hash_fn(key) % hash_table->capacity;
  size_t init_cursor = cursor;
  while (1) {
    if (hash_table->items[cursor].key == key &&
        hash_table->items[cursor].value != NULL) {
      if (result != NULL) {
        *result = hash_table->items[cursor].value;
      }
      return 1;
    }
    if (hash_table->items[cursor].value == NULL) {
      // Hit empty slot, therefore the key doesn't exist
      return 0;
    }
    cursor = (cursor + 1) % hash_table->capacity;
    if (cursor == init_cursor) {
      return 0;
    }
  }
}

int hash_table_collect(const HashTable *hash_table, HashItem **items,
                       size_t *count) {
  *items = malloc(hash_table->count * sizeof(HashItem));
  if (items == NULL) {
    return 0;
  }
  *count = hash_table->count;
  int cursor = 0;
  for (size_t i = 0; i < hash_table->capacity; i++) {
    if (hash_table->items[i].value != NULL) {
      (*items)[cursor] = hash_table->items[i];
      cursor++;
    }
  }
  return 1;
}

int hash_table_contains(const HashTable *hash_table, size_t key) {
  return hash_table_get(hash_table, key, NULL);
}

void hash_table_print(const HashTable *hash_table) {
  printf("Table with %zu items:\n", hash_table->count);
  for (size_t i = 0; i < hash_table->capacity; i++) {
    if (hash_table->items[i].value == NULL) {
      continue;
    }
    printf("\t[%zu] %zu\t%p\n", i, hash_table->items[i].key,
           hash_table->items[i].value);
  }
}

int main() {
  HashTable t = hash_table_new(INITIAL_CAPACITY);
  for (size_t i = 0; i < 5; i++) {
    hash_table_insert(&t, i << 5, (void *)(i << 16 | 0xcafe));
  }
  size_t key = 3 << 5;
  // void *result;
  // if (hash_table_get(&t, key, &result)) {
  //   printf("Value at key %zu: %p\n", key, result);
  // } else {
  //   fprintf(stderr, "No key %zu found\n", key);
  // }
  // hash_table_delete(&t, key);
  // printf("Table has value %zu: %s\n", key,
  //       hash_table_contains(&t, key) ? "yes" : "no");
  HashItem *items;
  size_t count;

  if (hash_table_collect(&t, &items, &count)) {
    printf("Collected hash table items:\n");
    for (size_t i = 0; i < count; i++) {
      printf("\t[%zu] %zu\t%p\n", i, items[i].key, items[i].value);
    }
  } else {
    fprintf(stdout, "Failed to collect hash table items\n");
  }

  free(items);
  hash_table_free(&t);
}
