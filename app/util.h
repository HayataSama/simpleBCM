#ifndef __UTIL_H__
#define __UTIL_H__

#include "main.h"
#include <stdint.h>

typedef struct {
  GPIO_PinState swStable;
  uint8_t counter;
} Debounce_t;

// GPIO Pin Level: Active-High or Active-Low
typedef enum { AL, AH } PinLevel;

typedef enum { OFF, ON } PinState;

#define MV2ADC(v) ((uint32_t)(v) * 4095 / 3300)
#define GPIO2STATE(pin, level)                                                 \
  (((GPIO_PinState)(pin) ^ (PinLevel)(level)) ? (PinState)(OFF)                \
                                              : (PinState)(ON))
#define STATE2GPIO(state, level)                                               \
  (((PinState)(state) ^ (PinLevel)(level)) ? (GPIO_PinState)(GPIO_PIN_RESET)   \
                                           : (GPIO_PinState)(GPIO_PIN_SET))

/**
 * @brief Debounces a switch
 *
 * @param sw  PinState of switch
 * @param cnt Number of cycles to wait for stabilization.
 *            Note that this multiplys by the period of the function
 *            that calls Debounce() to get real time.
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