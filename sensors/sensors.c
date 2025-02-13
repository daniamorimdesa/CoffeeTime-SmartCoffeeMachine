// sensors.c
// Includes ADC (linear potentiometers), DHT22 (temperature/humidity), and RTC (real-time clock)
// Also performs resource verification in the machine (future implementation with real sensors)

#include "sensors.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include "lcd_i2c.h"
#include "ir_control.h"
#include "pico/time.h"
#include "actuators.h"

#define RED_LED 12    // Red LED: indicates that the machine needs refilling
#define BUZZER_PIN 14 // Buzzer: used for sound notifications

extern float water_ml;
extern float coffee_beans_g;
extern bool play_pressed;
const uint MAX_TIMINGS = 85;

typedef enum {
  STATE_CONFIG_DAY,
  STATE_CONFIG_HOUR,
  STATE_CONFIG_MINUTES,
  STATE_VALIDATION,
  STATE_COMPLETED,
  STATE_INVALID,
} TimeConfigState;

// ---------------------------------- ADC (Potentiometers) ---------------------------------- //
// Initializes the ADC and configures the potentiometer pins
void init_adc() {
  adc_init();
  adc_gpio_init(26); // INTENSITY_POT_PIN
  adc_gpio_init(27); // TEMP_WATER_PIN
  adc_gpio_init(28); // WATER_AMOUNT_PIN
}

// Reads the intensity potentiometer (0 to 100%)
int read_intensity() {
  adc_select_input(0); // ADC0 (GPIO26)
  sleep_us(500);       // Waits for stabilization
  adc_read();          // Discards the first reading
  uint16_t raw_value = adc_read();
  return (raw_value * 100) / 4095; // Converts to percentage
}

// Reads the temperature potentiometer (85°C to 95°C)
float read_desired_temperature() {
  adc_select_input(1); // ADC1 (GPIO27)
  sleep_us(500);
  adc_read();          // Discards the first reading
  uint16_t raw_value = adc_read();

  float percentage = (raw_value * 100.0) / 4095.0;
  return 85.0 + ((percentage * 10.0) / 100.0); // Maps to 85°C - 95°C
}

// Reads the water quantity potentiometer (50 ml to 200 ml)
int read_water_quantity() {
  adc_select_input(2); // ADC2 (GPIO28)
  sleep_us(500);
  adc_read();          // Discards the first reading
  uint16_t raw_value = adc_read();
  return 50 + ((raw_value * 150) / 4095); // Maps to 50 ml - 200 ml
}

// ---------------------------------- DHT22 (Temperature and Humidity) ---------------------------------- //
void read_from_dht(dht_reading *result, const uint DHT_PIN) {
  int data[5] = {0, 0, 0, 0, 0};
  uint last = 1;
  uint j = 0;

  // Initialization
  gpio_set_dir(DHT_PIN, GPIO_OUT);
  gpio_put(DHT_PIN, 0);
  sleep_ms(20);
  gpio_set_dir(DHT_PIN, GPIO_IN);

  // Sensor reading
  for (uint i = 0; i < MAX_TIMINGS; i++) {
    uint count = 0;
    while (gpio_get(DHT_PIN) == last) {
      count++;
      sleep_us(1);
      if (count == 255) break;
    }
    last = gpio_get(DHT_PIN);
    if (count == 255) break;

    if ((i >= 4) && (i % 2 == 0)) {
      data[j / 8] <<= 1;
      if (count > 50) { // Fine-tuning of timing
        data[j / 8] |= 1;
      }
      j++;
    }
  }

  // Data validation
  if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
    result->humidity = (float)((data[0] << 8) + data[1]) / 10;
    if (result->humidity > 100) {
      result->humidity = data[0];
    }
    result->temp_celsius = (float)(((data[2] & 0x7F) << 8) + data[3]) / 10;
    if (result->temp_celsius > 125) {
      result->temp_celsius = data[2];
    }
    if (data[2] & 0x80) {
      result->temp_celsius = -result->temp_celsius;
    }
  } else {
    printf("Invalid DHT22 data\n");
    result->humidity = -1; // Error value
    result->temp_celsius = -1; // Error value
  }
}

