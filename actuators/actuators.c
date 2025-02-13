// actuators.c
// Includes control for LEDs, servomotors, stepper motor, and buzzer

#include <math.h>  // For using fmax
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "actuators.h"

#define GREEN_LED 7          // Green LED: indicates that the system is on
#define RED_LED 12           // Red LED: indicates that the machine needs refilling
#define BLUE_LED 13          // Blue LED: indicates that the coffee preparation process is active
// LED bar to display the coffee strength
const uint LED_BAR_PINS[10] = {6, 9, 15, 22, 21, 20, 19, 18, 17, 16};

// Servo pins definition
#define SERVO1_PIN 11 // Servo 1: Coffee bean gate
#define SERVO2_PIN 10 // Servo 2: Ground coffee gate
// Stepper motor pin definition
#define DIR_PIN 2    // Direction control pin
#define STEP_PIN 3   // Step control pin
#define BUZZER_PIN 14 // Buzzer for sound notifications

// -------------------------------------------------------------------------------------------------- //
// LEDs

void init_leds() {
  gpio_init(GREEN_LED); 
  gpio_set_dir(GREEN_LED, GPIO_OUT); 
  gpio_put(GREEN_LED, 0); 

  gpio_init(BLUE_LED);
  gpio_set_dir(BLUE_LED, GPIO_OUT);
  gpio_put(BLUE_LED, 0);

  gpio_init(RED_LED);
  gpio_set_dir(RED_LED, GPIO_OUT);
  gpio_put(RED_LED, 0);
}

void init_led_bar() {
  for (int i = 0; i < 10; i++) {
    gpio_init(LED_BAR_PINS[i]);
    gpio_set_dir(LED_BAR_PINS[i], GPIO_OUT);
    gpio_put(LED_BAR_PINS[i], 0);
  }
}

void blink_led_bar(int times, int interval_ms) {
  for (int i = 0; i < times; i++) {
    for (int j = 0; j < 10; j++) {
      gpio_put(LED_BAR_PINS[j], 1);
    }
    sleep_ms(interval_ms);

    for (int j = 0; j < 10; j++) {
      gpio_put(LED_BAR_PINS[j], 0);
    }
    sleep_ms(interval_ms);
  }
}

void update_led_bar(int pressure) {
  int num_leds = (int)fmax(1, (pressure * 10) / 100);
  for (int i = 0; i < 10; i++) {
    gpio_put(LED_BAR_PINS[i], (i < num_leds) ? 1 : 0);
    sleep_ms(200);
  }
}

// -------------------------------------------------------------------------------------------------- //
// Servomotors

void servo_init(void) {
  gpio_set_function(SERVO1_PIN, GPIO_FUNC_PWM);
  uint slice1 = pwm_gpio_to_slice_num(SERVO1_PIN);
  pwm_set_clkdiv(slice1, 64.0f);
  pwm_set_wrap(slice1, 20000);
  pwm_set_gpio_level(SERVO1_PIN, 0);
  pwm_set_enabled(slice1, true);

  gpio_set_function(SERVO2_PIN, GPIO_FUNC_PWM);
  uint slice2 = pwm_gpio_to_slice_num(SERVO2_PIN);
  pwm_set_clkdiv(slice2, 64.0f);
  pwm_set_wrap(slice2, 20000);
  pwm_set_gpio_level(SERVO2_PIN, 0);
  pwm_set_enabled(slice2, true);
}

void servo1_move(uint angle) {
  if (angle > 180) angle = 180;
  uint pulse_width = 870 + (angle * 2000 / 180);
  pwm_set_gpio_level(SERVO1_PIN, pulse_width);
}

void servo2_move(uint angle) {
  if (angle > 180) angle = 180;
  uint pulse_width = 870 + (angle * 2000 / 180);
  pwm_set_gpio_level(SERVO2_PIN, pulse_width);
}

void servo1_motion(void) {
  servo1_move(0);
  servo2_move(0);
  sleep_ms(500);

  servo1_move(90);
  sleep_ms(1000);
  servo1_move(180);
  sleep_ms(1000);
  servo1_move(0);
  sleep_ms(100);
}

void servo2_motion(void) {
  servo2_move(90);
  sleep_ms(1000);
  servo2_move(180);
  sleep_ms(1000);
  servo2_move(0);
  sleep_ms(100);
}

// -------------------------------------------------------------------------------------------------- //
// Stepper Motor

void stepper_init(void) {
  gpio_init(STEP_PIN);
  gpio_set_dir(STEP_PIN, GPIO_OUT);
  gpio_put(STEP_PIN, 0);

  gpio_init(DIR_PIN);
  gpio_set_dir(DIR_PIN, GPIO_OUT);
  gpio_put(DIR_PIN, 0);
}

void stepper_rotate(bool direction, uint32_t duration_ms, uint32_t step_delay_ms) {
  gpio_put(DIR_PIN, direction);
  uint32_t steps = duration_ms / step_delay_ms;
  for (uint32_t i = 0; i < steps; i++) {
    gpio_put(STEP_PIN, 1);
    sleep_ms(step_delay_ms / 2);
    gpio_put(STEP_PIN, 0);
    sleep_ms(step_delay_ms / 2);
  }
}

// -------------------------------------------------------------------------------------------------- //
// Buzzer

void setup_pwm(uint pin, uint freq, float duty_cycle) {
  gpio_set_function(pin, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(pin);
  uint channel = pwm_gpio_to_channel(pin);

  uint32_t clock = 125000000;
  uint32_t divider16 = clock / freq / 4096 + (clock % (freq * 4096) != 0);
  pwm_set_clkdiv(slice_num, divider16 / 16.0f);
  pwm_set_wrap(slice_num, 4095);
  pwm_set_chan_level(slice_num, channel, (uint32_t)(4095 * duty_cycle));
  pwm_set_enabled(slice_num, true);
}

void stop_pwm(uint pin) {
  uint slice_num = pwm_gpio_to_slice_num(pin);
  uint channel = pwm_gpio_to_channel(pin);
  pwm_set_chan_level(slice_num, channel, 0);
  pwm_set_enabled(slice_num, false);
}

void play_tone(uint pin, uint freq, uint duration_ms, float duty_cycle) {
  setup_pwm(pin, freq, duty_cycle);
  sleep_ms(duration_ms);
  stop_pwm(pin);
}

void play_error_tone(uint pin) {
  for (int i = 0; i < 3; i++) {
    play_tone(pin, 3000, 200, 0.5);
    sleep_ms(200);
  }
}

void play_beep_pattern(uint pin, uint freq, uint duration_ms, uint pause_ms, int repetitions, float duty_cycle) {
  for (int i = 0; i < repetitions; i++) {
    play_tone(pin, freq, duration_ms, duty_cycle);
    sleep_ms(pause_ms);
  }
}

void play_success_tone(uint pin) {
  play_tone(pin, 1000, 500, 0.5);
  sleep_ms(100);
  play_tone(pin, 2000, 500, 0.5);
}

void play_coffee_ready(uint pin) {
  play_tone(pin, 262, 200, 0.5);
  sleep_ms(100);
  play_tone(pin, 294, 200, 0.5);
  sleep_ms(100);
  play_tone(pin, 330, 200, 0.5);
  sleep_ms(100);
  play_tone(pin, 349, 200, 0.5);
  sleep_ms(100);
  play_tone(pin, 392, 400, 0.5);
}