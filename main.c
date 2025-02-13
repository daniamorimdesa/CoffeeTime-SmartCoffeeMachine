/* Coffee Time - Smart Coffee Machine with Raspberry Pi Pico W. 
This IoT project automates personalized coffee preparation, integrating
sensors and actuators for real-time monitoring and control.
Fully simulated on the Wokwi platform, it was developed as the final 
project of the EmbarcaTech program.

Author: Daniela Amorim de Sá
Electronic Engineer | Embedded Systems & IoT
Project developed as part of the EmbarcaTech course.
Access on GitHub: 
https://github.com/daniamorimdesa/CoffeeTime-SmartCoffeeMachine
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/time.h"
#include <ctype.h>
// Modular project libraries
#include "lcd_i2c.h"
#include "ir_control.h"
#include "sensors.h"
#include "actuators.h"
#include "user_interface.h"
#include "internal_operations.h"
#include "state.h"

#define IR_SENSOR_GPIO_PIN 1 // Remote IR control for sending commands to the machine

int main() {
  setup_machine();
  init_ir_irq_receiver(IR_SENSOR_GPIO_PIN, &ir_callback);

  while (true) {
    manage_state();  // Delegating control to the current state
    sleep_ms(200);
  }
  return 0;
}