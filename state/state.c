// state.c

#include "state.h"
#include "user_interface.h"
#include "internal_operations.h"
#include "ir_control.h"
#include "sensors.h"
#include "lcd_i2c.h"
#include <stdio.h>
#include <stdint.h>

// Pin definitions
// Shared pins for the LCD and RTC (their addresses are defined in their respective libraries)
#define I2C_PORT i2c0        // I2C communication for the LCD display and RTC
#define SDA_PIN 4
#define SCL_PIN 5

// Global variables
float water_ml = 1000.0;         // Initial reservoir of 1 liter
float coffee_beans_g = 250.0;    // Initial reservoir of 250g of coffee beans (each cup uses 10g)
int cups = 0;                    // Number of coffee cups
// Buffer that stores the scheduled brewing time
uint8_t day_config, month_config, hour_config, minutes_config;
bool play_pressed = false;       // Indicates if the PLAY button was pressed
bool greeting_displayed = false; // Flag to display "it's coffee time" only once
bool prepare_now = false;        // Flag to start coffee brewing immediately
bool key_pressed = false;
char key[16] = "";
ScheduledTime scheduled_time = {0, 0, 0, 0, false};
State current_state = STATE_INITIAL_SCREEN;
// Ensures no flickering between the cup selection state and scheduling state
State last_displayed_state = STATE_INITIAL_SCREEN;

// Monitors the machine's state and calls the corresponding function based on the current state
void manage_state() {
  switch (current_state)
  {
    case STATE_INITIAL_SCREEN:
      if (!greeting_displayed) {
        display_initial_screen();
        greeting_displayed = true;
        last_displayed_state = STATE_INITIAL_SCREEN;  // Ensures this state was displayed
      } else {
        display_clock();                              // Continuously updates the clock
        display_temperature_humidity();               // Updates ambient conditions
      }

      if (play_pressed) {
        current_state = STATE_SELECT_CUPS;
        play_pressed = false; // Resets the flag
        last_displayed_state = STATE_INITIAL_SCREEN; // Forces an update in the next state
      }
      break;

    case STATE_SELECT_CUPS:
      if (last_displayed_state != STATE_SELECT_CUPS) {
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("HOW MANY CUPS?");
        lcd_set_cursor(2, 0);
        lcd_print("- FROM 1 TO 5");
        lcd_set_cursor(3, 0);
        lcd_print("- 0 TO EXIT");
        last_displayed_state = STATE_SELECT_CUPS; // Updates the displayed state
      }
      break;

    case STATE_SCHEDULE_OR_NOW:
      if (last_displayed_state != STATE_SCHEDULE_OR_NOW) {
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("START TIME:");
        lcd_set_cursor(2, 0);
        lcd_print("1-NOW");
        lcd_set_cursor(3, 0);
        lcd_print("2-SCHEDULE");
        last_displayed_state = STATE_SCHEDULE_OR_NOW; // Updates the displayed state
      }
      break;

    case STATE_BREWING:
      prepare_coffee(cups);
      break;

    case STATE_SCHEDULING: // State for scheduling coffee preparation
      scheduled_time = configure_schedule(I2C_PORT, SDA_PIN, SCL_PIN, key);
      if (scheduled_time.valid_time) {
        current_state = STATE_WAITING;
      } else {
        current_state = STATE_INITIAL_SCREEN;
      }
      break;

    case STATE_WAITING: { // State that compares the current time with the scheduled time to start brewing
        uint8_t rtc_data[7];
        rtc_read(I2C_PORT, SDA_PIN, SCL_PIN, rtc_data); // Reads the current time

        uint8_t current_day = (rtc_data[4] & 0x0F) + ((rtc_data[4] >> 4) * 10);
        uint8_t current_month = (rtc_data[5] & 0x0F) + ((rtc_data[5] >> 4) * 10);
        uint8_t current_hour = (rtc_data[2] & 0x0F) + ((rtc_data[2] >> 4) * 10);
        uint8_t current_minute = (rtc_data[1] & 0x0F) + ((rtc_data[1] >> 4) * 10);

        // If the current time matches the scheduled time, start brewing
        if (current_day == scheduled_time.day &&
            current_month == scheduled_time.month &&
            current_hour == scheduled_time.hour &&
            current_minute == scheduled_time.minutes) {
          current_state = STATE_BREWING;
        }
        break;
      }

    default:
      current_state = STATE_INITIAL_SCREEN;
      break;
  }
}