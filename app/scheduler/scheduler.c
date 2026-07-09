#include "scheduler.h"
#include "tim.h"
#include <stdint.h>

#define SLOT_COUNT 10
#define TASK_PER_SLOT 10

typedef struct {
  void (*func)(void);
  uint32_t period; // In milliseconds
  uint32_t count;
} Task_t;

Task_t taskTable[SLOT_COUNT][TASK_PER_SLOT]; // 2D array of all tasks
static uint8_t taskIndex[SLOT_COUNT] = {0};  // Number of tasks in each slot

void scheduler(void) {
  static uint8_t slotIndex = 0;

  if (tick) {
    tick = 0;

    Task_t *slotTasks = taskTable[slotIndex]; // Select an slot
    uint8_t taskCnt =
        taskIndex[slotIndex]; // Number of tasks inside selected slot

    for (uint8_t i = 0; i < taskCnt; i++) {
      Task_t *task = &slotTasks[i]; // ith task in selected slot
      if (task->func) {
        if (++task->count >= task->period) {
          task->func();
          task->count = 0;
        }
      }
    }
    slotIndex = (slotIndex + 1) % SLOT_COUNT; // Go to next slot
  }
}

int16_t createTask(void (*func)(void), uint32_t period) {
  static uint8_t slotIndex = 0;

  for (uint8_t i = 0; i < SLOT_COUNT; i++) {
    slotIndex = (slotIndex + i) % SLOT_COUNT;

    if (taskIndex[slotIndex] < TASK_PER_SLOT) {
      // Create a new task in selectd slot and index
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
