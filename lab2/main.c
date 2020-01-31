#include <msp.h>
#include <driverlib.h>
#include <BSP.h>
#include <G8RTOS/G8RTOS.h>
#include <stdio.h>

/* ---------------------------------------- MAIN ---------------------------------------- */

/**
 * main.c
 */
void main(void)
{
    G8RTOS_Init();
    LP3943_LedModeSet(RED, 0xFFFF);
//    G8RTOS_AddThread(WaitInit,1,NULL);
    G8RTOS_Launch();
}
