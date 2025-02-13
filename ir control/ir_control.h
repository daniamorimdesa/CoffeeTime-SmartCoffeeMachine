// ir_control.h

// Header for the NEC infrared protocol. Used only for receiving.
#include <string.h>
#include <stdint.h>
#include "pico/stdlib.h"


#ifndef IR_CONTROL_H
#define IR_CONTROL_H

#define NORMAL 1
#define REPEAT 2

#define ZERO_SPACE     1125
#define ONE_SPACE      2250
#define MAXIMUM_SPACE 15000
#define REPEAT_SPACE  11250

// Representation of the pulses by time in microseconds.
struct _ir_data {
  size_t cnt; // Used for keeping track of pulse length.
  uint64_t rises[34]; // Used for keeping track of timings. Has time of each pulse in microseconds.
};

extern struct _ir_data ir_data;

// Used for repeat codes
extern uint16_t __last_address;
extern uint16_t __last_command;

//The user's function.
extern void (*user_function_callback) (uint16_t address, uint16_t command, int type);

//reset the ir_data struct
void reset_ir_data();

//type Type of processing. Can be REPEAT or NORMAL.
//Function that computes the differences between rises and decodes them.
void process_ir_data(int type);

//Function called automatically by the irq. Used for triggering the processing.
void irq_callback(uint gpio, uint32_t events);
void init_ir_irq_receiver(uint gpio, void (*callback) (uint16_t address, uint16_t command, int type));

const char* get_key_name(uint16_t command);

#endif // IR_CONTROL_H