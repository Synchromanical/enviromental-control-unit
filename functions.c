#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "functions.h"

//#####Delay#####

void delay(uint32_t time) {

    // Set the clock frequency to 500 kHz
    uint32_t clockDivider = (16000000 / (2 * 50000)) - 1;
    SSI1_CPSR_R = (SSI1_CPSR_R & ~SSI_CPSR_CPSDVSR_M) | (clockDivider << SSI_CPSR_CPSDVSR_S);

    NVIC_ST_RELOAD_R = (time * 16000) - 1; // Set the reload value for the desired delay (assuming 16 MHz clock)
    NVIC_ST_CURRENT_R = 0;                // Clear the current value register
    while((NVIC_ST_CTRL_R & 0x00010000) == 0) {
        // Wait for the COUNT flag to be set
    }
}

//#####SPI#####

void initSPI(void) {
    // Enable the SSI module
    SYSCTL_RCGCSSI_R |= SYSCTL_RCGCSSI_R1;
    while ((SYSCTL_PRSSI_R & SYSCTL_PRSSI_R1) == 0) {}

    // Enable the GPIO Port D and wait for the peripheral to be ready
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R3;
    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R3) == 0) {}

    // Configure the SSI pins as outputs
    GPIO_PORTD_DIR_R |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
    GPIO_PORTD_DEN_R |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);

    // Configure the pins for their alternate functions (SSI1)
    GPIO_PORTD_AFSEL_R |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
    GPIO_PORTD_PCTL_R = (GPIO_PORTD_PCTL_R & 0xFFFF0000) | (0x00000002 << 0) | (0x00000002 << 4) | (0x00000002 << 8) | (0x00000002 << 12);

    // Configure the SSI module
    SSI1_CR1_R &= ~SSI_CR1_SSE;
    SSI1_CR1_R &= ~SSI_CR1_MS;
    SSI1_CR1_R &= ~SSI_CR1_SOD;
    SSI1_CC_R = 0x0;
    SSI1_CR0_R |= SSI_CR0_DSS_8; // Set the data frame size to 8 bits
    SSI1_CR0_R |= SSI_CR0_SPH; // Set the clock to capture data on the second clock edge
    SSI1_CR0_R |= SSI_CR0_SPO; // Set the clock to drive data on the falling clock edge
    SSI1_CR0_R |= SSI_CR0_FRF_MOTO; // Set the SPI frame format to Motorola SPI mode

    // Set the clock frequency to 500 kHz
    uint32_t clockDivider = (16000000 / (2 * 50000)) - 1;
    SSI1_CPSR_R = (SSI1_CPSR_R & ~SSI_CPSR_CPSDVSR_M) | (clockDivider << SSI_CPSR_CPSDVSR_S);

    // Enable the SSI module
    SSI1_CR1_R |= SSI_CR1_SSE;
}

void sendChar(char data) {
    // Wait until the transmit buffer is not full
    while ((SSI1_SR_R & SSI_SR_TNF) == 0) {}

    // Send the data over SPI
    SSI1_DR_R = data;
}

