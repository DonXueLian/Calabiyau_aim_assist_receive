#ifndef __PD_CONTROLLER_H__
#define __PD_CONTROLLER_H__

#include <stdint.h>

extern uint8_t kp;
extern uint8_t kd;

void PD_controller(int16_t input_x, int16_t input_y, int8_t * output_x, int8_t * output_y);

#endif