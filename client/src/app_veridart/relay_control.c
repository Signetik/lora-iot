#include <zephyr.h>
#include <stdint.h>
#include "relay_control.h"


#define RELAY_PIN    PIN_PB30 

struct k_sem relay_mutex;

static uint8_t is_active;    

void relay_on()
{
    k_sem_take(&relay_mutex, K_FOREVER);
#ifdef STEVE
        ioport_set_pin_level(RELAY_PIN, IOPORT_PIN_LEVEL_HIGH);
#endif
        is_active = 1;
    k_sem_give(&relay_mutex);
}

void relay_off()
{
    k_sem_take(&relay_mutex, K_FOREVER);
#ifdef STEVE
        ioport_set_pin_level(RELAY_PIN, IOPORT_PIN_LEVEL_LOW);
#endif
        is_active = 0;
    k_sem_give(&relay_mutex);
}

uint8_t relay_is_active()
{
    uint8_t active;

    k_sem_take(&relay_mutex, K_FOREVER);
        active = is_active;
    k_sem_give(&relay_mutex);

    return active;
}

//This function must be called first before using other relay functions
void relay_init()
{
#ifdef STEVE
	ioport_set_pin_dir(RELAY_PIN,IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(RELAY_PIN, IOPORT_PIN_LEVEL_LOW);
#endif
	
	k_sem_init(&relay_mutex, 0, 1);
}
