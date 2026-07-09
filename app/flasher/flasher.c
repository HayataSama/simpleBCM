#include "flasher.h"
#include "adc.h"
#include "battery.h"
#include "main.h"
#include "util.h"
#include <stdint.h>
#include <stdio.h>

#define LAMP_HEALTH_LT MV2ADC(500)  // = 1V / 2 resistor divider
#define LAMP_HEALTH_UT MV2ADC(2000) // = 4V / 2 resistor divider
#define MIN_FLASHER_BLINK 3
#define FLASHER_BLINK_PERIOD_NORMAL 40 // 40 * 10ms ticks
#define FLASHER_BLINK_PERIOD_FAULT 20  // 20 * 10ms ticks

typedef enum {
  FLASHER_OFF,
  FLASHER_HAZARD,
  FLASHER_L,
  FLASHER_R,
} FlasherState_t;

typedef struct {
  PinState leftSw;
  PinState leftSwPrev; // {left,right}SwPrev is used to detect a new command
  PinState rightSw;
  PinState rightSwPrev;
  PinState hazardSw;
  PinState hazardSwPrev;
  uint16_t leftLampCurr;
  uint16_t rightLampCurr;
} FlasherInput_t;

static uint8_t flasherBlinkPeriod[2] = {
    FLASHER_BLINK_PERIOD_NORMAL,
    FLASHER_BLINK_PERIOD_NORMAL}; // [0]: Left Flasher [1]: Right Flasher
static uint8_t blinkCnt = 0;      // Used to enforce minimum blink count
static uint8_t toggleHazardState = 0;
static uint8_t reqNewCmd[2] = {0, 0}; // [0]: Left Flasher [1]: Right Flasher
static FlasherState_t flasherState = FLASHER_OFF;
static FlasherState_t flasherStatePrev = FLASHER_OFF;
static PinState flasherCmds[2] = {OFF,
                                  OFF}; // [0]: Left Flasher [1]: Right Flasher
static FlasherInput_t flasherInputs = {.leftSw = OFF,
                                       .leftSwPrev = OFF,
                                       .rightSw = OFF,
                                       .rightSwPrev = OFF,
                                       .hazardSw = OFF,
                                       .hazardSwPrev = OFF,
                                       .leftLampCurr = 0,
                                       .rightLampCurr = 0};
/**
 * @brief Checks lamps for ShortCircuit or OpenCircuit events based on how much
 * current they draw. Each lamp is handled seperately so if one lamp fails, the
 * other works as expected. It does not affect the hazard flasher.
 * This functions writes to the debug port the first time it detects a fault.
 */
static void checkLampHealth(void) {
  static uint8_t diagSent[2] = {0}; // [0]: Left Flasher [1]: Right Flasher

  /* Check Left Lamp*/
  if (flasherInputs.leftLampCurr < LAMP_HEALTH_LT) {
    // Open Circuit
    flasherBlinkPeriod[0] = FLASHER_BLINK_PERIOD_FAULT;

    if (!diagSent[0]) {
      diagSent[0] = 1;
      printf("FLHOC\r\n");
    }
  } else if (flasherInputs.leftLampCurr > LAMP_HEALTH_UT) {
    // Short Circuit
    flasherCmds[0] = 0;

    // Avoids a bug where you need to switch leftSw twice for it to take effect
    if (flasherState == FLASHER_L) {
      flasherState = FLASHER_OFF;
      flasherStatePrev = FLASHER_OFF;
      reqNewCmd[0] = 1;
      blinkCnt = 0;
    }

    if (!diagSent[0]) {
      diagSent[0] = 1;
      printf("FLHSC\r\n");
    }
  } else {
    // Healthy
    flasherBlinkPeriod[0] = FLASHER_BLINK_PERIOD_NORMAL;
    diagSent[0] = 0;
  }

  /* Check Left Lamp*/
  if (flasherInputs.rightLampCurr < LAMP_HEALTH_LT) {
    // Open Circuit
    flasherBlinkPeriod[1] = FLASHER_BLINK_PERIOD_FAULT;
    if (!diagSent[1]) {
      diagSent[1] = 1;
      printf("FRHOC\r\n");
    }
  } else if (flasherInputs.rightLampCurr > LAMP_HEALTH_UT) {
    // Short Circuit
    flasherCmds[1] = 0;

    // Avoids a bug where you need to switch rightSw twice for it to take effect
    if (flasherState == FLASHER_R) {
      flasherState = FLASHER_OFF;
      flasherStatePrev = FLASHER_OFF;
      reqNewCmd[1] = 1;
      blinkCnt = 0;
    }

    if (!diagSent[1]) {
      diagSent[1] = 1;
      printf("FRHSC\r\n");
    }
  } else {
    // Healthy
    flasherBlinkPeriod[1] = FLASHER_BLINK_PERIOD_NORMAL;
    diagSent[1] = 0;
  }
}

/**
 * @brief Checks battery voltage and turns off the output in case an OverVoltage
 * or UnderVoltage event is detected. This only applies to left and right
 * flashers and does not affect the hazard flasher
 */
