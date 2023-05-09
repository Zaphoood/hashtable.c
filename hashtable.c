#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 31
#define MIN_CAPACITY 16

#define EMPTY 0
#define BUSY 1
#define DELETED 2

typedef struct {
  size_t key;
  void *value;
  char state;
} HashItem;

typedef struct {
  HashItem *items;
  size_t capacity;
  size_t count;
} HashTable;

size_t hash_fn(size_t key) { return key; }

size_t probe_sequence(size_t start, size_t i) {
  // i++;
  // return (start + i * i) ^ (start >> i);
  return start + i;
}

/* Find the smallest prime p for which start <= p */
size_t next_prime(size_t start) {
  size_t end;
  int is_prime;
  for (size_t i = start;; i++) {
    end = (size_t)sqrtf((float)start) + 1;
    is_prime = 1;
    for (size_t j = 2; j <= end; j++) {
      if (i % j == 0) {
        is_prime = 0;
        break;
      }
    }
    if (is_prime) {
      return i;
    }
  }
}

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
  if (hash_table->count >= hash_table->capacity) {
    *hash_table =
        hash_table_resize(hash_table, next_prime(hash_table->capacity * 2));
  }

  size_t cursor;
  size_t init_cursor = hash_fn(key);
  // Try at most hash_table->capacity times. This is a safeguard to avoid an
  // infinite loop. It may be possible to hit a free slot after
  // n > hash_table->capacity tries if using a non-naive probing sequence, but
  // we ignore that possibility
  for (size_t i = 0; i < hash_table->capacity; i++) {
    cursor = probe_sequence(init_cursor, i) % hash_table->capacity;
    if (hash_table->items[cursor].key == key &&
        hash_table->items[cursor].state == BUSY) {
      // Existing item, overwrite
      hash_table->items[cursor].value = value;
      return;
    }
    if (hash_table->items[cursor].state == EMPTY ||
        hash_table->items[cursor].state == DELETED) {
      printf("Inserted item after %zu collisions\n", i);
      hash_table->items[cursor].key = key;
      hash_table->items[cursor].value = value;
      hash_table->items[cursor].state = BUSY;
      hash_table->count++;
      return;
    }
  }
  assert(0 && "FATAL: Inserting into hash table failed");
}

HashTable hash_table_resize(const HashTable *hash_table, size_t new_capacity) {
  assert(hash_table->count <= new_capacity);
  HashTable new_hash_table = hash_table_new(new_capacity);
  printf("Resizing from capacity %zu to %zu (count is %zu)\n",
         hash_table->capacity, new_hash_table.capacity, hash_table->count);
  for (size_t i = 0; i < hash_table->capacity; i++) {
    if (hash_table->items[i].state == BUSY) {
      hash_table_insert_unbalanced(&new_hash_table, hash_table->items[i].key,
                                   hash_table->items[i].value);
    }
  }
  hash_table_free(hash_table);
  return new_hash_table;
}

void hash_table_balance_size(HashTable *hash_table) {
  if (hash_table->count > hash_table->capacity / 2) {
    *hash_table =
        hash_table_resize(hash_table, next_prime(hash_table->capacity * 2));
  } else if (hash_table->count < hash_table->capacity / 4 &&
             hash_table->capacity > MIN_CAPACITY * 2) {
    *hash_table =
        hash_table_resize(hash_table, next_prime(hash_table->capacity / 2));
  }
}

void hash_table_insert(HashTable *hash_table, size_t key, void *value) {
  hash_table_balance_size(hash_table);
  hash_table_insert_unbalanced(hash_table, key, value);
}

void hash_table_delete_unbalanced(HashTable *hash_table, size_t key) {
  size_t cursor;
  size_t init_cursor = hash_fn(key);
  for (size_t i = 0; i < hash_table->capacity; i++) {
    cursor = probe_sequence(init_cursor, i) % hash_table->capacity;
    if (hash_table->items[cursor].key == key &&
        hash_table->items[cursor].state == BUSY) {
      hash_table->items[cursor].key = 0;
      hash_table->items[cursor].value = NULL;
      hash_table->items[cursor].state = DELETED;
      hash_table->count--;
      return;
    }
    if (hash_table->items[cursor].state == EMPTY) {
      // Hit end of potential probing sequence, therefore the key doesn't exist
      return;
    }
  }
}

void hash_table_delete(HashTable *hash_table, size_t key) {
  hash_table_delete_unbalanced(hash_table, key);
  hash_table_balance_size(hash_table);
}

int hash_table_get(const HashTable *hash_table, size_t key, void **result) {
  size_t cursor;
  size_t init_cursor = hash_fn(key);
  for (size_t i = 0; i < hash_table->capacity; i++) {
    cursor = probe_sequence(init_cursor, i) % hash_table->capacity;
    if (hash_table->items[cursor].key == key &&
        hash_table->items[cursor].state == BUSY) {
      if (result != NULL) {
        *result = hash_table->items[cursor].value;
      }
      return 1;
    }
    if (hash_table->items[cursor].state == EMPTY) {
      // Hit end of potential probing sequence, therefore the key doesn't exist
      return 0;
    }
  }
  return 0;
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
    if (hash_table->items[i].state == BUSY) {
      (*items)[cursor] = hash_table->items[i];
      cursor++;
    }
  }
  return 1;
}

int hash_table_contains(const HashTable *hash_table, size_t key) {
  return hash_table_get(hash_table, key, NULL);
}

void hash_table_debug_print(const HashTable *hash_table) {
  printf("Table with %zu items:\n", hash_table->count);
  for (size_t i = 0; i < hash_table->capacity; i++) {
    if (hash_table->items[i].state == BUSY) {
      printf("\t[%zu] %zu\t%p\n", i, hash_table->items[i].key,
             hash_table->items[i].value);
    }
    if (hash_table->items[i].state == DELETED) {
      printf("\t[%zu] %zu\t%p\t(deleted)\n", i, hash_table->items[i].key,
             hash_table->items[i].value);
    }
  }
}

int main() {
  HashTable t = hash_table_new(INITIAL_CAPACITY);
  for (size_t i = 0; i < 100; i++) {
    // hash_table_insert(&t, i << 5, (void *)(i << 16 | 0xcafe));
    hash_table_insert_unbalanced(&t, i << 10, (void *)(i << 16 | 0xcafe));
  }

  hash_table_free(&t);
}
