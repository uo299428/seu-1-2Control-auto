/*
 * Hardware Abstraction Layer (HAL)
 */

#include "hal.h"

static int controlState = OFF;

DHT dht(DHTPIN, DHTTYPE);

// Initialize temperature/humidity sensor
void initTempHumiditySensor()
{
    dht.begin();
}

// Return temperature in celsius
float getTemperature()
{
    return dht.readTemperature();
}

// Return relative humidity
float getHumidity()
{
    return dht.readHumidity();
}

// Get push button state ON/OFF
int getPushButtonState()
{
    static bool firstTime = true;

    // Set the pin as input on the first execution
    if (firstTime)
    {
        pinMode(PUSH_BUTTON_PIN, INPUT);
        firstTime = false;
    }

    if (digitalRead(PUSH_BUTTON_PIN) == BUTTON_RELEASED)
        return OFF;
    else
        return ON;
}

// Set control state ON/OFF
void setControlState(int state)
{
    static bool firstTime = true;

    if (firstTime)
    {
        pinMode(CONTROL_PIN, OUTPUT);
        firstTime = false;
    }

    controlState = state;
    if (controlState == ON){
        digitalWrite(CONTROL_PIN, HIGH);
        // Serial.println(F("Hight"));
    }
    else{
        digitalWrite(CONTROL_PIN, LOW);
        // Serial.println(F("Low"));
    }

}

// Set control state ON/OFF
void setStatusLed(int state)
{
    static bool firstTime = true;

    if (firstTime)
    {
        pinMode(CONTROL_PIN, OUTPUT);
        firstTime = false;
    }

    controlState = state;
    if (controlState == ON){
        digitalWrite(CONTROL_PIN, HIGH);
        // Serial.println(F("Hight"));
    }
    else{
        digitalWrite(CONTROL_PIN, LOW);
        // Serial.println(F("Low"));
    }

}

// Get heater/dehumidifier control state ON/OFF
int getControlState()
{
    return controlState;
}

// void setFactoryDefaults(eeprom_params_t &params)
// {
//   params.validation_code = 0xAABBCCDD;
//   params.character = 'A';
// }
// int checkEEpromInfo(eeprom_params_t eeprom_params, eeprom_params_t factory_default_params){
//    // If EEPROM info is invalid
//   if (eeprom_params.validation_code != factory_default_params.validation_code)
//   {
//     return 0;
//   }
//    else
//    return 1;
// }

// void storeLedState(int ledState, eeprom_params_t eeprom_params)
// {
//     eeprom_params.character = ledState;
//       EEPROM.writeBytes(0, &eeprom_params, sizeof(eeprom_params_t));
//       EEPROM.commit();
// }

