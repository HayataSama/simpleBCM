#include "scheduler.h"
#include "tim.h"
#include <stdint.h>

Task_t taskTable[SLOT_COUNT][TASK_PER_SLOT];
static uint8_t taskIndex[SLOT_COUNT] = {0};

void scheduler(void) {
  if (tick) {
    tick = 0;

    static uint8_t slotIndex = 0;
    Task_t *slotTasks = taskTable[slotIndex];
    uint8_t taskCount = taskIndex[slotIndex];

    for (uint8_t i = 0; i < taskCount; i++) {
      Task_t *task = &slotTasks[i];
      if (task->func) {
        if (++task->count >= task->period) {
          task->func();
          task->count = 0;
        }
      }
    }
    slotIndex = (slotIndex + 1) % SLOT_COUNT;
  }
}

int16_t createTask(void (*func)(void), uint32_t period) {
  static uint8_t slotIndex = 0;

  for (uint8_t i = 0; i < SLOT_COUNT; i++) {
    slotIndex = (slotIndex + i) % SLOT_COUNT;

    if (taskIndex[slotIndex] < TASK_PER_SLOT) {
      taskTable[slotIndex][taskIndex[slotIndex]].func = func;
      taskTable[slotIndex][taskIndex[slotIndex]].period = period / SLOT_COUNT;
      taskTable[slotIndex][taskIndex[slotIndex]].count = 0;

      taskIndex[slotIndex]++;
      slotIndex++;

      return slotIndex - 1; // return current slot
    }
  }

  return -1; // taskTable is filled
}
