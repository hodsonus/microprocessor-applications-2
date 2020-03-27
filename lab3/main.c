#include <G8RTOS/G8RTOS.h>
#include "msp.h"

/* ---------------------------------------- MAIN ---------------------------------------- */

#define QUIZ

/* Avoids breaking our scheduler
 * if every thread is blocked or asleep */
void idleThread(void) {
    while(1);
}

#ifdef DEMO
#include "Threads.h"

void main(void)
{
    // initialize all components on the board
    G8RTOS_Init();

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
    G8RTOS_AddThread(&thread0);
    G8RTOS_AddThread(&thread1);
    G8RTOS_AddThread(&thread2);
    G8RTOS_AddThread(&thread3);
    G8RTOS_AddThread(&thread4);

    // add our periodic events
    G8RTOS_AddPeriodicEvent(&pthread0, 100);
    G8RTOS_AddPeriodicEvent(&pthread1, 1000);

    /* Avoids breaking our scheduler
     * if every thread is blocked or asleep */
    G8RTOS_AddThread(&idleThread);

    // and launch the OS!
    G8RTOS_Launch();
}

#endif

#ifdef QUIZ
#include "Quiz.h"

void main(void)
{
    // initialize all components on the board
    G8RTOS_Init();

    // initialize our semaphores
    G8RTOS_InitSemaphore(&led_mutex, 1);

    // add our normal threads
    G8RTOS_AddThread(&quizCoinAndCollisionThread);
    G8RTOS_AddThread(&quizPlayerThread);
    G8RTOS_AddThread(&quizEnemyThread);

    // add our periodic events
    G8RTOS_AddPeriodicEvent(&quizPThread, 1000);

    /* Avoids breaking our scheduler
     * if every thread is blocked or asleep */
    G8RTOS_AddThread(&idleThread);

    // and launch the OS!
    G8RTOS_Launch();
}

#endif

/* ---------------------------------------- MAIN ---------------------------------------- */
