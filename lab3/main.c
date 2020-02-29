#include <G8RTOS/G8RTOS.h>
#include "Threads.h"

/* ---------------------------------------- MAIN ---------------------------------------- */

/**
 * main.c
 */
void main(void)
{
    G8RTOS_Init();

    // TODO - init GPIO ports

    G8RTOS_AddThread(&thread0);
    G8RTOS_AddThread(&thread1);
    G8RTOS_AddThread(&thread2);
    G8RTOS_AddThread(&thread3);
    G8RTOS_AddThread(&thread4);
    G8RTOS_AddThread(&thread5);

    G8RTOS_AddPeriodicEvent(&pthread0, 100);
    G8RTOS_AddPeriodicEvent(&pthread1, 1000);

    G8RTOS_Launch();
}

/* ---------------------------------------- MAIN ---------------------------------------- */
