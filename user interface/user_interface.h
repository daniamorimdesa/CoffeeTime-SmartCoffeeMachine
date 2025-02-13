// user_interface.h
// Header file for menu display, screens, and user interaction

#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <stdint.h>  // For standard data types (uint8_t)

// Interface Control Functions
void display_initial_screen();         // Displays the initial screen with system status (water, beans, greeting)
void ask_number_of_cups();             // Asks the user how many cups they want to prepare
void ask_when_to_prepare();            // Asks if the preparation should be immediate or scheduled

// Monitoring Functions
void display_temperature_humidity();   // Displays temperature and humidity from the DHT22 sensor
void display_clock();                  // Displays the current time read from the RTC

// Callback function to process IR remote control commands
void ir_callback(uint16_t address, uint16_t command, int type);

#endif // USER_INTERFACE_H
