#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include "memory_manager.h"
// Define the Node structure
typedef struct Node {
    uint16_t data;
    struct Node* next;
} Node;
size_t list_count_nodes(Node* head) {
    size_t count = 0;
    while (head != NULL) {
        count++;
        head = head->next;
    }
    return count;
}
Node* list_search(Node* head, uint16_t data) {
    while (head != NULL) {
        if (head->data == data) {
            return head;
        }
        head = head->next;
    }
    return NULL; // Data not found
}
void list_insert_before(Node** head, Node* next_node, uint16_t data) {
    pthread_mutex_lock(&list_mutex);

    if (*head == NULL || next_node == NULL) {
        printf("Cannot insert before a NULL node\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    Node* new_node = (Node*)mem_alloc(sizeof(Node));
    if (!new_node) {
        printf("Memory allocation failed\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    new_node->data = data;

    if (*head == next_node) { // If the next_node is the head
        new_node->next = *head;
        *head = new_node;
    } else {
        Node* current = *head;
        while (current->next != next_node && current->next != NULL) {
            current = current->next;
        }
        if (current->next == next_node) {
            new_node->next = next_node;
            current->next = new_node;
        } else {
            printf("Next node not found in the list\n");
            mem_free(new_node);
        }
    }

    pthread_mutex_unlock(&list_mutex);
}
void list_display_range(Node* head, size_t start, size_t end) {
    pthread_mutex_lock(&list_mutex);

    size_t index = 0;
    printf("[");
    while (head != NULL && index <= end) {
        if (index >= start) {
            printf("%u", head->data);
            if (index < end && head->next != NULL) {
                printf(", ");
            }
        }
        head = head->next;
        index++;
    }
    printf("]\n");

    pthread_mutex_unlock(&list_mutex);
}

// Mutex for thread synchronization
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initializes the linked list and memory manager
void list_init(Node** head, size_t size) {
    pthread_mutex_lock(&list_mutex);
    *head = NULL;
    mem_init(size);
    pthread_mutex_unlock(&list_mutex);
}

// Inserts a new node at the end of the list
void list_insert(Node** head, uint16_t data) {
    pthread_mutex_lock(&list_mutex);

    Node* new_node = (Node*)mem_alloc(sizeof(Node));
    if (!new_node) {
        printf("Memory allocation failed\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    new_node->data = data;
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
    } else {
        Node* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }

    pthread_mutex_unlock(&list_mutex);
}

// Inserts a new node after a given node
void list_insert_after(Node* prev_node, uint16_t data) {
    if (!prev_node) {
        printf("Previous node cannot be NULL\n");
        return;
    }

    pthread_mutex_lock(&list_mutex);

    Node* new_node = (Node*)mem_alloc(sizeof(Node));
    if (!new_node) {
        printf("Memory allocation failed\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    new_node->data = data;
    new_node->next = prev_node->next;
    prev_node->next = new_node;

    pthread_mutex_unlock(&list_mutex);
}

// Deletes the first node with the specified data
void list_delete(Node** head, uint16_t data) {
    pthread_mutex_lock(&list_mutex);

    if (*head == NULL) {
        printf("List is empty\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    Node* current = *head;
    Node* previous = NULL;

    while (current != NULL && current->data != data) {
        previous = current;
        current = current->next;
    }

    if (current == NULL) {
        printf("Data not found in the list\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    if (previous == NULL) {
        *head = current->next;
    } else {
        previous->next = current->next;
    }

    mem_free(current);

    pthread_mutex_unlock(&list_mutex);
}

// Displays the elements of the list
void list_display(Node** head) {
    pthread_mutex_lock(&list_mutex);

    Node* current = *head;
    printf("[");
    while (current != NULL) {
        printf("%u", current->data);
        if (current->next != NULL) {
            printf(", ");
        }
        current = current->next;
    }
    printf("]\n");

    pthread_mutex_unlock(&list_mutex);
}

// Cleans up the list and deallocates all memory
void list_cleanup(Node** head) {
    pthread_mutex_lock(&list_mutex);

    Node* current = *head;
    while (current != NULL) {
        Node* next_node = current->next;
        mem_free(current);
        current = next_node;
    }
    *head = NULL;
    mem_deinit();

    pthread_mutex_unlock(&list_mutex);
}
