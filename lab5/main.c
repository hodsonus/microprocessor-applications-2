/*
 * Places need to change name for different configuration:
 *      Game.h
 *      sl_common.h
 *      cc3100_usage.h
 */

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
    // Need to initialize first because the watchdog timer needs to be disabled.
    G8RTOS_Init(USING_TP, HOST_OR_CLIENT);

    // Can alternatively use TLV->RANDOM_NUM_1 for repeatable number generation.
    srand(time(NULL));

    // Add the thread that bootstraps the game.
    G8RTOS_AddThread(&HostVsClient, 0, "host vs client");

    // Launch the OS!
    G8RTOS_Launch();
}

/* ---------------------------------------- MAIN ---------------------------------------- */
