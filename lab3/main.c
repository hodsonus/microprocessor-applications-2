#include <G8RTOS/G8RTOS.h>
#include "Threads.h"

/* ---------------------------------------- MAIN ---------------------------------------- */

/**
 * main.c
 */
void main(void)
{
    // initialize all components on the board
    G8RTOS_Init();

    // initialize the GPIO pins used in the threads
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0 |
                                      GPIO_PIN1 |
                                      GPIO_PIN2);

    // initialize our FIFOs
    G8RTOS_InitFIFO(JOYSTICK_FIFO);
    G8RTOS_InitFIFO(TEMP_FIFO);
    G8RTOS_InitFIFO(LIGHT_FIFO);

    // initialize our semaphores
    G8RTOS_InitSemaphore(&led_mutex, 1);
    G8RTOS_InitSemaphore(&sensor_mutex, 1);

    // add our normal threads
    G8RTOS_AddThread(&thread0);
    G8RTOS_AddThread(&thread1);
    G8RTOS_AddThread(&thread2);
    G8RTOS_AddThread(&thread3);
    G8RTOS_AddThread(&thread4);
    G8RTOS_AddThread(&thread5);

    // add our periodic events
    G8RTOS_AddPeriodicEvent(&pthread0, 100);
    G8RTOS_AddPeriodicEvent(&pthread1, 1000);

    // and launch the OS!
    G8RTOS_Launch();
}

/* ---------------------------------------- MAIN ---------------------------------------- */
