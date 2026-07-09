#include "battery.h"
#include "adc.h"
#include "util.h"
#include <stdint.h>

/*
 * Voltage Thresholds:
 * Voltage > 17V -> OverVoltage
 * Voltage < 9.2V -> UnderVoltage
 * Voltage > 9.8V -> UnderVoltage Recovery
 *
 * I don't have 17V supply so I divided thresholds by 10: 1.7V, 0.92V and 0.98V
 * ADC_Value = (Voltage * 2^12) / VDDA
 */

#define OV_THRSH MV2ADC(1700)  // ~1.7V
#define UV_THRSH_H MV2ADC(980) // ~0.98V
#define UV_THRSH_L MV2ADC(920) // ~0.92V

typedef enum { STABLE, FALLING, RISING } VoltageTrend_t;

static uint32_t adcValue;

BatteryStatus_t batteryStatus = BATTERY_OK;

void Battery_Init(void) {}

void Battery_ReadInput(void) { adcValue = adcValues.battery; }

void Battery_Update(void) {
  if (adcValue > OV_THRSH) {
    batteryStatus = BATTERY_OV;
  } else if (adcValue < UV_THRSH_L) {
    batteryStatus = BATTERY_UV;
  } else if (adcValue > UV_THRSH_H && batteryStatus == BATTERY_UV) {
    batteryStatus = BATTERY_OK;
  } else {
    batteryStatus = BATTERY_OK;
  }
}