/*
 * Threads.c
 *
 *  Created on: Jan 31, 2020
 *      Author: johnhodson
 */

#include <stdint.h>
#include <G8RTOS/G8RTOS.h>
#include "Threads.h"

uint32_t counter0;
uint32_t counter1;
uint32_t counter2;

void task0(void)
{
    while(1)
    {
        G8RTOS_WaitSemaphore(&i2c);
        ++counter0;
        G8RTOS_SignalSemaphore(&i2c);
    }
}

void task1(void)
{
    while(1)
    {
        G8RTOS_WaitSemaphore(&i2c);
        ++counter1;
        G8RTOS_SignalSemaphore(&i2c);
    }
}

void task2(void)
{
    while(1)
    {
        G8RTOS_WaitSemaphore(&i2c);
        ++counter2;
        G8RTOS_SignalSemaphore(&i2c);
    }
}