static void checkBattery(void) {
  if (batteryStatus == BATTERY_OV) {
    flasherCmds[0] = OFF;
    flasherCmds[1] = OFF;
  } else if (batteryStatus == BATTERY_UV) {
    flasherCmds[0] = OFF;
    flasherCmds[1] = OFF;

    flasherState = FLASHER_OFF;
    flasherStatePrev = FLASHER_OFF;
    reqNewCmd[0] = 1;
    reqNewCmd[1] = 1;
    blinkCnt = 0;
  }
}

void Flasher_Init(void) {
  HAL_GPIO_WritePin(FLASHER_L_GPIO_Port, FLASHER_L_Pin, STATE2GPIO(OFF, AH));
  HAL_GPIO_WritePin(FLASHER_R_GPIO_Port, FLASHER_R_Pin, STATE2GPIO(OFF, AL));
}

void Flasher_ReadInput(void) {
  // Each pin has it's own debounce state in order to avoid rewriting debounce
  // function for each pin
  static GPIO_PinState input = 0;
  static Debounce_t leftSwDb = {.swStable = STATE2GPIO(OFF, AL), .counter = 0};
  static Debounce_t rightSwDb = {.swStable = STATE2GPIO(OFF, AL), .counter = 0};
  static Debounce_t hazardSwDb = {.swStable = STATE2GPIO(OFF, AH),
                                  .counter = 0};

  input = HAL_GPIO_ReadPin(SW_FLASHER_L_GPIO_Port, SW_FLASHER_L_Pin);
  flasherInputs.leftSwPrev = flasherInputs.leftSw;
  flasherInputs.leftSw = GPIO2STATE(debounce(input, 5, &leftSwDb), AL);

  input = HAL_GPIO_ReadPin(SW_FLASHER_R_GPIO_Port, SW_FLASHER_R_Pin);
  flasherInputs.rightSwPrev = flasherInputs.rightSw;
  flasherInputs.rightSw = GPIO2STATE(debounce(input, 5, &rightSwDb), AL);

  input = HAL_GPIO_ReadPin(SW_HAZARD_GPIO_Port, SW_HAZARD_Pin);
  flasherInputs.hazardSw = GPIO2STATE(debounce(input, 5, &hazardSwDb), AH);

  flasherInputs.leftLampCurr = adcValues.leftFlasherCurr;
  flasherInputs.rightLampCurr = adcValues.rightFlasherCurr;

  /* Implement latching for hazard switch */
  if (flasherInputs.hazardSwPrev == OFF && flasherInputs.hazardSw == ON) {
    // Switch pressed
    toggleHazardState = 1;
    flasherInputs.hazardSwPrev = flasherInputs.hazardSw;
  } else if (flasherInputs.hazardSwPrev == ON &&
             flasherInputs.hazardSw == OFF) {
    // Switch released
    flasherInputs.hazardSwPrev = flasherInputs.hazardSw;
  }
}

