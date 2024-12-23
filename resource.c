#include "defs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>

void resource_create(Resource **resource, const char *name, int amount, int max_capacity) {
    *resource = malloc(sizeof(Resource));
    if (!*resource) {
        fprintf(stderr, "Failed to allocate memory for resource.\n");
        exit(EXIT_FAILURE);
    }

    (*resource)->name = malloc(strlen(name) + 1);
    if (!(*resource)->name) {
        free(*resource);
        fprintf(stderr, "Failed to allocate memory for resource name.\n");
        exit(EXIT_FAILURE);
    }

    strcpy((*resource)->name, name);
    (*resource)->amount = amount;
    (*resource)->max_capacity = max_capacity;
    
    // Initialize semaphore
    sem_init(&(*resource)->mutex, 0, 1);
}
void resource_destroy(Resource *resource) {
    if (resource) {
    	sem_destroy(&resource->mutex);
        free(resource->name);
        free(resource);
    }
}

void resource_amount_init(ResourceAmount *resource_amount, Resource *resource, int amount) {
    resource_amount->resource = resource;
    resource_amount->amount = amount;
}

void resource_array_init(ResourceArray *array) {
    array->resources = malloc(sizeof(Resource *) * 1);
    if (!array->resources) {
        fprintf(stderr, "Failed to allocate memory for resource array.\n");
        exit(EXIT_FAILURE);
    }

    array->capacity = 1;
    array->size = 0;
}

void resource_array_clean(ResourceArray *array) {
    if (array && array->resources) {
        for (int i = 0; i < array->size; i++) {
            if (array->resources[i]) {
                resource_destroy(array->resources[i]);
                array->resources[i] = NULL; // Prevent dangling pointers
            }
        }
        free(array->resources);
        array->resources = NULL;
        array->capacity = 0;
        array->size = 0;
    }
}
void resource_array_add(ResourceArray *array, Resource *resource) {
    if (array->size == array->capacity) {
        // Double the capacity and allocate new memory
        int new_capacity = array->capacity * 2;
        Resource **new_resources = malloc(sizeof(Resource *) * new_capacity);
        if (!new_resources) {
            fprintf(stderr, "Failed to allocate memory while resizing resource array.\n");
            exit(EXIT_FAILURE);
        }

        // Copy old data to new array
        for (int i = 0; i < array->size; i++) {
            new_resources[i] = array->resources[i];
        }

        free(array->resources);
        array->resources = new_resources;
        array->capacity = new_capacity;
    }

    // Add the new resource
    array->resources[array->size++] = resource;
}
