// state.h

#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#include <stdbool.h>

// Coffee machine states
typedef enum {
  STATE_INITIAL_SCREEN,       // Displays the initial greeting, environment monitoring, resource levels, and current time
  STATE_SELECT_CUPS,          // Allows the user to select how many cups to prepare
  STATE_SCHEDULE_OR_NOW,      // User sets whether to prepare immediately or schedule for later
  STATE_BREWING,              // System starts the brewing routine, checking resources and extracting coffee
  STATE_SCHEDULING,           // User sets a scheduled time for brewing
  STATE_WAITING               // System waits until the current time matches the scheduled brewing time
} State;

// Function to manage states
void manage_state(void);

#endif // STATE_H