#ifndef FUNCTIONS_H
#define FUNCTIONS_H

// Declare the function
void delay(uint32_t time);
void initSPI(void);
void sendChar(char data);

typedef struct {
    char *lines[4];
    int links[4];
} Menu;

#endif