float convert_to_fahrenheit(float temp_celsius) {
  return (temp_celsius * 9 / 5) + 32;
}

bool is_valid_reading(const dht_reading *reading) {
  return reading->humidity > 0 && reading->temp_celsius > -40 && reading->temp_celsius < 125;
}

void print_dht_reading(const dht_reading *reading) {
  if (is_valid_reading(reading)) {
    float fahrenheit = convert_to_fahrenheit(reading->temp_celsius);
    printf("Humidity: %.1f%%, Temperature: %.1f°C (%.1f°F)\n",
           reading->humidity, reading->temp_celsius, fahrenheit);
  } else {
    printf("DHT22 reading error. Try again.\n");
  }
}

// ---------------------------------- RTC (Real-Time Clock) ---------------------------------- //
// Function to read RTC data
void rtc_read(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t *rtc_data) {
  // Configures the I2C pins for the RTC
  gpio_set_function(sda_pin, GPIO_FUNC_I2C);
  gpio_set_function(scl_pin, GPIO_FUNC_I2C);
  gpio_pull_up(sda_pin);
  gpio_pull_up(scl_pin);

  // Reads RTC data
  uint8_t reg = 0x00;
  int ret = i2c_write_blocking(i2c, RTC_ADDR, &reg, 1, true);
  if (ret < 0) {
    printf("Error writing to RTC\n");
    return;
  }
  ret = i2c_read_blocking(i2c, RTC_ADDR, rtc_data, 7, false);
  if (ret < 0) {
    printf("Error reading from RTC\n");
    return;
  }
}

// Function to format RTC data
void format_time(uint8_t *rtc_data, char *time_buffer, char *date_buffer) {
  const char *months[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
  };

  uint8_t seconds = (rtc_data[0] & 0x0F) + ((rtc_data[0] >> 4) * 10);
  uint8_t minutes = (rtc_data[1] & 0x0F) + ((rtc_data[1] >> 4) * 10);
  uint8_t hours = (rtc_data[2] & 0x0F) + ((rtc_data[2] >> 4) * 10);
  uint8_t date = (rtc_data[4] & 0x0F) + ((rtc_data[4] >> 4) * 10);
  uint8_t month = (rtc_data[5] & 0x0F) + ((rtc_data[5] >> 4) * 10);
  uint16_t year = 2000 + (rtc_data[6] & 0x0F) + ((rtc_data[6] >> 4) * 10);

  snprintf(time_buffer, 64, "%02d:%02d", hours, minutes);
  snprintf(date_buffer, 64, "%02d %s %04d", date, months[month - 1], year);
}

// Function to get the current date from the RTC
void get_current_date(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t *day, uint8_t *month, uint8_t *year) {
  uint8_t rtc_data[7];
  rtc_read(i2c, sda_pin, scl_pin, rtc_data);

  *day = (rtc_data[4] & 0x0F) + ((rtc_data[4] >> 4) * 10);
  *month = (rtc_data[5] & 0x0F) + ((rtc_data[5] >> 4) * 10);
  *year = (rtc_data[6] & 0x0F) + ((rtc_data[6] >> 4) * 10);
}

// Function to increment the date
void increment_date(uint8_t *day, uint8_t *month, uint8_t *year) {
  const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  uint8_t max_days = days_in_month[*month - 1];

  // Checks for leap year in February
  if (*month == 2 && ((*year % 4 == 0 && *year % 100 != 0) || (*year % 400 == 0))) {
    max_days = 29;
  }

  if (*day < max_days) {
    (*day)++;
  } else {
    *day = 1;
    if (*month < 12) {
      (*month)++;
    } else {
      *month = 1;
      (*year)++;
    }
  }
}

