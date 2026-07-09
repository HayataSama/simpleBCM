#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <stdint.h>

int16_t createTask(void (*func)(void), uint32_t period);
void scheduler(void);

#endif