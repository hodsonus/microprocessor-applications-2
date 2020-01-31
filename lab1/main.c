#include <msp.h>
#include <driverlib.h>
#include <stdio.h>
#include "RGBLeds.h"
#include "BSP.h"

/* ---------------------------------------- LAB PART A ---------------------------------------- */

/* Configuration for UART */
static const eUSCI_UART_Config Uart115200Config =
{
    EUSCI_A_UART_CLOCKSOURCE_SMCLK, // SMCLK Clock Source
    6, // BRDIV
    8, // UCxBRF
    0, // UCxBRS
    EUSCI_A_UART_NO_PARITY, // No Parity
    EUSCI_A_UART_LSB_FIRST, // LSB First
    EUSCI_A_UART_ONE_STOP_BIT, // One stop bit
    EUSCI_A_UART_MODE, // UART mode
    EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION // Oversampling
};

void uartInit()
{
    /* select GPIO functionality */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_12);

    /* configure baud rate */
    MAP_UART_initModule(EUSCI_A0_BASE, &Uart115200Config);

    /* enable UART */
    MAP_UART_enableModule(EUSCI_A0_BASE);
}

static inline void uartTransmitString(char* s)
{
    /* Loop while not null */
    while(*s)
    {
        MAP_UART_transmitData(EUSCI_A0_BASE, *s++);
    }
}

uint16_t fletcher16c(uint8_t *data, int count)
{
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;

    for (int i = 0; i < count; ++i)
    {
        sum1 = (sum1 + data[i]) % 255;
        sum2 = (sum2 + sum1) % 255;
    }

    return (sum2 << 8) | sum1;
}

extern uint16_t fletcher16s(uint8_t *data, int count);

int Modulus255(int val)
{
    return val % 255;
}

void fletcherChecksumDemo()
{
    uint8_t square[4][4] =
    {
        { 18, 14,  4, 15},
        { 8, 11,  5, 10},
        {13,  2, 16,  3},
        {12,  7,  9,  6}
    };

    int checksumc = fletcher16c(&square[0][0], 16);
    int checksums = fletcher16s(&square[0][0], 16);

    // serial port /dev/cu.usbmodemM43210051
    uartInit();

    char strc[255];
    snprintf(strc, 255, "C implementation checksum is %d\n", checksumc);
    uartTransmitString(strc);

    char strs[255];
    snprintf(strs, 255, "ASM implementation checksum is %d\n", checksums);
    uartTransmitString(strs);

    while(1);
}

/* ---------------------------------------- LAB PART B ---------------------------------------- */

void Delay(int cycles)
{
    for (int i = 0; i < cycles; ++i);
}

void rgbDriverDemo()
{
    init_RGBLEDS();

    uint8_t square[4][4] =
        {
            { 1, 14,  4, 15},
            { 8, 11,  5, 10},
            {13,  2, 16,  3},
            {12,  7,  9,  6}
        };

    uint16_t checksums = fletcher16s(&square[0][0], 16);

    LP3943_LedModeSet(RED, checksums);
    LP3943_LedModeSet(BLUE, checksums);
    LP3943_LedModeSet(GREEN, checksums);

    while(1);
}

void rgbLoop()
{
    init_RGBLEDS();

    int DELAY_LOOPS = ClockSys_GetSysFreq()/50;
    uint32_t LED;

    while (1)
    {
        LED = 0x0001;
        for (int i = 0; i < 16; ++i)
        {
            LP3943_LedModeSet(RED, LED);
            Delay(DELAY_LOOPS);
            LED <<= 1;
        }
        LP3943_LedModeSet(RED, 0x0000);

        LED = 0x0001;
        for (int i = 0; i < 16; ++i)
        {
            LP3943_LedModeSet(BLUE, LED);
            Delay(DELAY_LOOPS);
            LED <<= 1;
        }
        LP3943_LedModeSet(BLUE, 0x0000);

        LED = 0x0001;
        for (int i = 0; i < 16; ++i)
        {
            LP3943_LedModeSet(GREEN, LED);
            Delay(DELAY_LOOPS);
            LED <<= 1;
        }
        LP3943_LedModeSet(GREEN, 0x0000);
    }
}

/* ---------------------------------------- LAB PART C ---------------------------------------- */

uint32_t redState;
uint32_t blueState;
uint32_t greenState;
bool isPattern1, changeState;

void SysTick_Handler() {

    if (changeState)
    {
        isPattern1 = !isPattern1;
    }

    if (isPattern1)
    {
        if (redState == 0x0000 || changeState)
        {
            redState = 0xF000;
//            greenState = 0xF000;
            blueState = 0x000F;
        }
        else
        {
            redState >>= 1;
//            greenState >>= 1;
            blueState <<= 1;
        }
    }
    else
    {
        if (changeState)
        {
            redState = 0xAAAA;
//            greenState = 0xAAAA;
            blueState = 0x5555;
        }
        else
        {
            redState = ~redState;
//            greenState = ~greenState;
            blueState = ~blueState;
        }
    }

    changeState = false;

    LP3943_LedModeSet(RED, redState);
    LP3943_LedModeSet(GREEN, greenState);
    LP3943_LedModeSet(BLUE, blueState);
}

void PORT4_IRQHandler(void)
{
    P4->IFG &= ~BIT4; // clear IFG flag

    changeState = true;
}

void isrLightshowDemo()
{
    redState = 0;
    greenState = 0;
    blueState = 0;
    isPattern1 = true;
    changeState = false;

    init_RGBLEDS();

    SysTick_Config(ClockSys_GetSysFreq() / 4); // configure to trigger every 250ms

    P4->DIR &= ~BIT4; // configure P4.4 as input
    P4->IFG &= ~BIT4; // P4.4 IFG cleared
    P4->IE |= BIT4; // Enable interrupt on P4.4
    P4->IES |= BIT4; // high-to-low transition
    P4->REN |= BIT4; // Pull-up resister
    P4->OUT |= BIT4; // Sets res to pull-up

    NVIC_EnableIRQ(PORT4_IRQn);
    SysTick_enableInterrupt();

    PCM_gotoLPM0(); // enter LPM mode
}


/* ---------------------------------------- QUIZ ---------------------------------------- */

typedef struct timedate_t
{
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
} timedate_t;

extern timedate_t timeUntilNewYears(timedate_t* timedate);

void quiz(void)
{
    timedate_t today = {.month = 2,
                        .day = 30,
                        .hour = 10,
                        .minute = 4};

    timedate_t result = timeUntilNewYears(&today);

    while(1);
}

int Modulus(int val, int div)
{
    return val % div;
}

/* ---------------------------------------- QUIZ ---------------------------------------- */


/* ---------------------------------------- MAIN ---------------------------------------- */

/**
 * main.c
 */
void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD; // stop watchdog timer

    quiz();
}
