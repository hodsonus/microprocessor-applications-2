/*
 * RGBLeds.c
 *
 *  Created on: Jan 11, 2020
 *      Author: johnhodson
 */

#include <RGBLeds.h>

void LP3943_ColorSet(uint32_t unit, uint32_t PWM_DATA)
{
    /* TODO - complete LP3493 PWM write functionality */
}

void LP3943_LedModeSet(uint32_t unit, uint16_t LED_DATA)
{
    /*
     * LP3943_LedModeSet
     * This function will set each of the LEDs to the desired operating
     * mode. The operating modes are on, off, PWM1 and PWM2.
     *
     * The units that can be written to are:
     *  UNIT    |   0   |   Blue
     *  UNIT    |   1   |   Green
     *  UNIT    |   2   |   Red
     *
     * The registers to be written to are:
     *  LSO | LED0-3 Selector
     *  LS1 | LED4-7 Selector
     *  LS2 | LED8-11 Selector
     *  LS3 | LED12-16 Selector
     */

    // Generate data you want send via I2C
    uint8_t LSO_data = 0x00;
    uint8_t LS1_data = 0x00;
    uint8_t LS2_data = 0x00;
    uint8_t LS3_data = 0x00;

    int i;
    for (i = 0; i < 4; ++i) LSO_data |= ((LED_DATA >> i     ) & 0x1) << i*2;
    for (i = 0; i < 4; ++i) LS1_data |= ((LED_DATA >> (4+i) ) & 0x1) << i*2;
    for (i = 0; i < 4; ++i) LS2_data |= ((LED_DATA >> (8+i) ) & 0x1) << i*2;
    for (i = 0; i < 4; ++i) LS3_data |= ((LED_DATA >> (12+i)) & 0x1) << i*2;

    // Calculate slave address from unit no.
    // First 7 bits -> slave address
    // 8th bit -> R/~W
    // Set initial slave address since we are master
    const uint8_t BASE_ADDR = 0x60;
    EUSCI_B2->I2CSA = BASE_ADDR + unit;

    // Generate START condition, send chip address
    EUSCI_B2->CTLW0 |= EUSCI_B_CTLW0_TXSTT;

    // Wait for the start flag to go low
    // Wait for buffer availability
    // eUSCI_B transmit interrupt flag 0. UCTXIFG0 set when UCBxTXBUF empty
    while(EUSCI_B2->CTLW0 & EUSCI_B_CTLW0_TXSTT);

    // Fill TXBUF with register address and auto increment, wait for buffer
    EUSCI_B2->TXBUF = 0x16;
    while(!(EUSCI_B2->IFG & EUSCI_B_IFG_TXIFG0));

    // Fill TXBUF with LS0 data for the LP3943, wait for buffer
    EUSCI_B2->TXBUF = LSO_data;
    while(!(EUSCI_B2->IFG & EUSCI_B_IFG_TXIFG0));

    // Fill TXBUF with LS1 data for the LP3943, wait for buffer
    EUSCI_B2->TXBUF = LS1_data;
    while(!(EUSCI_B2->IFG & EUSCI_B_IFG_TXIFG0));

    // Fill TXBUF with LS2 data for the LP3943, wait for buffer
    EUSCI_B2->TXBUF = LS2_data;
    while(!(EUSCI_B2->IFG & EUSCI_B_IFG_TXIFG0));

    // Fill TXBUF with LS3 data for the LP3943, wait for buffer
    EUSCI_B2->TXBUF = LS3_data;
    while(!(EUSCI_B2->IFG & EUSCI_B_IFG_TXIFG0));

    // Generate STOP condition and wait for the STOP bit to go low
    EUSCI_B2->CTLW0 |= EUSCI_B_CTLW0_TXSTP;
    while(EUSCI_B2->CTLW0 & EUSCI_B_CTLW0_TXSTP);
}

void init_RGBLEDS()
{
    // Software reset enable
    EUSCI_B2->CTLW0 = EUSCI_B_CTLW0_SWRST;

    // Initialize I2C master
    // Sets as master, I2C mode, Clock sync, SMCLK source, Transmitter
    EUSCI_B2->CTLW0 |= EUSCI_B_CTLW0_MST
                    | EUSCI_B_CTLW0_MODE_3
                    | EUSCI_B_CTLW0_SYNC
                    | EUSCI_B_CTLW0_UCSSEL_2
                    | EUSCI_B_CTLW0_TR;

    // Sets the FCLCK as 400khz
    // Presumes that the SMCLK is selected as source and FSMCLK is 12MHz
    EUSCI_B2->BRW = 30;

    // In conjunction with the next line, this sets the pins as I2C mode
    // (table found on pg. 143 of SLAS826E)
    // Sets P3.6 as UCB2_SDA and 3.7 as UCB2_SCL
    P3->SEL0 |= BIT6 | BIT7;
    P3->SEL1 &= ~BIT6 & ~BIT7;

    // Bitwise ANDing of all bits except UCSWRST (appropriate settings enabled)
    EUSCI_B2->CTLW0 &= ~EUSCI_B_CTLW0_SWRST;

    uint16_t UNIT_OFF = 0x0000;

    LP3943_LedModeSet(RED, UNIT_OFF);
    LP3943_LedModeSet(GREEN, UNIT_OFF);
    LP3943_LedModeSet(BLUE, UNIT_OFF);
}
