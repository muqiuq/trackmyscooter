#include "locpin.h"


uint8_t button_state[] = {0,0,0};
uint8_t button_pins[] = {BUTTON_LOW, BUTTON_MIDDLE, BUTTON_TOP};
long button_timer[] = {0,0,0};

char input_buffer[INPUT_BUFFER_SIZE];
uint8_t input_buffer_pos = 0;

void (*callback)(String inputtext);

void init_buttons(void (*callbackFunc)(String inputtext)) {
  pinMode(BUTTON_LOW, INPUT_PULLUP);
  pinMode(BUTTON_MIDDLE, INPUT_PULLUP);
  pinMode(BUTTON_TOP, INPUT_PULLUP);
  callback = callbackFunc;
}

void push_inputBuffer(uint8_t num) {
    if(num == 0) {
        callback(String(input_buffer));
        for(int a = 0; a < INPUT_BUFFER_SIZE; a++) {
            input_buffer[a] = 0;
        }
        input_buffer_pos = 0;
    }else{
        input_buffer[input_buffer_pos] = 0x30 + num;
        input_buffer_pos++;
        if(input_buffer_pos == INPUT_BUFFER_SIZE) {
            input_buffer_pos = 0;
        }
    }

}

void loop_buttons() {
    for(int a = 0; a < BUTTON_COUNT; a++) {
        if(button_timer[a] < millis()) {
            if(digitalRead(button_pins[a]) == LOW && button_state[a] == 1) {
                push_inputBuffer(a);
                button_timer[a] = millis() + 100;
                button_state[a] = 0;
            }
            else if(digitalRead(button_pins[a]) == HIGH && button_state[a] == 0) {
                button_timer[a] = millis() + 100;
                button_state[a] = 1;
            }
        }
    }
}