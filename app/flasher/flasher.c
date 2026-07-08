#include "flasher.h"
#include "adc.h"
#include "battery.h"
#include "main.h"
#include "util.h"
#include <stdint.h>
#include <stdio.h>

#define LAMP_HEALTH_LT 620  // = 1V / 2
#define LAMP_HEALTH_UT 2482 // = 4V / 2

typedef enum {
  FLASHER_OFF,
  FLASHER_HAZARD,
  FLASHER_L,
  FLASHER_R,
  FLASHER_OV,
  FLASHER_UV
} FlasherState_t;

typedef struct {
  PinState leftSw;
  PinState rightSw;
  PinState hazardSw;
  uint16_t leftLampCurr;
  uint16_t rightLampCurr;
} FlasherInput_t;

FlasherInput_t flasherInputs = {.leftSw = OFF,
                                .rightSw = OFF,
                                .hazardSw = OFF,
                                .leftLampCurr = 0,
                                .rightLampCurr = 0};

static FlasherState_t flasherState = FLASHER_OFF;
static FlasherState_t flasherStatePrev = FLASHER_OFF;
static PinState flasherCmds[2] = {OFF,
                                  OFF}; // [0]: Left Flasher [1]: Right Flasher
static uint8_t flasherBlinkPeriod = 40; // 400ms with 10ms task period
static uint8_t toggleHazardState = 0;
static uint8_t flasherTimer = 0;
static uint8_t diagSent[2] = {0}; // [0]: Left Flasher [1]: Right Flasher

static void checkLampHealth(void) {
  /* Check Left Lamp*/
  if (flasherInputs.leftLampCurr < LAMP_HEALTH_LT) {
    // Open Circuit
    flasherBlinkPeriod = 20;
    if (!diagSent[0]) {
      diagSent[0] = 1;
      printf("FLHOC\r\n");
    }
  } else if (flasherInputs.leftLampCurr > LAMP_HEALTH_UT) {
    // Short Circuit
    // TODO: Requires a new cmd
    flasherCmds[0] = 0;
    if (!diagSent[0]) {
      diagSent[0] = 1;
      printf("FLHSC\r\n");
    }
  } else {
    // Healthy
    flasherBlinkPeriod = 40;
    diagSent[0] = 0;
  }

  /* Check Left Lamp*/
  if (flasherInputs.rightLampCurr < LAMP_HEALTH_LT) {
    // Open Circuit
    flasherBlinkPeriod = 20;
    if (!diagSent[1]) {
      diagSent[1] = 1;
      printf("FRHOC\r\n");
    }
  } else if (flasherInputs.rightLampCurr > LAMP_HEALTH_UT) {
    // Short Circuit
    // TODO: Requires a new cmd
    flasherCmds[1] = 0;
    if (!diagSent[1]) {
      diagSent[1] = 1;
      printf("FRHSC\r\n");
    }
  } else {
    // Healthy
    flasherBlinkPeriod = 40;
    diagSent[1] = 0;
  }
}

void Flasher_Init(void) {
  HAL_GPIO_WritePin(FLASHER_L_GPIO_Port, FLASHER_L_Pin, STATE2GPIO(OFF, AH));
  HAL_GPIO_WritePin(FLASHER_R_GPIO_Port, FLASHER_R_Pin, STATE2GPIO(OFF, AL));
}

