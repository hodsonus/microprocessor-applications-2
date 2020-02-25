/*
 * Quiz.c
 *
 *  Created on: Feb 21, 2020
 *      Author: johnhodson
 */

#include "Quiz.h"
#include "demo_sysctl.h"
#include <stdint.h>
#include <G8RTOS/G8RTOS.h>
#include <BSP.h>

void Thread0(void) {
    while(1) {
        G8RTOS_WaitSemaphore(&sem_counter0);
        counter0++;
        if (counter0 >= 8) counter0 = 0;
        G8RTOS_SignalSemaphore(&sem_counter0);
        DelayMs(500);
    }
}

void Thread1(void) {
    while(1) {
        G8RTOS_WaitSemaphore(&sem_counter1);
        counter1++;
        if (counter1 >= 8) counter1 = 0;
        G8RTOS_SignalSemaphore(&sem_counter1);
        DelayMs(600);
    }
}

void Thread2(void) {
    while(1) {
        G8RTOS_WaitSemaphore(&sem_counter2);
        counter2++;
        if (counter2 >= 8) counter2 = 0;
        G8RTOS_SignalSemaphore(&sem_counter2);
        DelayMs(700);
    }
}

void Thread3(void) {
    while(1) {
        G8RTOS_WaitSemaphore(&sem_counter3);
        counter3++;
        if (counter3 >= 8) counter3 = 0;
        G8RTOS_SignalSemaphore(&sem_counter3);
        DelayMs(800);
    }
}

void ResetThread(void) {
    while(1) {
        G8RTOS_WaitSemaphore(&sem_counter0);
        int sum = counter0;
        G8RTOS_SignalSemaphore(&sem_counter0);

        G8RTOS_WaitSemaphore(&sem_counter1);
        sum += counter1;
        G8RTOS_SignalSemaphore(&sem_counter1);

        G8RTOS_WaitSemaphore(&sem_counter2);
        sum += counter2;
        G8RTOS_SignalSemaphore(&sem_counter2);

        G8RTOS_WaitSemaphore(&sem_counter3);
        sum += counter3;
        G8RTOS_SignalSemaphore(&sem_counter3);

        if (sum >= 14) {
            G8RTOS_WaitSemaphore(&sem_counter0);
            counter0 = 0;
            G8RTOS_SignalSemaphore(&sem_counter0);

            G8RTOS_WaitSemaphore(&sem_counter1);
            counter1 = 0;
            G8RTOS_SignalSemaphore(&sem_counter1);

            G8RTOS_WaitSemaphore(&sem_counter2);
            counter2 = 0;
            G8RTOS_SignalSemaphore(&sem_counter2);

            G8RTOS_WaitSemaphore(&sem_counter3);
            counter3 = 0;
            G8RTOS_SignalSemaphore(&sem_counter3);
        }
    }
}

void LEDThread(void) {

    P4->DIR &= ~BIT4;
    P4->OUT |= BIT4;
    P4->REN |= BIT4;

    while (1) {

        uint16_t bitmask = 0;

        // counter 0 is our initial minimum
        G8RTOS_WaitSemaphore(&sem_counter0);
        int minCount = counter0;
        int minThread = 0;
        bitmask |= (counter0 << 2);
        G8RTOS_SignalSemaphore(&sem_counter0);

        // counter 1
        G8RTOS_WaitSemaphore(&sem_counter1);
        if (counter1 < minCount) {
            minCount = counter1;
            minThread = 1;
        }
        bitmask |= (counter1 << 5);
        G8RTOS_SignalSemaphore(&sem_counter1);

        // counter 2
        G8RTOS_WaitSemaphore(&sem_counter2);
        if (counter2 < minCount) {
            minCount = counter2;
            minThread = 2;
        }
        bitmask |= (counter2 << 8);
        G8RTOS_SignalSemaphore(&sem_counter2);

        // counter 3
        G8RTOS_WaitSemaphore(&sem_counter3);
        if (counter3 < minCount) {
            minCount = counter3;
            minThread = 3;
        }
        bitmask |= (counter3 << 11);
        G8RTOS_SignalSemaphore(&sem_counter3);

        bitmask |= minThread;

        G8RTOS_WaitSemaphore(&sem_leds);
        LP3943_LedModeSet(BLUE, bitmask);
        G8RTOS_SignalSemaphore(&sem_leds);

        while(!(P4->IN & BIT4));
    }
}
