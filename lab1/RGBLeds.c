/*
 * RGBLeds.c
 *
 *  Created on: Jan 11, 2020
 *      Author: johnhodson
 */

#include <RGBLeds.h>

// TODO
void LP3943_ColorSet(uint32_t unit, uint32_t PWM_DATA)
{

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
    UCB2I2CSA = BASE_ADDR + unit;

    // Generate START condition, send chip address
    UCB2CTLW0 |= UCTXSTT;

    // Wait for the start flag to go low
    // Wait for buffer availability
    // eUSCI_B transmit interrupt flag 0. UCTXIFG0 set when UCBxTXBUF empty
    while(UCB2CTLW0 & UCTXSTT);

    // Fill TXBUF with register address and auto increment, wait for buffer
    UCB2TXBUF = 0x16;
    while(!(UCB2IFG & UCTXIFG0));

    // Fill TXBUF with LS0 data for the LP3943, wait for buffer
    UCB2TXBUF = LSO_data;
    while(!(UCB2IFG & UCTXIFG0));

    // Fill TXBUF with LS1 data for the LP3943, wait for buffer
    UCB2TXBUF = LS1_data;
    while(!(UCB2IFG & UCTXIFG0));

    // Fill TXBUF with LS2 data for the LP3943, wait for buffer
    UCB2TXBUF = LS2_data;
    while(!(UCB2IFG & UCTXIFG0));

    // Fill TXBUF with LS3 data for the LP3943, wait for buffer
    UCB2TXBUF = LS3_data;
    while(!(UCB2IFG & UCTXIFG0));

    // Generate STOP condition and wait for the STOP bit to go low
    UCB2CTLW0 |= UCTXSTP;
    while(UCB2CTLW0 & UCTXSTP);
}

void init_RGBLEDS()
{
    // Software reset enable
    UCB2CTLW0 = UCSWRST;

    // Initialize I2C master
    // Sets as master, I2C mode, Clock sync, SMCLK source, Transmitter
    UCB2CTLW0 |= UCMST
               | UCMODE_3
               | UCSYNC
               | UCSSEL_2
               | UCTR;

    // Sets the FCLCK as 400khz
    // Presumes that the SMCLK is selected as source and FSMCLK is 12MHz
    UCB2BRW = 30;

    // In conjunction with the next line, this sets the pins as I2C mode
    // (table found on pg. 143 of SLAS826E)
    // Sets P3.6 as UCB2_SDA and 3.7 as UCB2_SCL
    P3SEL0 |= BIT6 | BIT7;
    P3SEL1 &= ~BIT6 & ~BIT7;

    // Bitwise ANDing of all bits except UCSWRST (appropriate settings enabled)
    UCB2CTLW0 &= ~UCSWRST;

    uint16_t UNIT_OFF = 0x0000;

    LP3943_LedModeSet(RED, UNIT_OFF);
    LP3943_LedModeSet(GREEN, UNIT_OFF);
    LP3943_LedModeSet(BLUE, UNIT_OFF);
}
