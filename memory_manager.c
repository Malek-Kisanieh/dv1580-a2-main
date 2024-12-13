#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

typedef struct MemBlock {
    size_t block_size;           
    int is_available;            
    struct MemBlock* next_block; 
    void* data_ptr;              
} MemBlock;

void* pool_start = NULL;       
MemBlock* pool_head = NULL;    
size_t total_pool_size = 0;    

pthread_mutex_t memory_lock = PTHREAD_MUTEX_INITIALIZER; // Global mutex for thread safety

void mem_init(size_t pool_size) {
    pthread_mutex_lock(&memory_lock);

    pool_start = malloc(pool_size);
    if (!pool_start) {
        perror("Failed to allocate memory pool");
        pthread_mutex_unlock(&memory_lock);
        exit(EXIT_FAILURE);
    }

    total_pool_size = pool_size;

    pool_head = (MemBlock*)malloc(sizeof(MemBlock));
    if (!pool_head) {
        perror("Failed to allocate block metadata");
        free(pool_start);
        pthread_mutex_unlock(&memory_lock);
        exit(EXIT_FAILURE);
    }

    pool_head->block_size = pool_size;
    pool_head->is_available = 1;
    pool_head->data_ptr = pool_start;
    pool_head->next_block = NULL;

    pthread_mutex_unlock(&memory_lock);
}

void* mem_alloc(size_t size) {
    pthread_mutex_lock(&memory_lock);

    MemBlock* current = pool_head;

    while (current != NULL) {
        if (current->is_available && current->block_size >= size) {
            if (current->block_size > size) {
                MemBlock* new_block = (MemBlock*)malloc(sizeof(MemBlock));
                if (!new_block) {
                    perror("Failed to create new block metadata");
                    pthread_mutex_unlock(&memory_lock);
                    return NULL;
                }

                new_block->block_size = current->block_size - size;
                new_block->is_available = 1;
                new_block->data_ptr = (char*)current->data_ptr + size;
                new_block->next_block = current->next_block;

                current->block_size = size;
                current->is_available = 0;
                current->next_block = new_block;
            } else {
                current->is_available = 0;
            }

            pthread_mutex_unlock(&memory_lock);
            return current->data_ptr;
        }
        current = current->next_block;
    }

    pthread_mutex_unlock(&memory_lock);
    return NULL;
}

void mem_free(void* ptr) {
    if (!ptr) {
        fprintf(stderr, "Warning: Attempted to free a NULL pointer.\n");
        return;
    }

    pthread_mutex_lock(&memory_lock);

    MemBlock* current = pool_head;
    while (current != NULL) {
        if (current->data_ptr == ptr) {
            if (current->is_available) {
                fprintf(stderr, "Warning: Block at %p is already free.\n", ptr);
                pthread_mutex_unlock(&memory_lock);
                return;
            }

            current->is_available = 1;

            MemBlock* next_block = current->next_block;
            while (next_block != NULL && next_block->is_available) {
                current->block_size += next_block->block_size;
                current->next_block = next_block->next_block;
                free(next_block);
                next_block = current->next_block;
            }

            pthread_mutex_unlock(&memory_lock);
            return;
        }
        current = current->next_block;
    }

    fprintf(stderr, "Warning: Pointer %p was not allocated from this pool.\n", ptr);
    pthread_mutex_unlock(&memory_lock);
}

void* mem_resize(void* ptr, size_t size) {
    if (!ptr) return mem_alloc(size);

    pthread_mutex_lock(&memory_lock);

    MemBlock* block = pool_head;
    while (block != NULL) {
        if (block->data_ptr == ptr) {
            if (block->block_size >= size) {
                pthread_mutex_unlock(&memory_lock);
                return ptr;
            } else {
                void* new_ptr = mem_alloc(size);
                if (new_ptr) {
                    memcpy(new_ptr, ptr, block->block_size);
                    mem_free(ptr);
                }
                pthread_mutex_unlock(&memory_lock);
                return new_ptr;
            }
        }
        block = block->next_block;
    }

    fprintf(stderr, "Warning: Resize failed, pointer %p not found.\n", ptr);
    pthread_mutex_unlock(&memory_lock);
    return NULL;
}

void mem_deinit() {
    pthread_mutex_lock(&memory_lock);

    free(pool_start);
    pool_start = NULL;

    MemBlock* current = pool_head;
    while (current != NULL) {
        MemBlock* next = current->next_block;
        free(current);
        current = next;
    }

    pool_head = NULL;
    total_pool_size = 0;

    pthread_mutex_unlock(&memory_lock);
}
