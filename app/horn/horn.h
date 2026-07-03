#ifndef __HORN_H__
#define __HORN_H__

typedef enum { OFF, ON, EXPIRED, OV, UV } HornState_t;

void Horn_Init(void);
void Horn_ReadInput(void);
void Horn_Update(void);
void Horn_WriteOutput(void);

#endif