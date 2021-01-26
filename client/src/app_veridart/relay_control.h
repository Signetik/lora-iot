#ifndef __RELAY_CONTROL_H
#define __RELAY_CONTROL_H

#include <stdint.h>

void relay_on(void);
void relay_off(void);
uint8_t relay_is_active(void);
void relay_init(void);


#endif /* __RELAY_CONTROL_H */