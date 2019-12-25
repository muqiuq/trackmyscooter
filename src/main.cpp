#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include "locpin.h"

#define LED_LEFT 2
#define LED_RIGHT 14
#define TILT_SENSOR 12
#define SPEAKER 13


Ticker ticker_frontLed;
Ticker ticker_checkTilt;
Ticker ticker_speaker;

bool front_led_state = false;

bool is_armed = true;
bool alarm_triggered = false;

bool ok_tilt_state = false;

long tiltcounter = 0;
uint32_t lastShockTimeout;

bool speaker_state = false;

void tick_speaker() {
  if(speaker_state) {
    analogWrite(SPEAKER, 140);
  }else{
    analogWrite(SPEAKER, 0);
  }
  speaker_state = !speaker_state;
}

void tick_frontLed() {
  if(is_armed) {
    if(alarm_triggered) {
      digitalWrite(LED_LEFT, front_led_state);
      digitalWrite(LED_RIGHT, front_led_state);  
      front_led_state = !front_led_state;
      
    }else{
      digitalWrite(LED_LEFT, false);
      if(front_led_state) {
        digitalWrite(LED_RIGHT, true);
        ticker_frontLed.attach(0.1, tick_frontLed);
        front_led_state = false;
      }else{
        digitalWrite(LED_RIGHT, false);
        front_led_state = true;
        ticker_frontLed.attach(10, tick_frontLed);
      }
    }
  }else{
    if(lastShockTimeout > millis()) {
      digitalWrite(LED_LEFT, front_led_state);
      digitalWrite(LED_RIGHT, !front_led_state);
      front_led_state = !front_led_state;
    }else{
      digitalWrite(LED_LEFT, false);
      digitalWrite(LED_RIGHT, false);
    }
  }
}

void processInput(String inputText) {
  Serial.println(inputText);
  if(inputText == "1122") {
    is_armed = !is_armed;
    alarm_triggered = false;
    ticker_speaker.detach();
    analogWrite(SPEAKER, 0);
    if(!is_armed) {
      ticker_frontLed.attach(0.5, tick_frontLed);
    }else{
      tiltcounter = 0;
    }
  }
}

void trigger_alarm() {
  alarm_triggered = true;
  ticker_frontLed.attach(0.1, tick_frontLed);
  ticker_speaker.attach(0.1, tick_speaker);
}


void tick_checkTilt() {
  uint8_t tilt_sensor_state = digitalRead(TILT_SENSOR);
  if(is_armed && !alarm_triggered && ok_tilt_state != tilt_sensor_state) {
    tiltcounter++;
    ok_tilt_state = tilt_sensor_state;
    if(tiltcounter > 10) {
      trigger_alarm();
    }
  }else if(!is_armed && ok_tilt_state != tilt_sensor_state && lastShockTimeout < millis() - 1000) {
    ok_tilt_state = tilt_sensor_state;
    lastShockTimeout = millis() + 10000;
  }
}



void setup() {
  Serial.begin(115200);
  Serial.println();
  
  pinMode(LED_LEFT, OUTPUT);
  pinMode(LED_RIGHT, OUTPUT);
  pinMode(TILT_SENSOR, INPUT_PULLUP);
  pinMode(SPEAKER, OUTPUT);

  init_buttons(processInput);

  ticker_frontLed.attach(0.5, tick_frontLed);

  ok_tilt_state = digitalRead(TILT_SENSOR);

  WiFi.begin("fl", "");

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.BSSIDstr());

  ticker_checkTilt.attach(0.1, tick_checkTilt);
  
}



void loop() {
  loop_buttons();  
  
}
