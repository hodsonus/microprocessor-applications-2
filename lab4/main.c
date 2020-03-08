#include <stdlib.h>
#include <time.h>
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

    /* Can alternatively use TLV->RANDOM_NUM_1 for repeatable color and
     * velocity generation. */
    srand(time(NULL));

    // initialize our FIFOs
    G8RTOS_InitFIFO(COORD_FIFO);

    // initialize the semaphore protecting the LCD
    G8RTOS_InitSemaphore(&lcd_mutex, 1);

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
