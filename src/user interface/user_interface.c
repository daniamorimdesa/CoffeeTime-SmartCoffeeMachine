// user_interface.c
// Displays menus, screens, and handles user interaction

#include "user_interface.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include <string.h>
#include "lcd_i2c.h"
#include "sensors.h"
#include "actuators.h"
#include "ir_control.h"
#include "state.h"

#define DHT_PIN 8      // DHT22 sensor for monitoring ambient temperature and humidity
#define I2C_PORT i2c0  
#define SDA_PIN 4
#define SCL_PIN 5
#define BUZZER_PIN 14  // Buzzer for sound notifications

extern float water_ml;
extern float coffee_beans_g;
extern State current_state;
extern int cups;
extern bool play_pressed;
extern bool key_pressed;
extern bool prepare_now;
extern char key[16];

// -------------------------------------------------------------------------------------------------- //
// Screen and Menu Functions

void ask_number_of_cups() {
  lcd_clear();
  lcd_set_cursor(0, 0);
  lcd_print("HOW MANY CUPS?");
  lcd_set_cursor(2, 0);
  lcd_print("- FROM 1 TO 5");
  lcd_set_cursor(3, 0);
  lcd_print("- 0 TO EXIT");
}

void ask_when_to_prepare() {
  lcd_clear();
  lcd_set_cursor(0, 0);
  lcd_print("START TIME:");
  lcd_set_cursor(2, 0);
  lcd_print("1-NOW");
  lcd_set_cursor(3, 0);
  lcd_print("2-SCHEDULE");
}

// -------------------------------------------------------------------------------------------------- //
// Initial Screen and Monitoring

// Displays the initial screen with updated B (beans = coffee beans) and W (water) values
void display_initial_screen() {
  gpio_put(7, 1); // Turns on the green LED to indicate that the machine is on
  static float last_water_ml = -1.0;
  static float last_coffee_beans_g = -1.0;

  lcd_clear();
  type_effect(" IT'S COFFEE TIME!", 0, 50);
  sleep_ms(500);

  if (water_ml != last_water_ml || coffee_beans_g != last_coffee_beans_g) {
    char status[32];
    snprintf(status, sizeof(status), "B:%.0fg|W:%.2fL", coffee_beans_g, water_ml / 1000);
    type_effect(status, 2, 100);
    last_water_ml = water_ml;
    last_coffee_beans_g = coffee_beans_g;
  }
}

// Displays updated ambient conditions on the initial screen
void display_temperature_humidity() {
  dht_reading reading;
  read_from_dht(&reading, DHT_PIN);

  lcd_set_cursor(3, 0);
  if (is_valid_reading(&reading)) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.1fC|H:%.1f%%", reading.temp_celsius, reading.humidity);
    lcd_print(buffer);
  } else {
    lcd_print("Error!");
    play_error_tone(BUZZER_PIN);
  }
  sleep_ms(300);
}

// Displays the HH:MM clock on the initial screen
void display_clock() {
  uint8_t rtc_data[7];
  char time_buffer[6];
  rtc_read(I2C_PORT, SDA_PIN, SCL_PIN, rtc_data);

  uint8_t hours = (rtc_data[2] & 0x0F) + ((rtc_data[2] >> 4) * 10);
  uint8_t minutes = (rtc_data[1] & 0x0F) + ((rtc_data[1] >> 4) * 10);

  snprintf(time_buffer, sizeof(time_buffer), "%02d:%02d", hours, minutes);
  lcd_set_cursor(3, 15);
  lcd_print(time_buffer);
  sleep_ms(300);
}

// -------------------------------------------------------------------------------------------------- //
// Callback function to process IR remote control commands
// Maps remote control buttons to specific actions, such as starting preparation, setting a time, etc.
void ir_callback(uint16_t address, uint16_t command, int type) {
  const char* key_name = get_key_name(command);

  // Checks if a valid key was pressed
  if (strlen(key_name) > 0) {
    key_pressed = true;
    strncpy(key, key_name, sizeof(key));
    key[sizeof(key) - 1] = '\0'; // Ensures null termination
  }

  if (strcmp(key_name, "PLAY") == 0) {
    play_pressed = true; // Marks that PLAY was pressed
  } else if (current_state == STATE_SELECT_CUPS) {
    if (strcmp(key_name, "0") == 0) { // If 0 is pressed, return to the start
      lcd_clear();
      display_initial_screen();
      current_state = STATE_INITIAL_SCREEN; // Returns to the initial screen
    } else if (strcmp(key_name, "1") == 0 || strcmp(key_name, "2") == 0 ||
               strcmp(key_name, "3") == 0 || strcmp(key_name, "4") == 0 || strcmp(key_name, "5") == 0) {
      cups = key_name[0] - '0'; // Converts the key into the desired number of cups
      current_state = STATE_SCHEDULE_OR_NOW;
    } else {
      lcd_clear();
      lcd_set_cursor(0, 0);
      lcd_print("INVALID KEY"); // If the user presses an invalid key
      lcd_set_cursor(2, 0);
      lcd_print("PLEASE SELECT 1 TO 5");
      sleep_ms(1000);
    }
  } else if (current_state == STATE_SCHEDULE_OR_NOW) { // User's choice to prepare now or schedule
    if (strcmp(key_name, "1") == 0) {
      prepare_now = true;
      current_state = STATE_BREWING;
    } else if (strcmp(key_name, "2") == 0) {
      prepare_now = false;
      current_state = STATE_SCHEDULING;
    }
  }
}