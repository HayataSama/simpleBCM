#include "horn.h"
#include "main.h"

// Keep track of current switch and LED state
static GPIO_PinState sw_horn = GPIO_PIN_RESET;
static HornState_t hornStatus = OFF;

void Horn_Init(void) {
  HAL_GPIO_WritePin(HORN_GPIO_Port, HORN_Pin, GPIO_PIN_SET);
}

void Horn_ReadInput(void) {
  sw_horn = HAL_GPIO_ReadPin(SW_HORN_GPIO_Port, SW_HORN_Pin);
}

void Horn_Update(void) {
  switch (hornStatus) {
  case OFF:
    break;

  case ON:
    break;

  case EXPIRED:
    break;

  case OV:
    break;

  case UV:
    break;
  }
}

void Horn_WriteOutput(void) {}

void Horn_Task(void) {}