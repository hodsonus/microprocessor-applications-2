#include "G8RTOS/G8RTOS.h"
#include "Balls.h"
#include "msp.h"

/* ---------------------------------------- MAIN ---------------------------------------- */

#define USING_TP true

/**
 * main.c
 */
void main(void)
{
    G8RTOS_Init(USING_TP);

    // initialize our FIFOs
    G8RTOS_InitFIFO(COORD_FIFO);

    // add our normal threads
    G8RTOS_AddThread(&ReadAccelerometer, ACCEL_THREAD_PRIORITY, "accelerometer");
    G8RTOS_AddThread(&LCDTappedWorker,   WORKER_PRIORITY,       "worker");
    G8RTOS_AddThread(&Idle,              255,                   "idle");

    // add the aperiodic event
    G8RTOS_AddAperiodicEvent(&LCDTapHandler, 4, PORT4_IRQn);

    // and launch the OS!
    G8RTOS_Launch();
}

/* ---------------------------------------- MAIN ---------------------------------------- */
