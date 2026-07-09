#ifndef __UTIL_H__
#define __UTIL_H__

#include "main.h"
#include <stdint.h>

typedef struct {
  GPIO_PinState swStable;
  uint8_t counter;
} Debounce_t;

// GPIO Logic Level: ActiveHigh or ActiveLow
typedef enum { AL, AH } PinLevel;

// Generic PinState regardless of pin logic level
typedef enum { OFF, ON } PinState;

// Convert milivolts to 12bit ADC value assuming 3.3V VDDA
#define MV2ADC(v) ((uint32_t)(v) * 4095 / 3300)

// Convert PinState to GPIO_PinState based on logic level
#define GPIO2STATE(pin, level)                                                 \
  (((GPIO_PinState)(pin) ^ (PinLevel)(level)) ? (PinState)(OFF)                \
                                              : (PinState)(ON))

// Convert GPIO_PinState to PinState based on logic level
#define STATE2GPIO(state, level)                                               \
  (((PinState)(state) ^ (PinLevel)(level)) ? (GPIO_PinState)(GPIO_PIN_RESET)   \
                                           : (GPIO_PinState)(GPIO_PIN_SET))

/**
 * @brief Debounce a switch
 *
 * @param sw  GPIO_PinState of switch
 * @param cnt Number of cycles to wait for stabilization.
 *            Note that this multiplys by the period of the function
 *            that calls debounce() to get real time.
 *
 * @retval GPIO_PinState swStable
 */
static GPIO_PinState debounce(GPIO_PinState sw, uint8_t cnt, Debounce_t *db) {
  if (sw == db->swStable) {
    db->counter = 0;
  } else {
    if (++db->counter >= cnt) {
      db->counter = 0;
      db->swStable = sw;
    }
  }

  return db->swStable;
}

#endif