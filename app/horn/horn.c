#include "horn.h"
#include "battery.h"
#include "main.h"
#include "stm32f0xx_hal_gpio.h"
#include "util.h"
#include <stdint.h>

#define HORN_TIMEOUT (5000 / 10) // 5 seconds with 10ms period

typedef enum { HORN_OFF, HORN_ON, HORN_EXPIRED, HORN_OV, HORN_UV } HornState_t;

// Current horn and switch states
static GPIO_PinState sw_horn = GPIO_PIN_RESET;
static HornState_t hornStatus = HORN_OFF;

static HornState_t hornCmd = HORN_OFF;
static uint16_t hornTimer = 0;

void Horn_Init(void) {
  HAL_GPIO_WritePin(HORN_GPIO_Port, HORN_Pin, GPIO_PIN_SET);
}

void Horn_ReadInput(void) {
  static Debounce_t hornSwDb = {.swStable = STATE2GPIO(OFF, AH), .counter = 0};
  GPIO_PinState input = HAL_GPIO_ReadPin(SW_HORN_GPIO_Port, SW_HORN_Pin);
  sw_horn = debounce(input, 5, &hornSwDb);
}

void Horn_Update(void) {
  switch (batteryStatus) {
    /* Check battery voltage */
  case BATTERY_OV:
    hornCmd = HORN_OFF;
    hornStatus = HORN_OV;
    break;

  case BATTERY_UV:
    hornCmd = HORN_OFF;
    hornStatus = HORN_UV;
    break;

  case BATTERY_OK:
    /* Horn timeout logic */
    if (hornTimer >= HORN_TIMEOUT && hornStatus == HORN_ON) {
      hornCmd = HORN_OFF;
      hornStatus = HORN_EXPIRED;
    }

    /* Horn status update */
    switch (hornStatus) {
    case HORN_OFF:
      if (sw_horn == GPIO_PIN_SET) {
        hornCmd = HORN_ON;
        hornStatus = HORN_ON;
        hornTimer = 0;
      }
      break;

    case HORN_ON:
      hornTimer++;
      if (sw_horn == GPIO_PIN_RESET) {
        hornCmd = HORN_OFF;
        hornStatus = HORN_OFF;
      }
      break;

    case HORN_EXPIRED:
      // Requires a new cmd
      if (sw_horn == GPIO_PIN_RESET) {
        hornCmd = HORN_OFF;
        hornStatus = HORN_OFF;
      }
      break;

    case HORN_OV:
      if (sw_horn == GPIO_PIN_SET) {
        hornCmd = HORN_ON;
        hornStatus = HORN_ON;
      } else {
        hornCmd = HORN_OFF;
        hornStatus = HORN_OFF;
      }
      break;

    case HORN_UV:
      // Requires a new cmd
      if (sw_horn == GPIO_PIN_RESET) {
        hornCmd = HORN_OFF;
        hornStatus = HORN_OFF;
      }
      break;
    }
    break;
  }
}

void Horn_WriteOutput(void) {
  // Note: Inverted logic
  HAL_GPIO_WritePin(HORN_GPIO_Port, HORN_Pin,
                    hornCmd ? GPIO_PIN_RESET : GPIO_PIN_SET);
}