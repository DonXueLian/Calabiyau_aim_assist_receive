#ifndef __MOUSE_IMITATE_H__
#define __MOUSE_IMITATE_H__

#include <stdint.h>

void mouse_init(void);
void mouse_report(uint8_t buttoms, int8_t delta_x, int8_t delta_y, uint8_t is_spi, int8_t whirl);

#endif
