#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <stdint.h>

#define SLOT_COUNT 10
#define TASK_PER_SLOT 10

typedef struct {
  void (*func)(void);
  uint32_t period; // ms
  uint32_t count;
} Task_t;

int16_t createTask(void (*func)(void), uint32_t period);
void scheduler(void);

#endif