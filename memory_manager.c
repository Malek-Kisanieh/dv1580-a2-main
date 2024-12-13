#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

// Structure representing a memory block
typedef struct MemBlock {
    size_t block_size;
    int is_available;
    void* data_ptr;
    struct MemBlock* next_block;
} MemBlock;

// Global variables for memory pool management
static void* memory_pool = NULL;
static MemBlock* pool_head = NULL;
static size_t total_pool_size = 0;
static size_t metadata_used = 0;
static size_t max_metadata = 0;
static pthread_mutex_t mem_lock;

// Function to initialize the memory manager
void mem_init(size_t pool_size) {
    pthread_mutex_init(&mem_lock, NULL);

    memory_pool = malloc(pool_size);
    if (!memory_pool) {
        fprintf(stderr, "Error: Unable to allocate memory pool.\n");
        exit(EXIT_FAILURE);
    }

    total_pool_size = pool_size;
    max_metadata = pool_size * 0.2; // 20% of the pool size for metadata

    pool_head = malloc(sizeof(MemBlock));
    if (!pool_head) {
        fprintf(stderr, "Error: Unable to allocate initial metadata.\n");
        free(memory_pool);
        exit(EXIT_FAILURE);
    }

    pool_head->block_size = pool_size;
    pool_head->is_available = 1;
    pool_head->data_ptr = memory_pool;
    pool_head->next_block = NULL;
    metadata_used = sizeof(MemBlock);
}

void* mem_alloc(size_t size) {
    printf("Requesting %zu bytes\n", size);
    pthread_mutex_lock(&mem_lock);

    defragment_memory_pool();  // Defragment memory pool before allocation

    MemBlock* current = pool_head;
    while (current != NULL) {
        printf("Block: size=%zu, is_available=%d\n", current->block_size, current->is_available);
        if (current->is_available && current->block_size >= size) {
            if (current->block_size > size + sizeof(MemBlock)) {
                MemBlock* new_block = malloc(sizeof(MemBlock));
                if (!new_block) {
                    pthread_mutex_unlock(&mem_lock);
                    return NULL;
                }

                metadata_used += sizeof(MemBlock);
                if (metadata_used > max_metadata) {
                    free(new_block);
                    pthread_mutex_unlock(&mem_lock);
                    printf("Allocation failed for %zu bytes: metadata limit exceeded\n", size);
                    return NULL;
                }

                new_block->block_size = current->block_size - size - sizeof(MemBlock);
                new_block->is_available = 1;
                new_block->data_ptr = (void*)((char*)current->data_ptr + size);
                new_block->next_block = current->next_block;

                current->block_size = size;
                current->is_available = 0;
                current->next_block = new_block;
            } else {
                current->is_available = 0;
            }

            pthread_mutex_unlock(&mem_lock);
            printf("Allocated %zu bytes at %p\n", size, current->data_ptr);
            return current->data_ptr;
        }
        current = current->next_block;
    }

    pthread_mutex_unlock(&mem_lock);
    printf("Allocation failed for %zu bytes: no suitable block found\n", size);
    return NULL;
}

// Function to free allocated memory safely
void mem_free(void* ptr) {
    if (!ptr) return;

    pthread_mutex_lock(&mem_lock);

    MemBlock* current = pool_head;
    while (current != NULL) {
        if (current->data_ptr == ptr) {
            current->is_available = 1;

            // Coalesce with the next block if available
            while (current->next_block && current->next_block->is_available) {
                MemBlock* next = current->next_block;
                current->block_size += next->block_size + sizeof(MemBlock);
                current->next_block = next->next_block;
                metadata_used -= sizeof(MemBlock);
            }
            break;
        }
        current = current->next_block;
    }

    pthread_mutex_unlock(&mem_lock);
}

// Function to resize allocated memory safely
void* mem_resize(void* ptr, size_t size) {
    if (!ptr) return mem_alloc(size);
    if (size == 0) {
        mem_free(ptr);
        return NULL;
    }

    pthread_mutex_lock(&mem_lock);

    MemBlock* current = pool_head;
    while (current != NULL) {
        if (current->data_ptr == ptr) {
            if (current->block_size >= size) {
                pthread_mutex_unlock(&mem_lock);
                return ptr;
            } else {
                void* new_ptr = mem_alloc(size);
                if (new_ptr) {
                    memcpy(new_ptr, ptr, current->block_size);
                    mem_free(ptr);
                }
                pthread_mutex_unlock(&mem_lock);
                return new_ptr;
            }
        }
        current = current->next_block;
    }

    pthread_mutex_unlock(&mem_lock);
    return NULL;
}

// Function to clean up memory manager
void mem_deinit() {
    pthread_mutex_destroy(&mem_lock);

    MemBlock* current = pool_head;
    while (current != NULL) {
        MemBlock* next = current->next_block;
        free(current);
        current = next;
    }

    free(memory_pool);
    memory_pool = NULL;
    pool_head = NULL;
    total_pool_size = 0;
    metadata_used = 0;
    max_metadata = 0;
}
void defragment_memory_pool() {
    pthread_mutex_lock(&mem_lock);
    MemBlock* current = pool_head;
    while (current != NULL && current->next_block != NULL) {
        if (current->is_available && current->next_block->is_available) {
            MemBlock* next = current->next_block;
            current->block_size += next->block_size + sizeof(MemBlock);
            current->next_block = next->next_block;
            free(next);
            metadata_used -= sizeof(MemBlock);
        } else {
            current = current->next_block;
        }
    }
    pthread_mutex_unlock(&mem_lock);
}