void configure_day(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t *day, uint8_t *month, uint8_t *year, const char *key) {
  uint8_t current_day, current_month, current_year;
  get_current_date(i2c, sda_pin, scl_pin, &current_day, &current_month, &current_year);

  lcd_clear();
  lcd_set_cursor(0, 0);
  lcd_print("SCHEDULE FOR:");
  lcd_set_cursor(2, 0);
  lcd_print("+ : TOMORROW");
  lcd_set_cursor(3, 0);
  lcd_print("- : TODAY");

  uint32_t start_time = to_ms_since_boot(get_absolute_time());

  while (to_ms_since_boot(get_absolute_time()) - start_time < 30000) { // 30 seconds timeout
    if (strcmp(key, "+") == 0) {
      increment_date(&current_day, &current_month, &current_year); // Increment to tomorrow
      *day = current_day;
      *month = current_month;
      break;
    } else if (strcmp(key, "-") == 0) {
      *day = current_day;  // Keeps the current day
      *month = current_month;
      break;
    }
  }

  lcd_clear();
  lcd_set_cursor(0, 0);
  lcd_print("DATE CONFIRMED!");
  sleep_ms(1000);
}

uint8_t read_digit(const char *key, uint32_t timeout_ms) {
  uint32_t start_time = to_ms_since_boot(get_absolute_time());
  uint8_t digit = 0xFF; // Invalid value by default

  while (to_ms_since_boot(get_absolute_time()) - start_time < timeout_ms) {
    if (strlen(key) == 1 && isdigit(key[0])) {
      digit = key[0] - '0'; // Converts the character to a number
      break;
    }
  }
  sleep_ms(100);
  return digit;
}

void configure_hour(uint8_t *hour, const char *key) {
  uint8_t first_digit, second_digit;
  lcd_clear();
  lcd_set_cursor(0, 0);
  lcd_print("SET HOURS:");
  lcd_set_cursor(2, 2);
  lcd_print(":");

  first_digit = read_digit(key, 30000); // 30-second timeout
  if (first_digit > 2) first_digit = 0;
  lcd_set_cursor(2, 0);
  char buffer[2] = {first_digit + '0', '\0'};
  lcd_print(buffer);
  sleep_ms(1000);

  second_digit = read_digit(key, 30000);
  if (first_digit == 2 && second_digit > 3) second_digit = 0;
  lcd_set_cursor(2, 1);
  char buffer1[2] = {second_digit + '0', '\0'};
  lcd_print(buffer1);
  sleep_ms(2000);

  *hour = (first_digit * 10) + second_digit;

  lcd_set_cursor(0, 0);
  lcd_print("HOURS OK!         ");
  sleep_ms(2000);
}

