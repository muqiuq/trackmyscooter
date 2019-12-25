#ifndef LOCPIN_H_
#define LOCPIN_H_

#include <Arduino.h>
#include <HardwareSerial.h>

#define BUTTON_TOP 0
#define BUTTON_MIDDLE 4
#define BUTTON_LOW 5
#define BUTTON_COUNT 3

#define INPUT_BUFFER_SIZE 20


void init_buttons(void (*callbackFunc)(String inputtext));

void loop_buttons();

#endif /* LOCPIN_H_ */