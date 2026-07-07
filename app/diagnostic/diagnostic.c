#include "adc.h"
#include <stdio.h>

void diagnostic(void) {
  printf("ADC ch6: %u  "
         "ADC ch7: %u  "
         "ADC ch8: %u  "
         "ADC ch9: %u"
         "\r\n",
         adcValues.battery, adcValues.gasTankPressure,
         adcValues.rightFlasherCurr, adcValues.leftFlasherCurr);
}