void configure_minutes(uint8_t *minutes, const char *key) {
  uint8_t first_digit, second_digit;
  lcd_set_cursor(0, 0);
  lcd_print("SET MINUTES:");

  sleep_ms(1500);
  first_digit = read_digit(key, 30000);
  if (first_digit > 5) first_digit = 0;
  lcd_set_cursor(2, 3);
  char buffer[2] = {first_digit + '0', '\0'};
  lcd_print(buffer);
  sleep_ms(1000);

  second_digit = read_digit(key, 30000);
  lcd_set_cursor(2, 4);
  char buffer1[2] = {second_digit + '0', '\0'};
  lcd_print(buffer1);
  sleep_ms(2000);

  *minutes = first_digit * 10 + second_digit;

  lcd_clear();
  lcd_print("MIN CONFIRMED!");
  sleep_ms(1000);
}
// Function declaration for scheduled time
ScheduledTime configure_schedule(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, const char *key) {
  ScheduledTime scheduled_time = {0, 0, 0, 0, false};  // Inicializa a estrutura corretamente
  TimeConfigState current_state = STATE_CONFIG_DAY;

  while (true) {
    switch (current_state) {
      case STATE_CONFIG_DAY: {
          uint8_t year;
          configure_day(i2c, sda_pin, scl_pin, &scheduled_time.day, &scheduled_time.month, &year, key);
          current_state = STATE_CONFIG_HOUR;
          break;
        }

      case STATE_CONFIG_HOUR:
        configure_hour(&scheduled_time.hour, key);
        current_state = STATE_CONFIG_MINUTES;
        break;

      case STATE_CONFIG_MINUTES:
        configure_minutes(&scheduled_time.minutes, key);
        current_state = STATE_VALIDATION;
        break;

      case STATE_VALIDATION: {
          uint8_t rtc_data[7];
          rtc_read(i2c, sda_pin, scl_pin, rtc_data);

          uint8_t current_day = (rtc_data[4] & 0x0F) + ((rtc_data[4] >> 4) * 10);
          uint8_t current_month = (rtc_data[5] & 0x0F) + ((rtc_data[5] >> 4) * 10);
          uint8_t current_hour = (rtc_data[2] & 0x0F) + ((rtc_data[2] >> 4) * 10);
          uint8_t current_minute = (rtc_data[1] & 0x0F) + ((rtc_data[1] >> 4) * 10);

          // Check if the scheduled time is in the future
          if (scheduled_time.month > current_month ||
              (scheduled_time.month == current_month && scheduled_time.day > current_day) ||
              (scheduled_time.month == current_month && scheduled_time.day == current_day &&
               (scheduled_time.hour > current_hour ||
                (scheduled_time.hour == current_hour && scheduled_time.minutes > current_minute)))) {
            scheduled_time.valid_time = true;
            current_state = STATE_COMPLETED;
          } else {
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_print("Invalid Date/Time!");
            lcd_set_cursor(2, 0);
            lcd_print("PRESS PLAY TO RESET:");
            sleep_ms(3000);
            current_state = STATE_INVALID;
          }
          break;
        }

      case STATE_COMPLETED:
        lcd_clear();
        char buffer[32];
        lcd_set_cursor(0, 0);
        lcd_print("COFFEE SCHEDULED!");
        snprintf(buffer, sizeof(buffer), "%02d/%02d", scheduled_time.day, scheduled_time.month);
        lcd_set_cursor(2, 0);
        lcd_print("DATE: ");
        lcd_set_cursor(2, 6);
        lcd_print(buffer);
        snprintf(buffer, sizeof(buffer), "%02d:%02d", scheduled_time.hour, scheduled_time.minutes);
        lcd_set_cursor(3, 0);
        lcd_print("TIME: ");
        lcd_set_cursor(3, 6);
        lcd_print(buffer);
        sleep_ms(3000);
        return scheduled_time;

      case STATE_INVALID:
        if (strcmp(key, "PLAY") == 0) {
          current_state = STATE_CONFIG_DAY;  // Restart the configuration
        }
        break;

      default:
        break;
    }
  }
}

// ---------------------------------- Resource Verification ---------------------------------- //
// Function to check the amount of water and coffee beans in the machine
// Verifies if there are enough resources for the selected number of cups.
// If resources are insufficient, alerts the user to refill.
void check_simulated_resources(int cups, int water_per_cup) {
  float required_beans = cups * 10; // 10g per cup
  float required_water = cups * water_per_cup; // Considers the chosen water quantity
  bool needs_refill = false;

  if (water_ml < required_water) { // Checks if there is enough water
    gpio_put(RED_LED, 1); // Turns on the red LED
    play_beep_pattern(BUZZER_PIN, 400, 400, 300, 4, 0.8); // Alert sound
    lcd_clear();
    blink_text("REFILL MACHINE!", 0, 2, 3, 500);
    lcd_set_cursor(2, 0);
    lcd_print("PRESS PLAY TO FILL:");
    needs_refill = true;
  }

  if (coffee_beans_g < required_beans) { // Checks if there are enough coffee beans
    gpio_put(RED_LED, 1); // Turns on the red LED
    play_beep_pattern(BUZZER_PIN, 400, 400, 300, 4, 0.8); // Alert sound
    lcd_clear();
    blink_text("REFILL MACHINE!", 0, 2, 3, 500);
    lcd_set_cursor(2, 0);
    lcd_print("PRESS PLAY TO FILL:");
    needs_refill = true;
  }

  if (needs_refill) {
    while (!play_pressed) { // Waits for the user to press PLAY
      sleep_ms(200); // Waiting loop
    }

    // Simulates refilling beans and water
    coffee_beans_g = 250.0; // Beans refilled
    water_ml = 1000.0;      // Water refilled
    gpio_put(RED_LED, 0);   // Turns off the red LED

    // Signals that the machine is ready again
    lcd_clear();
    lcd_set_cursor(1, 4);
    lcd_print("READY AGAIN!");
    play_success_tone(BUZZER_PIN); // Sound indicating the machine has been refilled
    sleep_ms(2000);
    play_pressed = false; // Resets the flag
  }
}