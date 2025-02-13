// sensors.h
// Includes ADC (linear potentiometers), DHT22 (temperature/humidity), and RTC (real-time clock)

#ifndef SENSORS_H
#define SENSORS_H

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "ir_control.h"
#include "lcd_i2c.h"

// RTC address
#define RTC_ADDR 0x68

// Types and structures

// Structure to store temperature and humidity readings
typedef struct {
  float humidity;
  float temp_celsius;
} dht_reading;

// Structure to store the configured time
typedef struct {
  uint8_t day;
  uint8_t month;
  uint8_t hour;
  uint8_t minutes;
  bool valid_time; // Indicates whether the configured time is valid
} ScheduledTime;

// Functions for ADC sensors (Potentiometers)
void init_adc();
int read_intensity();             // Reads coffee intensity (0 to 100%)
float read_desired_temperature(); // Reads the desired temperature (85°C to 95°C)
int read_water_quantity();        // Reads the desired water quantity (50 ml to 200 ml)

// Functions for the DHT22 sensor
void read_from_dht(dht_reading *result, const uint DHT_PIN);
float convert_to_fahrenheit(float temp_celsius);
bool is_valid_reading(const dht_reading *reading);
void print_dht_reading(const dht_reading *reading);

// Functions for the DS1307 RTC
void rtc_read(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t *rtc_data);
void format_time(uint8_t *rtc_data, char *time_buffer, char *date_buffer);
void get_current_date(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t *day, uint8_t *month, uint8_t *year);
void increment_date(uint8_t *day, uint8_t *month, uint8_t *year);
void configure_day(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t *day, uint8_t *month, uint8_t *year, const char *key);
uint8_t read_digit(const char *key, uint32_t timeout_ms);
void configure_hour(uint8_t *hour, const char *key);
void configure_minutes(uint8_t *minutes, const char *key);

// Function declaration for scheduled time
ScheduledTime configure_schedule(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, const char *key);

// Resource Management
void check_simulated_resources(int cups, int water_per_cup);

#endif // SENSORS_H