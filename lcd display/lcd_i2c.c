// lcd_i2c.c

/*LCD Commands:
  0x28 - Set 4-bit mode, 2-line display
  0x08 - Display OFF
  0x0C - Display ON, cursor OFF
  0x01 - Clear display
  0x06 - Increment cursor (shift right)*/

#include "lcd_i2c.h"
#include "hardware/i2c.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include <string.h>

// I2C communication for the LCD display and RTC
#define I2C_PORT i2c0
#define SDA_PIN 4
#define SCL_PIN 5
static i2c_inst_t *i2c_instance;

/*Sends a command to the LCD over I2C in 4-bit mode
  The command is split into two nibbles (upper and lower) since
  the LCD controller only processes 4 bits at a time.
  This is required for compatibility with the I2C expander module.*/
static void lcd_send_command(uint8_t cmd) {
  uint8_t upper = cmd & 0xF0;
  uint8_t lower = (cmd << 4) & 0xF0;

  uint8_t data[4] = {
    upper | 0x0C, // Sends enable signal
    upper,        // Disables enable signal
    lower | 0x0C, // Sends enable signal
    lower         // Disables enable signal
  };

  i2c_write_blocking(i2c_instance, LCD_ADDR, data, sizeof(data), false);
}

// Writes a character to the LCD
void lcd_send_char(char c) {
  uint8_t upper = c & 0xF0;
  uint8_t lower = (c << 4) & 0xF0;

  uint8_t data[4] = {
    upper | 0x0D, // Sends enable signal
    upper,        // Disables enable signal
    lower | 0x0D, // Sends enable signal
    lower         // Disables enable signal
  };

  i2c_write_blocking(i2c_instance, LCD_ADDR, data, sizeof(data), false);
}

void lcd_init(i2c_inst_t *i2c) {
  i2c_instance = i2c;

  sleep_ms(50); // Waits for LCD initialization
  lcd_send_command(0x03);
  sleep_ms(5);
  lcd_send_command(0x03);
  sleep_us(150);
  lcd_send_command(0x03);
  lcd_send_command(0x02);

  // LCD configuration
  lcd_send_command(0x28); // 4-bit mode, 2 lines
  lcd_send_command(0x08); // Turns off display
  lcd_send_command(0x01); // Clears display
  sleep_ms(2);
  lcd_send_command(0x06); // Increments cursor
  lcd_send_command(0x0C); // Turns on display and cursor
}

// Clears the display
void lcd_clear() {
  lcd_send_command(0x01); // Command to clear
  sleep_ms(2);
}

// Initializes I2C communication for the LCD
void init_i2c_lcd() {
  i2c_init(I2C_PORT, 100 * 1000); // Configures I2C bus to 100 kHz
  // Defines SDA and SCL pins
  gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
  // Enables pull-up resistors for stable communication
  gpio_pull_up(SDA_PIN);
  gpio_pull_up(SCL_PIN);

  lcd_init(I2C_PORT); // Initializes the LCD
  lcd_clear();       // Clears the screen
}

// Sets the cursor position for text display
void lcd_set_cursor(int row, int col) {
  const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
  lcd_send_command(0x80 | (col + row_offsets[row]));
}

// Prints a string on the LCD
void lcd_print(const char *str) {
  while (*str) {
    lcd_send_char(*str++);
  }
}

// Creates a custom character
void create_custom_char(int location, uint8_t charmap[]) {
  location &= 0x7; // The LCD supports 8 characters (0-7)
  lcd_send_command(0x40 | (location << 3));
  for (int i = 0; i < 8; i++) {
    lcd_send_char(charmap[i]);
  }
}

// Displays a custom character at a specific position
void display_custom_char(int location, int row, int col) {
  lcd_set_cursor(row, col);
  lcd_send_char(location);
}

// **Animation Functions**

// Scroll text animation
void scroll_text(const char *message, int row, int delay_ms) {
  int len = strlen(message);
  char buffer[LCD_COLS + 1] = {0};

  for (int start = 0; start < len; start++) {
    strncpy(buffer, message + start, LCD_COLS);
    buffer[LCD_COLS] = '\0';

    lcd_set_cursor(row, 0);
    lcd_print(buffer);
    sleep_ms(delay_ms);

    if (start + LCD_COLS >= len) {
      start = -1; // Restart the cycle
    }
  }
}
// Example usage in `main`:
// scroll_text("Welcome to Raspberry Pi Pico!", 0, 200);

// Typing effect animation
void type_effect(const char *message, int row, int delay_ms) {
  lcd_set_cursor(row, 0);
  for (int i = 0; i < strlen(message); i++) {
    lcd_print((char[]) {
      message[i], '\0'
    }); // Sends one character at a time
    sleep_ms(delay_ms);
  }
}
// Example usage in `main`:
// type_effect("Hello, World!", 0, 100);

// Progress bar animation
void progress_bar(int percentage, int row) {
  int filled = (percentage * LCD_COLS) / 100;
  lcd_set_cursor(row, 0);
  for (int i = 0; i < LCD_COLS; i++) {
    if (i < filled) {
      lcd_print("_");
    } else {
      lcd_print(" ");
    }
  }
}
// Example usage in `main`:
// for (int i = 0; i <= 100; i += 10) {
//   progress_bar(i, 1);
//   sleep_ms(500);
// }

// Blinking text animation (alert)
void blink_text(const char *message, int row, int col, int times, int delay_ms) {
  for (int i = 0; i < times; i++) {
    lcd_set_cursor(row, col);
    lcd_print(message);
    sleep_ms(delay_ms);

    lcd_set_cursor(row, col);
    for (int j = 0; j < strlen(message); j++) {
      lcd_print(" "); // Erases text
    }
    sleep_ms(delay_ms);
  }

  // Restores the text
  lcd_set_cursor(row, col);
  lcd_print(message);
}
// Example usage in `main`:
// blink_text("ALERT!", 0, 5, 5, 500);

// Fade text effect (erases and writes)
void fade_text(const char *message1, const char *message2, int row, int delay_ms) {
  lcd_set_cursor(row, 0);
  lcd_print(message1);
  sleep_ms(delay_ms);

  for (int i = strlen(message1); i >= 0; i--) {
    lcd_set_cursor(row, i);
    lcd_print(" ");
    sleep_ms(50);
  }

  lcd_set_cursor(row, 0);
  lcd_print(message2);
}
// Example usage in `main`:
// fade_text("Welcome!", "Learning C!", 0, 1000);

// Simple clock animation
void simple_clock() {
  for (int seconds = 0; seconds < 1000; seconds++) {
    char time[20];
    snprintf(time, sizeof(time), "Time: %03d sec", seconds);
    lcd_set_cursor(0, 0);
    lcd_print(time);
    sleep_ms(1000);
  }
}
// Example usage in `main`:
// simple_clock();