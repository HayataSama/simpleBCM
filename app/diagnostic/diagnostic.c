#include "adc.h"
#include <stdio.h>

void diagnostic(void) {
  printf("battery: %u  "
         "gasTank: %u  "
         "leftFlasher: %u  "
         "rightFlasher: %u"
         "\r\n",
         adcValues.battery, adcValues.gasTankPressure,
         adcValues.leftFlasherCurr, adcValues.rightFlasherCurr);
}