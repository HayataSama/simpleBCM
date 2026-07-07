#include "flasher.h"
#include "adc.h"
#include "main.h"
#include "util.h"
#include <stdint.h>

typedef struct {
  PinState leftSw;
  PinState rightSw;
  PinState hazardSw;
  uint16_t leftLampCurr;
  uint16_t rightLampCurr;
} FlasherInput_t;

static PinState flasherCmds[2] = {OFF,
                                  OFF}; // [0]: Left Flasher [1]: Right Flasher
static uint8_t toggleHazardState;
static FlasherInput_t flasherInputs = {.leftSw = OFF,
                                       .rightSw = OFF,
                                       .hazardSw = OFF,
                                       .leftLampCurr = 0,
                                       .rightLampCurr = 0};

void Flasher_Init(void) {
  HAL_GPIO_WritePin(FLASHER_L_GPIO_Port, FLASHER_L_Pin, STATE2GPIO(OFF, AH));
  HAL_GPIO_WritePin(FLASHER_R_GPIO_Port, FLASHER_R_Pin, STATE2GPIO(OFF, AL));
}

void Flasher_ReadInput(void) {
  static GPIO_PinState input = 0;
  static PinState hazardSwPrev = OFF;
  static Debounce_t leftSwDb = {0};
  static Debounce_t rightSwDb = {0};
  static Debounce_t hazardSwDb = {0};

  input = HAL_GPIO_ReadPin(SW_FLASHER_L_GPIO_Port, SW_FLASHER_L_Pin);
  flasherInputs.leftSw = GPIO2STATE(debounce(input, 5, &leftSwDb), AL);

  input = HAL_GPIO_ReadPin(SW_FLASHER_R_GPIO_Port, SW_FLASHER_R_Pin);
  flasherInputs.rightSw = GPIO2STATE(debounce(input, 5, &rightSwDb), AL);

  input = HAL_GPIO_ReadPin(SW_HAZARD_GPIO_Port, SW_HAZARD_Pin);
  flasherInputs.hazardSw = GPIO2STATE(debounce(input, 5, &hazardSwDb), AH);

  flasherInputs.leftLampCurr = adcValues.leftFlasherCurr;
  flasherInputs.rightLampCurr = adcValues.rightFlasherCurr;

  if (hazardSwPrev == OFF && flasherInputs.hazardSw == ON) {
    // Switch pressed
    toggleHazardState = 1;
    hazardSwPrev = flasherInputs.hazardSw;
  } else if (hazardSwPrev == ON && flasherInputs.hazardSw == OFF) {
    // Switch released
    hazardSwPrev = flasherInputs.hazardSw;
  }
}

void Flasher_Update(void) {}

void Flasher_WriteOutput(void) {
  HAL_GPIO_WritePin(FLASHER_L_GPIO_Port, FLASHER_L_Pin,
                    flasherCmds[0] ? STATE2GPIO(ON, AH) : STATE2GPIO(OFF, AH));
  HAL_GPIO_WritePin(FLASHER_R_GPIO_Port, FLASHER_R_Pin,
                    flasherCmds[1] ? STATE2GPIO(ON, AL) : STATE2GPIO(OFF, AL));
}