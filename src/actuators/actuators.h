// actuators.h
// Includes control for LEDs, servomotors, stepper motor, and buzzer

#ifndef ACTUATORS_H
#define ACTUATORS_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <stdio.h>

// Functions for LEDs and LED bar control
void init_leds();
void init_led_bar();
void blink_led_bar(int times, int interval_ms); // Blinks the LED bar a specified number of times
void update_led_bar(int pressure);              // Updates the LED bar based on coffee strength

// Functions for servomotor control
void servo_init(void);        // Initializes PWM for servomotors
void servo1_move(uint angle); // Moves servo 1 to the specified angle (0 to 180 degrees)
void servo2_move(uint angle); // Moves servo 2 to the specified angle (0 to 180 degrees)
void servo1_motion(void);     // Simulates the movement cycle to release coffee beans
void servo2_motion(void);     // Simulates the movement cycle to release ground coffee

// Functions for stepper motor control
void stepper_init(void); // Initializes stepper motor pins
void stepper_rotate(bool direction, uint32_t duration_ms, uint32_t step_delay_ms); 
// Rotates the motor continuously for a specified time (in ms) in the given direction

// Functions for buzzer control
void setup_pwm(uint pin, uint freq, float duty_cycle); // Sets up PWM for the specified pin with frequency and duty cycle
void stop_pwm(uint pin);                               // Stops PWM on the specified pin
void play_tone(uint pin, uint freq, uint duration_ms, float duty_cycle); 
// Plays a tone on the specified pin for a given duration in milliseconds
void play_beep_pattern(uint pin, uint freq, uint duration_ms, uint pause_ms, int repetitions, float duty_cycle); 
// Plays a beep pattern with frequency, duration, pause, and repetitions
void play_error_tone(uint pin);       // Plays an error tone
void play_success_tone(uint pin);     // Plays a success tone (ascending frequencies)
void play_coffee_ready(uint pin);     // Plays a sound to indicate that the coffee is ready

#endif // ACTUATORS_H