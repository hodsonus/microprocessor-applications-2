#include <msp.h>
#include <driverlib.h>
#include <BSP.h>
#include <G8RTOS/G8RTOS.h>
#include <stdio.h>

/* ---------------------------------------- MAIN ---------------------------------------- */

uint32_t counter0;
uint32_t counter1;
uint32_t counter2;

void task0(void)
{
    while(1)
    {
        ++counter0;
    }
}

void task1(void)
{
    while(1)
    {
        ++counter1;
    }
}

void task2(void)
{
    while(1)
    {
        ++counter2;
    }
}

/**
 * main.c
 */
void main(void)
{
    G8RTOS_Init();
    G8RTOS_AddThread(&task0);
    G8RTOS_AddThread(&task1);
    G8RTOS_AddThread(&task2);
    G8RTOS_Launch();
}

/* ---------------------------------------- MAIN ---------------------------------------- */
