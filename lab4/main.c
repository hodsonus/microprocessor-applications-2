#include "G8RTOS/G8RTOS.h"
#include "Threads.h"
#include "msp.h"

/* ---------------------------------------- MAIN ---------------------------------------- */

#define USING_TP true

/**
 * main.c
 */
void main(void)
{
    G8RTOS_Init(USING_TP);

    // initialize the GPIO pins used in the threads
    P4->DIR |= BIT5 | BIT7;
    P5->DIR |= BIT4;

    // initialize our FIFOs
    G8RTOS_InitFIFO(JOYSTICK_FIFO);
    G8RTOS_InitFIFO(TEMP_FIFO);
    G8RTOS_InitFIFO(LIGHT_FIFO);

    // initialize our semaphores
    G8RTOS_InitSemaphore(&led_mutex, 1);
    G8RTOS_InitSemaphore(&sensor_mutex, 1);

    // add our normal threads
    G8RTOS_AddThread(&thread0,0,"thread0");
    G8RTOS_AddThread(&thread1,0,"thread1");
    G8RTOS_AddThread(&thread2,0,"thread2");
    G8RTOS_AddThread(&thread3,0,"thread3");
    G8RTOS_AddThread(&thread4,0,"thread4");
    G8RTOS_AddThread(&thread5,0,"thread5");

    // add our periodic events
    G8RTOS_AddPeriodicEvent(&pthread0, 100);
    G8RTOS_AddPeriodicEvent(&pthread1, 1000);

    // and launch the OS!
    G8RTOS_Launch();
}

/* ---------------------------------------- MAIN ---------------------------------------- */
