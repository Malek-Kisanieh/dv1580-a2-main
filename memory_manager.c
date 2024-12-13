#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
typedef struct MemBlock {
    size_t block_size;           
    int is_available;            
    struct MemBlock* next_block; 
    void* data_ptr;              
} MemBlock;

// Global variables for memory management
void* pool_start = NULL;    // Start of the memory pool
size_t total_pool_size = 0; // Total size of the memory pool
pthread_mutex_t mem_lock = PTHREAD_MUTEX_INITIALIZER;
MemBlock* pool_head = NULL;

// Initialize memory pool
void mem_init(size_t pool_size) {
    pool_start = malloc(pool_size);
    if (!pool_start) {
        perror("Failed to allocate memory pool");
        exit(EXIT_FAILURE);
    }
    total_pool_size = pool_size;

    // Embed the pool size as a header at the start
    *(size_t*)pool_start = pool_size; // Header for the entire pool
}


pthread_mutex_t mem_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread-safe operations

void* mem_alloc(size_t size) {
    pthread_mutex_lock(&mem_mutex); // Lock the critical section

    MemBlock* current = pool_head;

    // Traverse the list to find a free block of sufficient size
    while (current != NULL) {
        if (current->is_available && current->block_size >= size) {
            if (current->block_size > size) {
                // If the block is larger than needed, split it into two blocks
                MemBlock* new_block = (MemBlock*)malloc(sizeof(MemBlock));
                if (!new_block) {
                    perror("Failed to create new block metadata");
                    pthread_mutex_unlock(&mem_mutex); // Unlock before returning
                    return NULL;
                }

                // Initialize the new block with the remaining size
                new_block->block_size = current->block_size - size;
                new_block->is_available = 1; // The new block is available
                new_block->data_ptr = (char*)current->data_ptr + size; // Adjust the data pointer
                new_block->next_block = current->next_block; // Link to the next block

                // Update the current block to the requested size and mark it as occupied
                current->block_size = size;
                current->is_available = 0; // Mark as occupied
                current->next_block = new_block; // Link to the new block
            } else {
                // If the block size exactly matches the requested size, mark it as occupied
                current->is_available = 0;
            }

            pthread_mutex_unlock(&mem_mutex); // Unlock after finishing allocation
            return current->data_ptr;
        }
        current = current->next_block; // Move to the next block in the list
    }

    // If no suitable block is found, return NULL
    pthread_mutex_unlock(&mem_mutex); // Unlock before returning
    return NULL;
}
// Free allocated memory
void mem_free(void* ptr) {
    if (!ptr) return;

    pthread_mutex_lock(&mem_lock);

    uint8_t* block = (uint8_t*)ptr - sizeof(size_t); // Get the block header
    if (!(*(size_t*)block & 1)) {
        fprintf(stderr, "Warning: Double free or invalid pointer\n");
        pthread_mutex_unlock(&mem_lock);
        return;
    }

    *(size_t*)block &= ~1; // Mark the block as free (unset LSB)

    // Coalesce with next block if free
    uint8_t* next_block = block + (*(size_t*)block & ~1) + sizeof(size_t);
    if (next_block < (uint8_t*)pool_start + total_pool_size && !(*(size_t*)next_block & 1)) {
        *(size_t*)block += *(size_t*)next_block + sizeof(size_t);
    }

    pthread_mutex_unlock(&mem_lock);
}

// Resize an allocated block
void* mem_resize(void* ptr, size_t size) {
    if (!ptr) return mem_alloc(size);

    pthread_mutex_lock(&mem_lock);

    uint8_t* block = (uint8_t*)ptr - sizeof(size_t);
    size_t block_size = *(size_t*)block & ~1;

    if (block_size >= size) {
        pthread_mutex_unlock(&mem_lock);
        return ptr; // Reuse the block if it fits
    }

    pthread_mutex_unlock(&mem_lock);

    // Allocate new block and copy data
    void* new_ptr = mem_alloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block_size);
        mem_free(ptr);
    }
    return new_ptr;
}

// Deinitialize the memory pool
void mem_deinit() {
    pthread_mutex_lock(&mem_lock);

    free(pool_start);
    pool_start = NULL;
    total_pool_size = 0;

    pthread_mutex_unlock(&mem_lock);
    pthread_mutex_destroy(&mem_lock);
}
