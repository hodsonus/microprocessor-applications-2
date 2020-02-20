#include <G8RTOS/G8RTOS.h>
#include "Threads.h"

/* ---------------------------------------- MAIN ---------------------------------------- */

/**
 * main.c
 */
void main(void)
{
    G8RTOS_Init();

    G8RTOS_InitSemaphore(&led, 1);
    G8RTOS_InitSemaphore(&sensor, 1);

    G8RTOS_AddThread(&task0);
    G8RTOS_AddThread(&task1);
    G8RTOS_AddThread(&task2);

    G8RTOS_Launch();
}

/* ---------------------------------------- MAIN ---------------------------------------- */
