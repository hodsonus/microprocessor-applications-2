#include <msp.h>
#include <driverlib.h>
#include <stdio.h>
#include "RGBLeds.h"

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

/**
 * main.c
 */
void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD; // stop watchdog timer

	init_RGBLEDS();
	LP3943_LedModeSet(RED, 0b1111001000001010);
    for(;;);

	uint8_t square[4][4] =
	{
        { 1, 14,  4, 15},
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

	for(;;);
}
