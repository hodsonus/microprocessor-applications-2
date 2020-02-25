#include <G8RTOS/G8RTOS.h>
#include "Threads.h"
#include "Quiz.h"

/* ---------------------------------------- MAIN ---------------------------------------- */

/**
 * main.c
 */
void main(void)
{
    G8RTOS_Init();

    // Lab Semaphores
    G8RTOS_InitSemaphore(&led, 1);
    G8RTOS_InitSemaphore(&sensor, 1);
    // Lab Threads
    G8RTOS_AddThread(&task0);
    G8RTOS_AddThread(&task1);
    G8RTOS_AddThread(&task2);

//    // Quiz Semaphores
//    G8RTOS_InitSemaphore(&sem_counter0, 1);
//    G8RTOS_InitSemaphore(&sem_counter1, 1);
//    G8RTOS_InitSemaphore(&sem_counter2, 1);
//    G8RTOS_InitSemaphore(&sem_counter3, 1);
//    G8RTOS_InitSemaphore(&sem_leds, 1);
//    // Quiz Threads
//    G8RTOS_AddThread(&Thread0);
//    G8RTOS_AddThread(&Thread1);
//    G8RTOS_AddThread(&Thread2);
//    G8RTOS_AddThread(&Thread3);
//    G8RTOS_AddThread(&ResetThread);
//    G8RTOS_AddThread(&LEDThread);

    G8RTOS_Launch();
}

/* ---------------------------------------- MAIN ---------------------------------------- */
