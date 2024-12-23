#include "defs.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

// Helper functions just used by this C file to clean up our code
// Using static means they can't get linked into other files

static int system_convert(System *);
static void system_simulate_process_time(System *);
static int system_store_resources(System *);

void system_create(System **system, const char *name, ResourceAmount consumed, ResourceAmount produced, int processing_time, EventQueue *event_queue) {
    *system = malloc(sizeof(System));
    if (!*system) {
        fprintf(stderr, "Failed to allocate memory for system.\n");
        exit(EXIT_FAILURE);
    }

    (*system)->name = malloc(strlen(name) + 1);
    if (!(*system)->name) {
        free(*system);
        fprintf(stderr, "Failed to allocate memory for system name.\n");
        exit(EXIT_FAILURE);
    }

    strcpy((*system)->name, name);
    (*system)->consumed = consumed;
    (*system)->produced = produced;
    (*system)->processing_time = processing_time;
    (*system)->event_queue = event_queue;
    (*system)->status = STANDARD;
    (*system)->amount_stored = 0;
    sem_init(&(*system)->mutex, 0, 1);  // Initialize semaphore
}

void system_destroy(System *system) {
    if (system) {
        free(system->name);
        sem_destroy(&system->mutex);
        free(system);
    }
}

void system_run(System *system) {
    Event event;
    int result_status;
    
    sem_wait(&system->mutex);
    
    if (system->amount_stored == 0) {
        // Need to convert resources (consume and process)
        result_status = system_convert(system);

        if (result_status != STATUS_OK) {
            // Report that resources were out / insufficient
            event_init(&event, system, system->consumed.resource, result_status, PRIORITY_HIGH, system->consumed.resource->amount);
            event_queue_push(system->event_queue, &event);    
            // Sleep to prevent looping too frequently and spamming with events
            usleep(SYSTEM_WAIT_TIME * 1000);          
        }
    }

    if (system->amount_stored  > 0) {
        // Attempt to store the produced resources
        result_status = system_store_resources(system);

        if (result_status != STATUS_OK) {
            event_init(&event, system, system->produced.resource, result_status, PRIORITY_LOW, system->produced.resource->amount);
            event_queue_push(system->event_queue, &event);
            // Sleep to prevent looping too frequently and spamming with events
            usleep(SYSTEM_WAIT_TIME * 1000);
        }
    }
    sem_post(&system->mutex);
}

static int system_convert(System *system) {
    int status;
    Resource *consumed_resource = system->consumed.resource;
    int amount_consumed = system->consumed.amount;
    
    sem_wait(&system->consumed.resource->mutex);

    // We can always convert without consuming anything
    if (consumed_resource == NULL) {
        status = STATUS_OK;
    } else {
        // Attempt to consume the required resources
        if (consumed_resource->amount >= amount_consumed) {
            consumed_resource->amount -= amount_consumed;
            status = STATUS_OK;
        } else {
            status = (consumed_resource->amount == 0) ? STATUS_EMPTY : STATUS_INSUFFICIENT;
        }
    }

    if (status == STATUS_OK) {
        system_simulate_process_time(system);

        if (system->produced.resource != NULL) {
            system->amount_stored += system->produced.amount;
        }
        else {
            system->amount_stored = 0;
        }
    }
	sem_post(&system->consumed.resource->mutex);
    return status;
}

static void system_simulate_process_time(System *system) {
    int adjusted_processing_time;

    // Adjust based on the current system status modifier
    switch (system->status) {
        case SLOW:
            adjusted_processing_time = system->processing_time * 2;
            break;
        case FAST:
            adjusted_processing_time = system->processing_time / 2;
            break;
        default:
            adjusted_processing_time = system->processing_time;
    }

    // Sleep for the required time
    usleep(adjusted_processing_time * 1000);
}

static int system_store_resources(System *system) {
    Resource *produced_resource = system->produced.resource;
    int available_space, amount_to_store;
    
    sem_wait(&system->produced.resource->mutex);

    // We can always proceed if there's nothing to store
    if (produced_resource == NULL || system->amount_stored == 0) {
        system->amount_stored = 0;
        return STATUS_OK;
    }

    amount_to_store = system->amount_stored;

    // Calculate available space
    available_space = produced_resource->max_capacity - produced_resource->amount;

    if (available_space >= amount_to_store) {
        // Store all produced resources
        produced_resource->amount += amount_to_store;
        system->amount_stored = 0;
    } else if (available_space > 0) {
        // Store as much as possible
        produced_resource->amount += available_space;
        system->amount_stored = amount_to_store - available_space;
    }
    
    sem_post(&system->produced.resource->mutex);

    if (system->amount_stored != 0) {
        return STATUS_CAPACITY;
    }

    return STATUS_OK;
}

void system_array_init(SystemArray *array) {
    array->systems = malloc(sizeof(System *) * 1);
    if (!array->systems) {
        fprintf(stderr, "Failed to allocate memory for system array.\n");
        exit(EXIT_FAILURE);
    }

    array->capacity = 1;
    array->size = 0;
}

void system_array_clean(SystemArray *array) {
    if (array && array->systems) {
        for (int i = 0; i < array->size; i++) {
            if (array->systems[i]) {
                system_destroy(array->systems[i]);
                array->systems[i] = NULL; // Prevent dangling pointers
            }
        }
        free(array->systems);
        array->systems = NULL;
        array->capacity = 0;
        array->size = 0;
    }
}

void system_array_add(SystemArray *array, System *system) {
    if (array->size == array->capacity) {
        int new_capacity = array->capacity * 2;
        System **new_systems = malloc(sizeof(System *) * new_capacity);
        if (!new_systems) {
            fprintf(stderr, "Failed to allocate memory while resizing system array.\n");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < array->size; i++) {
            new_systems[i] = array->systems[i];
        }

        free(array->systems);
        array->systems = new_systems;
        array->capacity = new_capacity;
    }

    array->systems[array->size++] = system;
}

void *system_thread(void *arg) {
    System *system = (System *)arg;
    while (system->status != TERMINATE) {
        system_run(system);
        usleep(SYSTEM_WAIT_TIME * 1000);
    }
    return NULL;
}
