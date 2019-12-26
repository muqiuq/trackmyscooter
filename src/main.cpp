#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <WiFiUdp.h>
#include "locpin.h"
#include "config.h"

#define LED_LEFT 2
#define LED_RIGHT 14
#define TILT_SENSOR 12
#define SPEAKER 13
#define SABOTAGE 15

Ticker ticker_frontLed;
Ticker ticker_checkTilt;
Ticker ticker_speaker;
Ticker ticker_transmit;

bool flag_doreconnect = false;
bool flag_test = false;
bool flag_transmit = false;

bool front_led_state = false;
bool sabotage = false;

bool is_armed = true;
bool alarm_triggered = false;

bool ok_tilt_state = false;

long tiltcounter = 0;
uint32_t lastShockTimeout;

#define SIRENE_DURATION 60000
uint32_t sirene_timeout = 0;

bool speaker_state = false;

WiFiUDP Udp;
unsigned int localUdpPort = 5028;

void tick_speaker() {
  if(sirene_timeout > millis()) {
    if(speaker_state) {
      analogWrite(SPEAKER, 140);
    }else{
      analogWrite(SPEAKER, 0);
    }
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
  if(!WiFi.isConnected()) {
    digitalWrite(LED_LEFT, HIGH);
  }
}

void tick_transmit() {
  flag_transmit = true;
}

void setLedStateDisarmed() {
  ticker_frontLed.attach(0.5, tick_frontLed);
}

void processInput(String inputText) {
  #ifdef DEBUG
  Serial.println(inputText);
  #endif
  if(inputText == PASSWORD) {
    is_armed = !is_armed;
    alarm_triggered = false;
    ticker_speaker.detach();
    analogWrite(SPEAKER, 0);
    if(!is_armed) {
      // Just Disarmed
      digitalWrite(LED_RIGHT, HIGH);
      setLedStateDisarmed();
      ticker_transmit.attach(15, tick_transmit);
    }else{
      // Just armed
      ticker_transmit.attach(2, tick_transmit);
      digitalWrite(LED_LEFT, HIGH);
      digitalWrite(LED_RIGHT, HIGH);
      tiltcounter = 0;
    }
  }
  else if(inputText == RECONNECT_CODE && !is_armed) {
    flag_doreconnect = true;
    digitalWrite(LED_RIGHT, HIGH);
  }else if(inputText == TEST_CODE && !is_armed) {
    flag_test = true;
  }
}

void trigger_alarm() {
  alarm_triggered = true;
  ticker_frontLed.attach(0.1, tick_frontLed);
  ticker_speaker.attach(0.1, tick_speaker);
  sirene_timeout = millis() + SIRENE_DURATION;
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
  if(sabotage != true && digitalRead(SABOTAGE) == HIGH) {
    #ifdef DEBUG
    Serial.println("Sabotage!");
    #endif
    sabotage = true;
  }
}

void checkAndWaitForWifiState() {
  Serial.print("Connecting");
  int conn_timeout = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    conn_timeout++;
    if(conn_timeout > 20) break;
  }
  Serial.println();

  if(WiFi.isConnected()) {
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.BSSIDstr());
  }else{
    Serial.println("Not connected");
  }
}



void setup() {
  Serial.begin(115200);
  Serial.println();
  
  pinMode(LED_LEFT, OUTPUT);
  pinMode(LED_RIGHT, OUTPUT);
  pinMode(TILT_SENSOR, INPUT_PULLUP);
  pinMode(SABOTAGE, INPUT_PULLUP);
  pinMode(SPEAKER, OUTPUT);

  init_buttons(processInput);

  setLedStateDisarmed();

  
#ifdef WIFI_PASSPHRASE
  WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);
#else
  WiFi.begin(WIFI_SSID);
#endif

  checkAndWaitForWifiState();

  ok_tilt_state = digitalRead(TILT_SENSOR);
  ticker_checkTilt.attach(0.1, tick_checkTilt);

  Udp.begin(localUdpPort);
  ticker_transmit.attach(2, tick_transmit);
  
}

void loop_flags() {
  if(flag_doreconnect) {
    WiFi.reconnect();
    flag_doreconnect = false;
    checkAndWaitForWifiState();
  }
  if(flag_test) {
    ticker_frontLed.detach();
    digitalWrite(LED_LEFT, HIGH);
    digitalWrite(LED_RIGHT, HIGH);
    analogWrite(SPEAKER, 140);
    delay(1000);
    analogWrite(SPEAKER, 0);
    setLedStateDisarmed();
    flag_test = false;
  }
}

void loop_transmit() {
  if(flag_transmit) {
    if(WiFi.isConnected()) {
      #ifdef DEBUG
      Serial.print("Sending Status...");
      #endif
      Udp.beginPacket(STATUS_SERVER_HOST, STATUS_SERVER_PORT);
      String returnMessage = "TMSTMSTMS " + WiFi.BSSIDstr();
      returnMessage = returnMessage + " " + (is_armed ? "A" : "D");
      returnMessage = returnMessage + " " + (alarm_triggered ? "1" : "0");
      returnMessage = returnMessage + " " + (sirene_timeout > millis() ? "S" : "0");
      returnMessage = returnMessage + " " + (sabotage ? "T" : "0");
      const char * replyPacket = returnMessage.c_str();
      Udp.write(replyPacket);
      Udp.endPacket();
      #ifdef DEBUG
      Serial.println("Done");
      #endif
    }
    flag_transmit = false;
  }
}

void loop() {
  loop_buttons();  
  loop_flags();
  loop_transmit();
}