void Flasher_ReadInput(void) {
  static GPIO_PinState input = 0;
  static PinState hazardSwPrev = OFF;
  static Debounce_t leftSwDb = {.swStable = STATE2GPIO(OFF, AL), .counter = 0};
  static Debounce_t rightSwDb = {.swStable = STATE2GPIO(OFF, AL), .counter = 0};
  static Debounce_t hazardSwDb = {.swStable = STATE2GPIO(OFF, AH),
                                  .counter = 0};

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

void Flasher_Update(void) {
  switch (batteryStatus) {
  case BATTERY_UV:
    if (flasherState != FLASHER_HAZARD) {
      flasherCmds[0] = OFF;
      flasherCmds[1] = OFF;
      flasherState = FLASHER_UV;
    } else {
      // probably do nothing?
    }
    // TODO: write fault to diag if it's the first time this loop is run
    break;

  case BATTERY_OV:
    if (flasherState != FLASHER_HAZARD) {
      flasherCmds[0] = OFF;
      flasherCmds[1] = OFF;
      flasherState = FLASHER_OV;
    } else {
      // probably do nothing?
    }
    break;

  case BATTERY_OK:
    // Every time we transition between states, we turn the output OFF for a
    // breif moment to avoid desyncronization between left and right flashers

    switch (flasherState) {
    case FLASHER_OFF:
      flasherCmds[0] = OFF;
      flasherCmds[1] = OFF;

      if (toggleHazardState) {
        toggleHazardState = 0;
        flasherState = FLASHER_HAZARD;
        flasherStatePrev = FLASHER_OFF;
      } else if (flasherInputs.leftSw == ON) {
        flasherState = FLASHER_L;
        flasherStatePrev = FLASHER_OFF;
      } else if (flasherInputs.rightSw == ON) {
        flasherState = FLASHER_R;
        flasherStatePrev = FLASHER_OFF;
      }
      break;

    case FLASHER_HAZARD:
      if (++flasherTimer >= flasherBlinkPeriod) {
        flasherTimer = 0;
        flasherCmds[0] ^= ON;
        flasherCmds[1] ^= ON;
      }

      if (!toggleHazardState && flasherInputs.leftSw) {
        flasherCmds[0] = OFF;
        flasherCmds[1] = OFF;

        flasherState = FLASHER_L;
        flasherStatePrev = FLASHER_HAZARD;
      } else if (!toggleHazardState && flasherInputs.rightSw) {
        flasherCmds[0] = OFF;
        flasherCmds[1] = OFF;

        flasherState = FLASHER_R;
        flasherStatePrev = FLASHER_HAZARD;
      } else if (toggleHazardState) {
        // FIXME: these two fail when we want to change from H -> OFF in the
        // first one and H -> L in the second one
        // OFF -> H -> L -> H -> OFF
        // OFF -> L -> H -> L -> OFF
        toggleHazardState = 0;

        flasherCmds[0] = OFF;
        flasherCmds[1] = OFF;

        flasherState = flasherStatePrev;
        flasherStatePrev = FLASHER_HAZARD;
      }
      break;

    case FLASHER_L:
      if (++flasherTimer >= flasherBlinkPeriod) {
        flasherTimer = 0;
        flasherCmds[0] ^= ON;
      }
      flasherCmds[1] = OFF;

      // next state
      if (flasherInputs.leftSw == OFF) {
        flasherCmds[0] = OFF;
        flasherCmds[1] = OFF;

        flasherState = flasherStatePrev;
        flasherStatePrev = FLASHER_L;
      } else if (flasherInputs.leftSw == ON && toggleHazardState == 1) {
        toggleHazardState = 0;

        flasherCmds[0] = OFF;
        flasherCmds[1] = OFF;

        flasherState = FLASHER_HAZARD;
        flasherStatePrev = FLASHER_L;
      }
      break;

    case FLASHER_R:
      if (++flasherTimer >= flasherBlinkPeriod) {
        flasherTimer = 0;
        flasherCmds[1] ^= ON;
      }
      flasherCmds[0] = OFF;

      // next state
      if (flasherInputs.rightSw == OFF) {
        flasherCmds[0] = OFF;
        flasherCmds[1] = OFF;

        flasherState = flasherStatePrev;
        flasherStatePrev = FLASHER_R;
      } else if (flasherInputs.rightSw == ON && toggleHazardState == 1) {
        toggleHazardState = 0;

        flasherCmds[0] = OFF;
        flasherCmds[1] = OFF;

        flasherState = FLASHER_HAZARD;
        flasherStatePrev = FLASHER_R;
      }
      break;

    case FLASHER_OV:
      break;

    case FLASHER_UV:
      break;
    }
    break;
  }

  // This overwrites flasherCmds if lamps have a problem
  checkLampHealth();
}

void Flasher_WriteOutput(void) {
  HAL_GPIO_WritePin(FLASHER_L_GPIO_Port, FLASHER_L_Pin,
                    flasherCmds[0] ? STATE2GPIO(ON, AH) : STATE2GPIO(OFF, AH));
  HAL_GPIO_WritePin(FLASHER_R_GPIO_Port, FLASHER_R_Pin,
                    flasherCmds[1] ? STATE2GPIO(ON, AL) : STATE2GPIO(OFF, AL));
}
