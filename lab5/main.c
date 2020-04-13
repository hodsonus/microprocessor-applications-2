/*
 * Places need to change name for different configuration:
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

void latencyTest(void);

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
//    G8RTOS_AddThread(&HostVsClient, 0, "host vs client");
    G8RTOS_AddThread(&latencyTest, 0, "host vs client");

    // Launch the OS!
    G8RTOS_Launch();
}

int elapsedTime;

void latencyTest(void)
{
    while (1)
    {
        SpecificPlayerInfo_t tempClientInfo = {
                            CONFIG_IP,     // IP Address
                            0,             // displacement
                            TOP,           // playerNumber
                            1,             // ready
                            0,             // joined
                            0              // acknowledge
                };

        // Start timer
        int startTime = SystemTime;

        // Send packet to Shida's board
        SendData((uint8_t*)(&tempClientInfo), HOST_IP_ADDR, sizeof(SpecificPlayerInfo_t)/sizeof(uint8_t));

        // Receive packet from Shida's board
        while( ReceiveData((uint8_t*)(&tempClientInfo), sizeof(SpecificPlayerInfo_t)/sizeof(uint8_t)) == NOTHING_RECEIVED);

        // Stop timer
        elapsedTime = SystemTime - startTime;

        DelayMs(5);
    }
}

/* ---------------------------------------- MAIN ---------------------------------------- */
