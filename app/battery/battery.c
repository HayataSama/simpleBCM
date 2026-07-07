#include "battery.h"
#include "adc.h"
#include <stdint.h>

/*
  Voltage Thresholds:
  Voltage > 17V -> over-voltage
  Voltage < 9.2V -> under-voltage
  Voltage > 9.8V -> under-voltage recovery

  I don't have 17V supply so I divided thresholds by 10: 1.7V, 0.92V, 0.98V
  ADC_Value = (Voltage * 2^12) / VDDA
*/

#define OV_THRSH 2110   // ~1.7V
#define UV_THRSH_H 1216 // ~0.98V
#define UV_THRSH_L 1140 // ~0.92V
#define VTREND_HYST 5   // ~0.004V

typedef enum { STABLE, FALLING, RISING } VoltageTrend_t;

static uint32_t adcValue;
static uint32_t adcValuePrev;
static VoltageTrend_t vTrend = STABLE;

BatteryStatus_t batteryStatus = BATTERY_OK;

void Battery_Init(void) {}

void Battery_ReadInput(void) { adcValue = adcValues.ch6; }

void Battery_Update(void) {
  // FIXME: This implementation of rising and falling edge is kinda stupid
  if (adcValue > (adcValuePrev + VTREND_HYST)) {
    vTrend = RISING;
  } else if (adcValue < (adcValuePrev - VTREND_HYST)) {
    vTrend = FALLING;
  } else {
    vTrend = STABLE;
  }

  if (adcValue > OV_THRSH) {
    batteryStatus = BATTERY_OV;
  } else if (adcValue < UV_THRSH_L && vTrend == FALLING) {
    batteryStatus = BATTERY_UV;
  } else if (adcValue > UV_THRSH_H && vTrend == RISING &&
             batteryStatus == BATTERY_UV) {
    batteryStatus = BATTERY_OK;
  } else if (batteryStatus != BATTERY_UV) {
    batteryStatus = BATTERY_OK;
  }
}