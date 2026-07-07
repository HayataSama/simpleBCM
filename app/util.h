#ifndef __UTIL_H__
#define __UTIL_H__

#include "main.h"
#include <stdint.h>

#define MV2ADC(v) ((uint32_t)(v) * 4095 / 3300)

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
static GPIO_PinState debounce(GPIO_PinState sw, uint8_t cnt) {
  static GPIO_PinState swStable = 0;
  static uint8_t counter = 0;

  if (sw == swStable) {
    counter = 0;
  } else {
    counter++;
    if (counter > cnt) {
      swStable = sw;
    }
  }

  return swStable;
}

#endif