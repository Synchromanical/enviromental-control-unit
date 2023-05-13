#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "functions.h"

//#####Menus#####

Menu menuItems[] = {
   {
        .lines = {// LINK 0
            "MAIN MENU",
            "1> Parameters",
            "2> Alarms",
            "3> Settings~"
        },
        .links = {1, 2, 3, -2}
    },
    {
        .lines = { //LINK 1
            "PARAMETERS",
            "TEMP:%3.1fC HUMD:%3.1f%%",
            "1> Temp.   2> Humd.",
            "3> Main Menu~"
        },
        .links = {4, 5, 0, -2}
    },
    {
        .lines = { //LINK 2
            "ALARM HISTORY",
            "-[TYPE] too LOW/HIGH!",
            "-[TYPE] too LOW/HIGH!",
            "1>Nxt pg 2>Main menu~"
        },
        .links = {2, 0, -2, -2}
    },
    {
        .lines = { //LINK 3
            "SETTINGS",
            "1> Motor settings",
            "2> Clear alarms",
            "3> Main menu~"
        },
        .links = {6, -2, 0, -2}
    },
    {
        .lines = { //LINK 4
            "TEMP. MENU",
            "1> Increase Vent.",
            "2> Decrease Vent.",
            "3> Back~"
        },
        .links = {-2, -2, -1, -2}
    },
    {
        .lines = { //LINK 5
            "HUMID. MENU",
            "1> Increase Temp.",
            "2> Decrease Temp.",
            "3> Back~"
        },
        .links = {-2, -2, -1, -2}
    },
    {
        .lines = { //LINK 6
            "MOTOR SETTINGS",
            "1> Turn CW",
            "2> Turn CCW",
            "3> Back~"
        },
        .links = {-3, -4, -1, -2}
    },
};

void sendData(int selectedMenu) {
    // Send input data to LED using SPI
    sendChar(0xFE);
    sendChar(0x01);
    sendChar(0xFE);
    sendChar(0x80);
    Menu currentMenu = menuItems[selectedMenu];
    int line = 0;
    uint8_t lineAddr[] = {0x80, 0xC0, 0x94, 0xD4};
    for (line = 0; line < 4; ++line) {
        sendChar(0xFE);
        sendChar(lineAddr[line]);
        int i;
        for (i = 0 ; currentMenu.lines[line][i] != '\0'; ++i) {
            sendChar(currentMenu.lines[line][i]);
        }
    }
}

//#####Keypad#####

void handleKeyPress(int inputNumber, int *currentMenu, int *prevMenu) {
    Menu checkMenu = menuItems[*currentMenu];
    int link = checkMenu.links[inputNumber];
    if (link >= 0) {
        *prevMenu = *currentMenu;
        sendData(link);
        *currentMenu = link;
    } else if (link == -1) {
        sendData(*prevMenu);
        *currentMenu = *prevMenu;
    } else if (link == -3) {
        rotate(1, 200); // Rotate 50 steps 
    } else if (link == -4) {
        rotate(0, 200); // Rotate 50 steps 
    }
}

//##########Motor##########

void rotate(int direction, int steps) {
    const uint8_t sequence[4] = {
        0b0001, // High PB0, Low PB1
        0b0100, // High PB2, Low PB3
        0b0010, // Low PB0, High PB1
        0b1000  // Low PB2, High PB3
    };
    
    int currentStep = 0;
    int i;

    for (i = 0; i < steps; i++) {
        // Apply the current step state to the GPIO pins
        GPIO_PORTB_DATA_R = (GPIO_PORTB_DATA_R & 0xF0) | (sequence[currentStep] & 0x0F);

        // Update the current step depending on the direction
        if (direction == 0) {
            currentStep = (currentStep + 1) % 4;
        } else {
            currentStep = (currentStep + 3) % 4;
        }

        delay(1);
    }
}

//##########Humididty Sensor##########

// Configure PB4 as input for DHT22
#define AM2302_PIN     0x10 
#define AM2302_PORT    GPIO_PORTB_BASE

uint32_t read_AM2302_data() {
    uint32_t last_state = 1, counter = 0, j = 0, i;
    uint32_t data[5] = {0, 0, 0, 0, 0};

    GPIO_PORTB_DIR_R |= AM2302_PIN;
    GPIO_PORTB_DATA_R &= ~AM2302_PIN;
    delay(1000); // 1ms delay
    GPIO_PORTB_DIR_R &= ~AM2302_PIN;
    GPIO_PORTB_PUR_R |= AM2302_PIN;

    for (i = 0; i < 85; i++) {
        counter = 0;
        while ((GPIO_PORTB_DATA_R & AM2302_PIN) == last_state) {
            counter++;
            delay(3);
            if (counter == 255) break;
        }

        last_state = GPIO_PORTB_DATA_R & AM2302_PIN;
        if (counter == 255) break;
        if (i >= 3 && (i % 2 == 0)) {
            data[j / 8] <<= 1;
            if (counter > 20) data[j / 8] |= 1;
            j++;
        }
    }

    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

void Reading(uint8_t storage[4]) {
    uint32_t sensor_data, humidity, temperature;
    bool negative_temperature;

    // Define variables to store the temperature and humidity values
    uint8_t humidity_int, humidity_frac, temperature_int, temperature_frac;

    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R5) == 0) {}

    sensor_data = read_AM2302_data();

    humidity = sensor_data >> 24;
    temperature = (sensor_data >> 8) & 0xFFFF;

    negative_temperature = temperature & 0x8000;
    if (negative_temperature) {
        temperature = temperature & 0x7FFF;
    }
        // Calculate the integer and fractional parts of the humidity and temperature values
        humidity_int = humidity / 10;
        humidity_frac = humidity % 10;
        temperature_int = temperature / 10;
        temperature_frac = temperature % 10;

        // Wait for 2 seconds before reading data again
        delay(2000);

        storage[0] = humidity_int;
        storage[1] = humidity_frac;
        storage[2] = temperature_int;
        storage[3] = temperature_frac;
}

