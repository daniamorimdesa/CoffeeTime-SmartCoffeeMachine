//lcd_i2c.h

// This file provides LCD control functions, including:
// - Basic text display
// - Custom characters
// - Animations for better UI experience

#ifndef LCD_I2C_H
#define LCD_I2C_H

#include "hardware/i2c.h"

#define LCD_ADDR 0x27
#define LCD_ROWS 4
#define LCD_COLS 20

void lcd_init(i2c_inst_t *i2c);
void lcd_clear();
void init_i2c_lcd();
void lcd_set_cursor(int row, int col);
void lcd_print(const char *str);
void lcd_send_char(char c);
void create_custom_char(int location, uint8_t charmap[]);
void display_custom_char(int location, int row, int col);

void scroll_text(const char *message, int row, int delay_ms);
void type_effect(const char *message, int row, int delay_ms);
void progress_bar(int percentage, int row);
void blink_text(const char *message, int row, int col, int times, int delay_ms);
void fade_text(const char *message1, const char *message2, int row, int delay_ms);
void simple_clock();

#endif // LCD_I2C_H
