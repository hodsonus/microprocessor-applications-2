#include <stdlib.h>
#include <time.h>
#include "G8RTOS/G8RTOS.h"
#include "msp.h"
#include "Game.h"

/* ---------------------------------------- MAIN ---------------------------------------- */

#define USING_TP true
#define HOST_OR_CLIENT Host

/**
 * main.c
 */
void main(void)
{
    G8RTOS_Init(USING_TP, HOST_OR_CLIENT);

    G8RTOS_AddThread(&HostVsClient, 0, "host vs client");

    G8RTOS_Launch();
}

/* ---------------------------------------- MAIN ---------------------------------------- */
