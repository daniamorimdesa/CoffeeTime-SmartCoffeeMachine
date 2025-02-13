// internal_operations.c
// Initial configuration and core operations for the coffee machine

#include "internal_operations.h"
#include "sensors.h"
#include "actuators.h"
#include "lcd_i2c.h"
#include "user_interface.h"
#include "state.h"
#include <stdio.h>
#include "pico/stdlib.h"

#define DHT_PIN 8      // DHT22 sensor for monitoring ambient temperature and humidity
#define BUZZER_PIN 14  // Buzzer for sound notifications
#define BLUE_LED 13    // Blue LED: indicates that the coffee preparation process is active

extern float water_ml;
extern float coffee_beans_g;
extern bool play_pressed;
extern State current_state;

void setup_machine() {
  stdio_init_all();
  init_leds();
  init_led_bar();
  init_i2c_lcd();
  servo_init();
  stepper_init();
  gpio_init(DHT_PIN);
  init_adc();
  play_success_tone(BUZZER_PIN);

  printf("COFFEE MACHINE INSTRUCTIONS\n");
  printf("=====================================================================================\n");
  printf(">> Customize your drink: strength, temperature, and water amount.\n");
  printf(">> Use the IR remote control to navigate. Press PLAY to start.\n");
  printf(">> Use the DHT22 sensor to monitor ambient temperature and humidity.\n");
  printf(">> If you schedule preparation, the machine will wait for the set time.\n");
  printf(">> During preparation, the LED bar indicates coffee strength.\n");
  printf(">> The initial screen updates the values as they change.\n");
}

// Simulates water heating to the desired temperature
void simulate_water_heating(float desired_temp) {
  float current_temp = 25.0;
  lcd_clear();
  lcd_set_cursor(1, 2);
  lcd_print("HEATING WATER...");

  while (current_temp <= desired_temp) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "TEMP: %.1f C", current_temp);
    lcd_set_cursor(2, 4);
    lcd_print(buffer);
    current_temp += 2.5;
    sleep_ms(400);
  }

  lcd_clear();
  type_effect("   WATER READY!", 1, 50);
  sleep_ms(500);
}

// Determines coffee strength based on pressure
const char* determine_coffee_strength(int pressure) {
  if (pressure <= 33) return "MILD";
  else if (pressure <= 66) return "MEDIUM";
  else return "STRONG";
}

// Determines coffee temperature level
const char* determine_temperature_level(float temperature) {
  if (temperature < 90) return "WARM";
  else if (temperature < 94) return "HOT";
  else return "HOT++";
}

// Main function to simulate coffee preparation
// 1. Verifies resources
// 2. Lights up the LED bar based on coffee strength
// 3. Simulates water heating to the desired temperature
// 4. Moves servos and the stepper motor
// 5. Finalizes the process and updates resources
void prepare_coffee(int cups) {
  int pressure = read_intensity();                   // Coffee strength (extraction pressure)
  float desired_temp = read_desired_temperature();   // Desired beverage temperature
  int water_per_cup = read_water_quantity();         // Water amount per cup
  const char* strength = determine_coffee_strength(pressure);
  const char* temp_level = determine_temperature_level(desired_temp);

  check_simulated_resources(cups, water_per_cup); // Verifies resources using a simulated routine

  gpio_put(BLUE_LED, 1); // Turn on the blue LED to indicate preparation
  play_tone(BUZZER_PIN, 500, 600, 0.8);  // Sound at the start of preparation
  sleep_ms(1000);

  lcd_clear();
  lcd_set_cursor(1, 0);
  lcd_print("STARTING PROCESS ...");
  for (int i = 0; i <= 80; i += 10) {
    progress_bar(i, 2);
    sleep_ms(300);
  }

  update_led_bar(pressure); // Updates the LED bar based on coffee strength

  simulate_water_heating(desired_temp); // Simulates water heating
  int total_water = cups * water_per_cup;

  // Moves the first servo to release coffee beans
  lcd_clear();
  lcd_set_cursor(1, 1);
  lcd_print("RELEASING BEANS...");
  servo1_motion();

  // Stepper motor simulates grinding coffee beans
  lcd_clear();
  lcd_set_cursor(1, 4);
  lcd_print("GRINDING ...");
  stepper_rotate(true, 5000, 5);
  sleep_ms(500);

  // Coffee extraction begins
  int brewing_time = 5000 - (pressure * 20); // Adjusts brewing time based on pressure
  lcd_clear();

  char temp_buffer[21];
  snprintf(temp_buffer, sizeof(temp_buffer), "BREWING COFFEE:%s", temp_level);
  lcd_set_cursor(0, 0);
  lcd_print(temp_buffer);

  char water_buffer[21];
  if (cups == 1) {
    snprintf(water_buffer, sizeof(water_buffer), "1 CUP OF %d ML", water_per_cup);
  } else {
    snprintf(water_buffer, sizeof(water_buffer), "%d CUPS OF %d ML", cups, water_per_cup);
  }
  lcd_set_cursor(2, 0);
  lcd_print(water_buffer);

  char strength_buffer[21];
  snprintf(strength_buffer, sizeof(strength_buffer), "INTENSITY: %s", strength);
  lcd_set_cursor(3, 0);
  lcd_print(strength_buffer);

  servo2_move(45);
  sleep_ms(brewing_time);

  water_ml -= total_water;
  coffee_beans_g -= cups * 10;

  // Final message on the display
  servo2_motion();
  lcd_clear();
  fade_text("  COFFEE IS READY!", "      GRAB IT!", 1, 1000);
  play_coffee_ready(BUZZER_PIN);
  blink_led_bar(3, 300); // Blink LED bar
  gpio_put(BLUE_LED, 0);
  sleep_ms(2000);

  display_initial_screen();
  current_state = STATE_INITIAL_SCREEN; // Return to the initial screen
}