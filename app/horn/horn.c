#include "horn.h"
#include "battery.h"
#include "main.h"
#include "stm32f0xx_hal_gpio.h"
#include "util.h"
#include <stdint.h>

#define HORN_TIMEOUT 500 // 500 * 10ms ticks

typedef enum { HORN_OFF, HORN_ON, HORN_EXPIRED, HORN_OV, HORN_UV } HornState_t;

static PinState hornSw = OFF;
static PinState hornCmd = OFF;
static HornState_t hornStatus = HORN_OFF;

void Horn_Init(void) {
  HAL_GPIO_WritePin(HORN_GPIO_Port, HORN_Pin, STATE2GPIO(OFF, AL));
}

void Horn_ReadInput(void) {
  static Debounce_t hornSwDb = {.swStable = STATE2GPIO(OFF, AH), .counter = 0};
  static GPIO_PinState input = 0;

  input = HAL_GPIO_ReadPin(SW_HORN_GPIO_Port, SW_HORN_Pin);
  hornSw = GPIO2STATE(debounce(input, 5, &hornSwDb), AH);
}

void Horn_Update(void) {
  static uint16_t hornTimer = 0;

  /*
   * This module's state machine is implemented rather different from flasher
   * module (which is more complicated), but it works so there is no need to
   * update it.
   * I assumed OV, UV and EXPIRED as seperate states that horn can be in,
   * instead of just overwriting the output command and setting a flag.
   */
  switch (batteryStatus) {
    /* Check battery voltage */
  case BATTERY_OV:
    hornCmd = OFF;
    hornStatus = HORN_OV;
    break;

  case BATTERY_UV:
    hornCmd = OFF;
    hornStatus = HORN_UV;
    break;

  case BATTERY_OK:
    /* Horn timeout logic */
    if (hornTimer >= HORN_TIMEOUT && hornStatus == HORN_ON) {
      hornCmd = OFF;
      hornStatus = HORN_EXPIRED;
    }

    /* Horn status update */
    switch (hornStatus) {
    case HORN_OFF:
      if (hornSw == ON) {
        hornCmd = ON;
        hornStatus = HORN_ON;
        hornTimer = 0;
      }
      break;

    case HORN_ON:
      hornTimer++;
      if (hornSw == OFF) {
        hornCmd = OFF;
        hornStatus = HORN_OFF;
      }
      break;

    case HORN_EXPIRED:
      // Requires a new cmd
      if (hornSw == OFF) {
        hornCmd = OFF;
        hornStatus = HORN_OFF;
      }
      break;

    case HORN_OV:
      if (hornSw == ON) {
        hornCmd = ON;
        hornStatus = HORN_ON;
      } else {
        hornCmd = OFF;
        hornStatus = HORN_OFF;
      }
      break;

    case HORN_UV:
      // Requires a new cmd
      if (hornSw == OFF) {
        hornCmd = OFF;
        hornStatus = HORN_OFF;
      }
      break;
    }
    break;
  }
}

void Horn_WriteOutput(void) {
  HAL_GPIO_WritePin(HORN_GPIO_Port, HORN_Pin,
                    hornCmd ? STATE2GPIO(ON, AL) : STATE2GPIO(OFF, AL));
}