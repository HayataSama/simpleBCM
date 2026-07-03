#ifndef __BATTERY_H__
#define __BATTERY_H__

typedef enum { BATTERY_OK, BATTERY_OV, BATTERY_UV } BatteryStatus_t;

extern BatteryStatus_t batteryStatus;

void Battery_Init(void);
void Battery_ReadInput(void);
void Battery_Update(void);

#endif