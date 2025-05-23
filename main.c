#include "defs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

void load_data(Manager *manager);
void *manager_thread(void *arg);
void *system_thread(void *arg);

int main(void) {
    Manager manager;
    pthread_t manager_tid;
    pthread_t *system_tids;
    
    manager_init(&manager);
    load_data(&manager);
    
    system_tids = malloc(manager.system_array.size * sizeof(pthread_t));

    pthread_create(&manager_tid, NULL, manager_thread, &manager);

    for (int i = 0; i < manager.system_array.size; i++) {
        pthread_create(&system_tids[i], NULL, system_thread, manager.system_array.systems[i]);
    }

    pthread_join(manager_tid, NULL);

    for (int i = 0; i < manager.system_array.size; i++) {
        pthread_join(system_tids[i], NULL);
    }

    free(system_tids);
    manager_clean(&manager);

    return 0;
}

void load_data(Manager *manager) {
    // Create resources
    Resource *fuel, *oxygen, *energy, *distance;
    resource_create(&fuel, "Fuel", 1000, 1000);
    resource_create(&oxygen, "Oxygen", 20, 50);
    resource_create(&energy, "Energy", 30, 50);
    resource_create(&distance, "Distance", 0, 5000);

    resource_array_add(&manager->resource_array, fuel);
    resource_array_add(&manager->resource_array, oxygen);
    resource_array_add(&manager->resource_array, energy);
    resource_array_add(&manager->resource_array, distance);

    // Create systems
    System *propulsion_system, *life_support_system, *crew_capsule_system, *generator_system;
    ResourceAmount consume_fuel, produce_distance;
    resource_amount_init(&consume_fuel, fuel, 5);
    resource_amount_init(&produce_distance, distance, 25);
    system_create(&propulsion_system, "Propulsion", consume_fuel, produce_distance, 50, &manager->event_queue);

    ResourceAmount consume_energy, produce_oxygen;
    resource_amount_init(&consume_energy, energy, 7);
    resource_amount_init(&produce_oxygen, oxygen, 4);
    system_create(&life_support_system, "Life Support", consume_energy, produce_oxygen, 10, &manager->event_queue);

    ResourceAmount consume_oxygen, produce_nothing;
    resource_amount_init(&consume_oxygen, oxygen, 1);
    resource_amount_init(&produce_nothing, NULL, 0);
    system_create(&crew_capsule_system, "Crew", consume_oxygen, produce_nothing, 2, &manager->event_queue);

    ResourceAmount consume_fuel_for_energy, produce_energy;
    resource_amount_init(&consume_fuel_for_energy, fuel, 5);
    resource_amount_init(&produce_energy, energy, 10);
    system_create(&generator_system, "Generator", consume_fuel_for_energy, produce_energy, 20, &manager->event_queue);

    system_array_add(&manager->system_array, propulsion_system);
    system_array_add(&manager->system_array, life_support_system);
    system_array_add(&manager->system_array, crew_capsule_system);
    system_array_add(&manager->system_array, generator_system);
}
