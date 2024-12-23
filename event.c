#include "defs.h"
#include <stdlib.h>
#include <stdio.h>


void event_init(Event *event, System *system, Resource *resource, int status, int priority, int amount) {
    event->system = system;
    event->resource = resource;
    event->status = status;
    event->priority = priority;
    event->amount = amount;
}

void event_queue_init(EventQueue *queue) {
    queue->head = NULL;
    queue->size = 0;

    // Initialize mutex semaphore for the queue
    sem_init(&queue->mutex, 0, 1);
}

void event_queue_clean(EventQueue *queue) {
    EventNode *current = queue->head;
    EventNode *next;

    // Free each node in the queue
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    queue->head = NULL;  // Reset the head to NULL after cleanup
	sem_destroy(&queue->mutex);
}

void event_queue_push(EventQueue *queue, const Event *event) {
    // Lock the queue for safe modification
    sem_wait(&queue->mutex);

    // Create a new node for the event
    EventNode *new_node = (EventNode *)malloc(sizeof(EventNode));
    if (!new_node) {
        fprintf(stderr, "Failed to allocate memory for new EventNode.\n");
        sem_post(&queue->mutex);  // Unlock before exiting
        exit(EXIT_FAILURE);
    }
    new_node->event = *event;
    new_node->next = NULL;

    // Insert the new node in priority order
    if (queue->head == NULL || queue->head->event.priority < event->priority) {
        // New node has the highest priority, so it becomes the new head
        new_node->next = queue->head;
        queue->head = new_node;
    } else {
        // Find the correct position to insert the new node
        EventNode *current = queue->head;
        while (current->next != NULL && current->next->event.priority >= event->priority) {
            current = current->next;
        }
        // Insert the new node after the current node
        new_node->next = current->next;
        current->next = new_node;
    }

    queue->size++;

    // Unlock the queue after modification
    sem_post(&queue->mutex);
}

int event_queue_pop(EventQueue *queue, Event *event) {
    // Lock the queue for safe modification
    sem_wait(&queue->mutex);

    if (queue->head == NULL) {
        // No events to pop
        sem_post(&queue->mutex);
        return 0;
    }

    // Remove the head node and retrieve its event data
    EventNode *node_to_pop = queue->head;
    *event = node_to_pop->event;
    queue->head = node_to_pop->next;

    free(node_to_pop);
    queue->size--;

    // Unlock the queue after modification
    sem_post(&queue->mutex);

    return 1;
}
