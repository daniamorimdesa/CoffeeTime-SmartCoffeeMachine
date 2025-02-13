// internal_operations.h
// Initial configuration and core operations for the coffee machine

#ifndef INTERNAL_OPERATIONS_H
#define INTERNAL_OPERATIONS_H

void setup_machine();                                       // Initializes the machine
void prepare_coffee(int cups);                              // Simulates the coffee preparation process
void simulate_water_heating(float desired_temp);            // Simulates water heating
const char* determine_coffee_strength(int pressure);        // Determines the coffee strength based on pressure
const char* determine_temperature_level(float temperature); // Determines the coffee temperature level

#endif // INTERNAL_OPERATIONS_H
