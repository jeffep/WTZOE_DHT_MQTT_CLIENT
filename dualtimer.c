#include "w7500x_dualtimer.h"

static void DUALTIMER_Config(void)
{
    DUALTIMER_InitTypDef DUALTIMER_InitStructure;
    //NVIC_InitTypeDef NVIC_InitStructure;
    DUALTIMER_InitStructure.Timer_Load = GetSystemClock() / 3000000; //10us for system at 32,000,000
    DUALTIMER_InitStructure.Timer_Prescaler = DUALTIMER_Prescaler_1;
    DUALTIMER_InitStructure.Timer_Wrapping = DUALTIMER_Free_Running; // continuously
    DUALTIMER_InitStructure.Timer_Repetition = DUALTIMER_Wrapping;  // continuously countdown
    DUALTIMER_InitStructure.Timer_Size = DUALTIMER_Size_32;
    DUALTIMER_Init(DUALTIMER0_0, &DUALTIMER_InitStructure);

    DUALTIMER_ITConfig(DUALTIMER0_0, ENABLE);

    DUALTIMER_Cmd(DUALTIMER0_0, ENABLE);
}

void wait_us(uint32_t microseconds) {
    uint32_t timeout_us = 5000000; // 5 second timeout
    //DEBUG_PRINT("In wait_us, waiting for %lu", microseconds);
    // Clear any pending interrupts 
    DUALTIMER_ClearIT(DUALTIMER0_0);
    DUALTIMER_ITConfig(DUALTIMER0_0, ENABLE);
    DUALTIMER_Cmd(DUALTIMER0_0, ENABLE);

    //Dualtimer0 counts down from 10 microseconds and resets repeatedly
    //Everytime it counts to zero, we subtract 10 microseconds from the wait time

    // Wait until the total time elapses
    while ((microseconds > 0) && ((timeout_us > 0) && (timeout_us <= 5000))) {
        // Wait for the next interrupt
        while ((DUALTIMER_GetITStatus(DUALTIMER0_0) != SET) && (timeout_us > 0)) {
            //DEBUG_PRINT("Dualtimer not done. timeout=%lu",timeout_us);
            timeout_us -= 10;
        }

        // Clear the interrupt flag
        DUALTIMER_ClearIT(DUALTIMER0_0);

        // Decrement the remaining time
        microseconds -= 10;
        timeout_us -= 10;
    }
    if (timeout_us <=0) printf("Error dualtimer didn't work on %d", microseconds);
}

void wait_ms(uint32_t milliseconds) {
    DEBUG_PRINT("In wait_ms");
    uint32_t microseconds = milliseconds * 1000;
    wait_us(microseconds);
}

void wait_s(uint32_t seconds) {
    uint32_t microseconds = seconds * 1000000;
    //DEBUG_PRINT("In wait_s(%d)",seconds);
    wait_us(microseconds);
}