void Flasher_Update(void) {
  static uint8_t flasherTimer = 0; // Used to enforce blink period

  if (reqNewCmd[0] || reqNewCmd[1]) {
    // If newCmd is required, check if switches have been turn off or not
    if (flasherInputs.leftSwPrev == ON && flasherInputs.leftSw == OFF) {
      reqNewCmd[0] = 0;
    }
    if (flasherInputs.rightSwPrev == ON && flasherInputs.rightSw == OFF) {
      reqNewCmd[1] = 0;
    }
  }

  /*
   * Main state machine:
   * In each state, first the output suitable for that state is set.
   * Then we determine the next state based on the swtich inputs, whether the
   * minimum blink count has been reached or not and if newCmd is required or
   * not.
   * To implement the priority functionality (last input always has a higher
   * priority), in order to avoid unnecessary complications, we first go back
   * to FLASHER_OFF for a brief moment before switching to the desired state.
   * This fixes the desynchronization issue between lamps and also avoids
   * rewriting simliar code in each state.
   * This is achieved by using a second flasherState variable
   * (flasherStatePrev) to track if we went from OFF state to either ON states
   * or not. If not, then when last state's switch is turned off, we go back
   * to the previous state. Take notice to where flasherStatePrev is updated
   * in each state.
   * For example activate left flasher in hazard state:
   * OFF -> HZ -> OFF -> L (lSw off) L -> OFF -> HZ (hzSw off) HZ -> OFF
   */
  switch (flasherState) {
  case FLASHER_OFF:
    flasherCmds[0] = OFF;
    flasherCmds[1] = OFF;

    if (flasherStatePrev == FLASHER_OFF) {
      if (flasherInputs.leftSw && !reqNewCmd[0]) {
        flasherState = FLASHER_L;
      } else if (flasherInputs.rightSw && !reqNewCmd[1]) {
        flasherState = FLASHER_R;
      } else if (toggleHazardState) {
        toggleHazardState = 0;
        flasherState = FLASHER_HAZARD;
      }
    }
    // These 3 if-statements handle going back to the previous state
    else if (flasherStatePrev == FLASHER_HAZARD) {
      if (flasherInputs.leftSw) {
        flasherState = FLASHER_L;
      } else if (flasherInputs.rightSw) {
        flasherState = FLASHER_R;
      } else {
        flasherState = flasherStatePrev;
        flasherStatePrev = FLASHER_OFF;
      }
    } else if (flasherStatePrev == FLASHER_L) {
      if (flasherInputs.rightSw) {
        flasherState = FLASHER_R;
      } else if (toggleHazardState) {
        toggleHazardState = 0;
        flasherState = FLASHER_HAZARD;
      } else {
        flasherState = flasherStatePrev;
        flasherStatePrev = FLASHER_OFF;
      }
    } else if (flasherStatePrev == FLASHER_R) {
      if (flasherInputs.leftSw) {
        flasherState = FLASHER_L;
      } else if (toggleHazardState) {
        toggleHazardState = 0;
        flasherState = FLASHER_HAZARD;
      } else {
        flasherState = flasherStatePrev;
        flasherStatePrev = FLASHER_OFF;
      }
    }
    break;

  case FLASHER_HAZARD:
    if (++flasherTimer >= FLASHER_BLINK_PERIOD_NORMAL) {
      flasherTimer = 0;
      flasherCmds[0] ^= ON;
      flasherCmds[1] ^= ON;
      blinkCnt += flasherCmds[0];
    }

    // Determine the next state
    if (flasherStatePrev == FLASHER_OFF) {
      if (toggleHazardState && blinkCnt > MIN_FLASHER_BLINK) {
        toggleHazardState = 0;
        blinkCnt = 0;
        flasherState = FLASHER_OFF;
      } else if (flasherInputs.leftSw && !reqNewCmd[0]) {
        blinkCnt = 0;
        flasherState = FLASHER_OFF;
        flasherStatePrev = FLASHER_HAZARD;
      } else if (flasherInputs.rightSw && !reqNewCmd[1]) {
        blinkCnt = 0;
        flasherState = FLASHER_OFF;
        flasherStatePrev = FLASHER_HAZARD;
      }
    } else {
      // I seperated this from `if (toggleHazardState && blinkCnt >
      // MIN_FLASHER_BLINK)` to bypass the minimum blink count when the previous
      // state is not FLASHER_OFF
      if (toggleHazardState) {
        toggleHazardState = 0;
        flasherState = FLASHER_OFF;
      }
    }
    break;

  case FLASHER_L:
    if (++flasherTimer >= flasherBlinkPeriod[0]) {
      flasherTimer = 0;
      flasherCmds[0] ^= ON;
      blinkCnt += flasherCmds[0];
    }
    flasherCmds[1] = OFF;

    // Determine the next state
    if (flasherStatePrev == FLASHER_OFF) {
      if (!flasherInputs.leftSw && blinkCnt > MIN_FLASHER_BLINK) {
        blinkCnt = 0;
        flasherState = FLASHER_OFF;
      } else if (flasherInputs.rightSw && !reqNewCmd[1]) {
        blinkCnt = 0;
        flasherState = FLASHER_OFF;
        flasherStatePrev = FLASHER_L;
      } else if (toggleHazardState) {
        blinkCnt = 0;
        flasherState = FLASHER_OFF;
        flasherStatePrev = FLASHER_L;
      }
    } else {
      if (!flasherInputs.leftSw) {
        blinkCnt = 0;
        flasherState = FLASHER_OFF;
      }
    }
    break;

  case FLASHER_R:
    if (++flasherTimer >= flasherBlinkPeriod[1]) {
      flasherTimer = 0;
      flasherCmds[1] ^= ON;
      blinkCnt += flasherCmds[1];
    }
    flasherCmds[0] = OFF;

    // Determine the next state
    if (flasherStatePrev == FLASHER_OFF) {
      if (!flasherInputs.rightSw && blinkCnt > MIN_FLASHER_BLINK) {
        blinkCnt = 0;
        flasherState = FLASHER_OFF;
      } else if (flasherInputs.leftSw && !reqNewCmd[0]) {
        blinkCnt = 0;
        flasherState = FLASHER_OFF;
        flasherStatePrev = FLASHER_R;
      } else if (toggleHazardState) {
        blinkCnt = 0;
        flasherState = FLASHER_OFF;
        flasherStatePrev = FLASHER_R;
      }
    } else {
      if (!flasherInputs.rightSw) {
        blinkCnt = 0;
        flasherState = FLASHER_OFF;
      }
    }
    break;
  }

  // Hazard flasher bypasses battery and lamp fault conditions
  if (flasherState != FLASHER_HAZARD) {
    checkLampHealth();
    checkBattery();
  }
}

void Flasher_WriteOutput(void) {
  HAL_GPIO_WritePin(FLASHER_L_GPIO_Port, FLASHER_L_Pin,
                    flasherCmds[0] ? STATE2GPIO(ON, AH) : STATE2GPIO(OFF, AH));
  HAL_GPIO_WritePin(FLASHER_R_GPIO_Port, FLASHER_R_Pin,
                    flasherCmds[1] ? STATE2GPIO(ON, AL) : STATE2GPIO(OFF, AL));
}
