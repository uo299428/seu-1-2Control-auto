/*
 * Header for the Hardware Abstraction Layer (HAL)
 */

#ifndef HAL_H 
#define HAL_H

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <eeprom.h>

// DHT sensor pin and type
#define DHTPIN 22 //GPIO22
#define DHTTYPE DHT11

// Push button pin
#define PUSH_BUTTON_PIN 34 // GPIO34

// Heater/dehumidifier control pin
#define CONTROL_PIN 25 // GPIO25
  
// States of push button and control
#define OFF  0
#define ON   1

// Pressed and released status for push button
#define BUTTON_PRESSED  0 
#define BUTTON_RELEASED 1 

// typedef struct
// {
//   uint32_t validation_code;
//   char character;
// } eeprom_params_t;

void initTempHumiditySensor();
float getTemperature();
float getHumidity();  
int getPushButtonState();
void setControlState(int state);
int getControlState();
void setStatusLed(int state);
// void setFactoryDefaults(eeprom_params_t &params);
// int checkEEpromInfo(eeprom_params_t eeprom_params, eeprom_params_t factory_default_params);
// void storeLedState(int ledState, eeprom_params_t eeprom_params);

#endif