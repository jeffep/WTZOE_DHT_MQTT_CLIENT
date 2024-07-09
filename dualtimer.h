#include "w7500x_dualtimer.h"

static void DUALTIMER_Config(void);
void wait_us(uint32_t microseconds);
void wait_ms(uint32_t milliseconds);
void wait_s(uint32_t seconds);