//##########Main##########

int main(void) {
    // Initialize SysTick timer
    NVIC_ST_CTRL_R = 0;                // Disable SysTick during setup
    NVIC_ST_RELOAD_R = 0x00FFFFFF;     // Maximum reload value
    NVIC_ST_CURRENT_R = 0;             // Any write to CURRENT clears it
    NVIC_ST_CTRL_R = 0x05;             // Enable SysTick with core clock

    // Enable GPIO Port B, Port F and Port A
    SYSCTL_RCGCGPIO_R |= 0x23; // FEDCBA (22=100010=B+F)

    // Wait for the GPIO modules to be ready
    while ((SYSCTL_PRGPIO_R & 0x23) == 0) {};

    // Configure input (7654321 = 2=>10 3=>100 4=>1000 7=>1000000)
    GPIO_PORTA_DIR_R &= ~0x02;
    GPIO_PORTA_DEN_R |= 0x02;
    GPIO_PORTA_PUR_R |= 0x02;

    GPIO_PORTA_DIR_R &= ~0X04;
    GPIO_PORTA_DEN_R |= 0X04;
    GPIO_PORTA_PUR_R |= 0X04;

    GPIO_PORTA_DIR_R &= ~0x08;
    GPIO_PORTA_DEN_R |= 0x08;
    GPIO_PORTA_PUR_R |= 0x08;

    GPIO_PORTA_DIR_R &= ~0x40;
    GPIO_PORTA_DEN_R |= 0x40;
    GPIO_PORTA_PUR_R |= 0x40;

    // Configure output (onboard LED)
    GPIO_PORTF_DIR_R |= 0x0E;
    GPIO_PORTF_DEN_R |= 0x0E;

    // Initialize the GPIO pins for the stepper motor
    GPIO_PORTB_DIR_R |= 0x0F;
    GPIO_PORTB_DEN_R |= 0x0F;
    GPIO_PORTB_PUR_R |= 0x0F;

    // Configure PB4 as input for DHT22
    GPIO_PORTB_DIR_R &= ~0x10;
    GPIO_PORTB_DEN_R |= 0x10;
    GPIO_PORTB_PUR_R |= 0x10;

    // Initialize the SPI module
    initSPI();

    int currentMenu = 0;
    int prevMenu = 0;
    sendData(0);

    // Initial Light
    GPIO_PORTF_DATA_R |= 0x02; //Red
    //GPIO_PORTF_DATA_R |= 0x08; //Green
    GPIO_PORTF_DATA_R |= 0x04; //Blue

    while (1) {
        uint8_t keyOne = GPIO_PORTA_DATA_R &  0x04;
        uint8_t keyTwo = GPIO_PORTA_DATA_R & 0x02;
        uint8_t keyThree = GPIO_PORTA_DATA_R & 0x08;
        uint8_t keyFour = GPIO_PORTA_DATA_R & 0x40;
        
        static int lastButtonState = -1;
        int currentButtonState = -1;
        
        if (!keyTwo) {
            currentButtonState = 1;
        } else if (!keyOne) {
            currentButtonState = 0;
        } else if (!keyThree) {
            currentButtonState = 2;
        } else if (!keyFour) {
            currentButtonState = 3;
        }
        
        if (currentButtonState != lastButtonState) {
            if (lastButtonState != -1 && currentButtonState == -1) {
                switch (lastButtonState) {
                    case 0:
                        handleKeyPress(0, &currentMenu, &prevMenu);
                        GPIO_PORTF_DATA_R &= ~0x02;
                        GPIO_PORTF_DATA_R &= ~0x08;
                        GPIO_PORTF_DATA_R |= 0x04;
                        break;
                    case 1:
                        handleKeyPress(1, &currentMenu, &prevMenu);
                        GPIO_PORTF_DATA_R &= ~0x04;
                        GPIO_PORTF_DATA_R &= ~0x08;
                        GPIO_PORTF_DATA_R |= 0x02;
                        break;
                    case 2:
                        handleKeyPress(2, &currentMenu, &prevMenu);
                        GPIO_PORTF_DATA_R &= ~0x02;
                        GPIO_PORTF_DATA_R &= ~0x04;
                        GPIO_PORTF_DATA_R |= 0x08;
                        break;
                    case 3:
                        handleKeyPress(3, &currentMenu, &prevMenu);
                        GPIO_PORTF_DATA_R &= ~0x02;
                        GPIO_PORTF_DATA_R |= 0x04;
                        GPIO_PORTF_DATA_R |= 0x08;
                        break;
                }
            }
            lastButtonState = currentButtonState;
        }

        delay(100);
    }
}
