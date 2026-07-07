#include "fuel.h"
#include "adc.h"
#include "main.h"
#include "tim.h"
#include "util.h"
#include <stdint.h>

#define GAS_BLINK_PERIOD (400 / 10) // 400ms with 10ms task period
#define ENGINE_RPM_THRSH 2400
#define ENGINE_RPM_MIN 600

// 1/2 resistor divider is used on pressure gauge output
#define GAS_TANK_P0 MV2ADC(0)        // p <= 30psi
#define GAS_TANK_P1 MV2ADC(1000 / 2) // 30 < p <= 60
#define GAS_TANK_P2 MV2ADC(2000 / 2) // 60 < p <= 90
#define GAS_TANK_P3 MV2ADC(3000 / 2) // 90 < p <= 140
#define GAS_TANK_P4 MV2ADC(4000 / 2) // P > 140

typedef enum { PETROL, GAS_STOP, GAS_RUN } FuelState_t;
typedef struct {
  PinState gasValve;
  PinState gasInd;
  PinState petrolInd;
  PinState f1;
  PinState f2;
  PinState f3;
  PinState f4;
} FuelCmds;

static FuelState_t fuelState = PETROL;
static uint8_t toggleState = 0;
static uint8_t gasIndTimer = 0;
static uint16_t engineRPM = 0;
static uint16_t gasTankPressure = 0;
static FuelCmds fuelCmds = {.gasValve = OFF,
                            .gasInd = OFF,
                            .petrolInd = ON,
                            .f1 = OFF,
                            .f2 = OFF,
                            .f3 = OFF,
                            .f4 = OFF};

static void setGasGaugeCmds(void) {
  // This implementation is a bit different from what is reqired.
  // I changed it to suit the parts that I have on hand.
  if (gasTankPressure < GAS_TANK_P1) {
    fuelCmds.f1 = OFF;
    fuelCmds.f2 = OFF;
    fuelCmds.f3 = OFF;
    fuelCmds.f4 = OFF;
  } else if (gasTankPressure > GAS_TANK_P1 && gasTankPressure < GAS_TANK_P2) {
    fuelCmds.f1 = ON;
    fuelCmds.f2 = OFF;
    fuelCmds.f3 = OFF;
    fuelCmds.f4 = OFF;
  } else if (gasTankPressure > GAS_TANK_P2 && gasTankPressure < GAS_TANK_P3) {
    fuelCmds.f1 = ON;
    fuelCmds.f2 = ON;
    fuelCmds.f3 = OFF;
    fuelCmds.f4 = OFF;
  } else if (gasTankPressure > GAS_TANK_P3 && gasTankPressure < GAS_TANK_P4) {
    fuelCmds.f1 = ON;
    fuelCmds.f2 = ON;
    fuelCmds.f3 = ON;
    fuelCmds.f4 = OFF;
  } else if (gasTankPressure > GAS_TANK_P4) {
    fuelCmds.f1 = ON;
    fuelCmds.f2 = ON;
    fuelCmds.f3 = ON;
    fuelCmds.f4 = ON;
  }
}

void Fuel_Init(void) {
  HAL_TIM_IC_Start_IT(&htim15, TIM_CHANNEL_2);

  HAL_GPIO_WritePin(GAS_VALVE_GPIO_Port, GAS_VALVE_Pin,
                    fuelCmds.gasValve ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GAS_IND_GPIO_Port, GAS_IND_Pin,
                    fuelCmds.gasInd ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(PETROL_IND_GPIO_Port, PETROL_IND_Pin,
                    fuelCmds.petrolInd ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(F1_GPIO_Port, F1_Pin,
                    fuelCmds.f1 ? GPIO_PIN_RESET : GPIO_PIN_SET);
  HAL_GPIO_WritePin(F2_GPIO_Port, F2_Pin,
                    fuelCmds.f2 ? GPIO_PIN_RESET : GPIO_PIN_SET);
  HAL_GPIO_WritePin(F3_GPIO_Port, F3_Pin,
                    fuelCmds.f3 ? GPIO_PIN_RESET : GPIO_PIN_SET);
  HAL_GPIO_WritePin(F4_GPIO_Port, F4_Pin,
                    fuelCmds.f4 ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void Fuel_ReadInput(void) {
  // TODO: update sw_gas type to PinState and change if statements
  // Also update debonce function.
  // Also create a macro that relates PinState and GPIO state given the pin and
  // LS or HS
  static GPIO_PinState gasSw = GPIO_PIN_SET;
  static GPIO_PinState gasSwPrev = GPIO_PIN_SET;
  static Debounce_t gasSwDb = {0};

  GPIO_PinState input = HAL_GPIO_ReadPin(SW_GAS_GPIO_Port, SW_GAS_Pin);
  gasSw = debounce(input, 5, &gasSwDb);

  if (gasSwPrev == GPIO_PIN_SET && gasSw == GPIO_PIN_RESET) {
    // Switch pressed
    toggleState = 1;
    gasSwPrev = gasSw;
  } else if (gasSwPrev == GPIO_PIN_RESET && gasSw == GPIO_PIN_SET) {
    // Switch released
    gasSwPrev = gasSw;
  }

  gasTankPressure = adcValues.gasTankPressure;
}

void Fuel_Update(void) {
  switch (fuelState) {
  case PETROL:
    fuelCmds.petrolInd = ON;
    fuelCmds.gasValve = OFF;
    fuelCmds.gasInd = OFF;
    fuelCmds.f1 = OFF;
    fuelCmds.f2 = OFF;
    fuelCmds.f3 = OFF;
    fuelCmds.f4 = OFF;

    if (toggleState) {
      toggleState = 0;
      fuelState = GAS_STOP;
    }
    break;

  case GAS_STOP:
    if (++gasIndTimer >= GAS_BLINK_PERIOD) {
      gasIndTimer = 0;
      fuelCmds.gasInd ^= ON;
    }
    fuelCmds.petrolInd = OFF;
    fuelCmds.gasValve = OFF;
    setGasGaugeCmds();

    if (toggleState) {
      toggleState = 0;
      fuelState = PETROL;
    } else if (engineRPM >= ENGINE_RPM_THRSH) {
      fuelState = GAS_RUN;
    }
    break;

  case GAS_RUN:
    fuelCmds.petrolInd = OFF;
    fuelCmds.gasValve = ON;
    fuelCmds.gasInd = ON;
    setGasGaugeCmds();

    if (toggleState) {
      toggleState = 0;
      fuelState = PETROL;
    } else if (engineRPM <= ENGINE_RPM_MIN) {
      fuelState = GAS_STOP;
    }
    break;
  }
}

void Fuel_WriteOutput(void) {
  HAL_GPIO_WritePin(GAS_VALVE_GPIO_Port, GAS_VALVE_Pin,
                    fuelCmds.gasValve ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GAS_IND_GPIO_Port, GAS_IND_Pin,
                    fuelCmds.gasInd ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(PETROL_IND_GPIO_Port, PETROL_IND_Pin,
                    fuelCmds.petrolInd ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(F1_GPIO_Port, F1_Pin,
                    fuelCmds.f1 ? GPIO_PIN_RESET : GPIO_PIN_SET);
  HAL_GPIO_WritePin(F2_GPIO_Port, F2_Pin,
                    fuelCmds.f2 ? GPIO_PIN_RESET : GPIO_PIN_SET);
  HAL_GPIO_WritePin(F3_GPIO_Port, F3_Pin,
                    fuelCmds.f3 ? GPIO_PIN_RESET : GPIO_PIN_SET);
  HAL_GPIO_WritePin(F4_GPIO_Port, F4_Pin,
                    fuelCmds.f4 ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void readRPM(void) {
  static uint32_t edge1;
  static uint32_t edge2;
  static uint8_t isFirstCapture = 1;
  static uint32_t period;

  if (rpmReady) {
    rpmReady = 0;
    if (isFirstCapture) {
      isFirstCapture = 0;
      edge1 = capture;
    } else {
      edge2 = capture;
      if (edge2 >= edge1) {
        period = edge2 - edge1;
      } else {
        // Timer overflowed
        period = (0xFFFF - edge1 + 1) + edge2;
      }
      edge1 = edge2;
    }

    engineRPM = (1000000 * 60) / period;

    HAL_TIM_IC_Start_IT(&htim15, TIM_CHANNEL_2);
  }
}
