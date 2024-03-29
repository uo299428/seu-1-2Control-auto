#include <arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include "hal.h"
#include "webhandle.h"
#include "control.h"

// Write down your UO here
const char *UO = "uo299428";

// SSID prefix and password as an access point
const char *SSID_PREFIX = "SEU"; 
const char *PASSWORD    = "12345678"; 

// Default device name
const char DEFAULT_NAME[] = "controlseu";

// Start running as access point
int run_mode = RUN_AS_AP; 

// Maximuum bouncing time of the push button in ms
#define MAX_BOUNCING_TIME 100

// Minimum pressing time to recover factory defaults in ms
#define MIN_FACTORY_TIME 5000

// Web server on TCP port 80
WebServer server(80);

// The push button interrupt service routine sets these variables to true after 
// pressing for a short or long time 
bool restart = false;
bool invalidate_eeprom = false;

// EEPROM factory defaults
eeprom_params_t factory_default_params;

// Set factory defaults parameters
void setFactoryDefaults(eeprom_params_t &params)
{
  params.validation_code = 0xAABBCCDD;
  params.ssid_sta[0] = '\0';
  params.password_sta[0] = '\0';
  strcpy(params.name, DEFAULT_NAME);
  params.temp_setpoint = 20.0; 
  params.hum_setpoint = 60.0;
  strcpy(params.control_mode, CONTROL_MODE_OFF);
}

///////////////////////////////////////////////////////////////////////////////
// Run as access point with a web server for getting SSID, password and device 
// name parameters.
// The SSID of the access point is given by SSID_PREFIX and the UO
///////////////////////////////////////////////////////////////////////////////
void configure_as_ap_webserver()
{
  // Get the SSID as access point. For example, SEU-2A3F125D
  char ssid_ap[32];
  sprintf(ssid_ap, "%s-%s", SSID_PREFIX, UO);

  // Set up WiFi interface as access point
  Serial.printf("Setting AP with SSID: %s", ssid_ap);
  Serial.printf(WiFi.softAP(ssid_ap, PASSWORD) ? " -> OK;" : " ->Failed");
  Serial.printf("  IP address: %s\n", WiFi.softAPIP().toString().c_str());
  
  // Start the mDNS service
  if (MDNS.begin(eeprom_params.name)) 
    Serial.printf("mDNS service started with name %s\n", eeprom_params.name);

  // Start the web server
  server.on("/index.html", handleRoot_ap);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.printf("Web server started\n");
}

///////////////////////////////////////////////////////////////////////////////
// Try to connect to an access point. Return true when it is connected
///////////////////////////////////////////////////////////////////////////////
bool wifi_connection(const char ssid[], const char password[])
{
  static bool first_time = true;

  if (first_time)
  { 
    // Set up WiFi mode to station and connect to a WiFi network
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.printf("\nConnecting to %s ", ssid);
    first_time = false;
  }

  // Wait for 400 ms and blink status LED meanwhile
  if (WiFi.status() != WL_CONNECTED)
  {
    setStatusLed(ON);
    delay(200);
    setStatusLed(OFF);
    delay(200);
    Serial.printf(".");
  }

  return WiFi.status() == WL_CONNECTED;
}

///////////////////////////////////////////////////////////////////////////////
// ISR for the push button.
// If the push button is pressed for a short time (less than MIN_FACTORY_TIME)
// global variable restart is set. If the push button is pressed for a 
// long time (higher than MIN_FACTORY_TIME) global variable recover_factory is set.
// It assumes a released default state for the push button. 
// In addtion, is assumes that interrupts are generated by pressing or releasing
// the push button.
// Bouncing is considered to last MAX_BOUNCING_TIME ms at most.
///////////////////////////////////////////////////////////////////////////////
void IRAM_ATTR reset_button_isr()
{
  static bool wait_for_press = true; // True if waiting for a push button press
  static unsigned long int push_time = 0; // Time at the push button press
  static unsigned long int prev_int_time = 0; // Time at the previous interrupt
  unsigned long int curr_time = millis();  // Get the current time

  // If waiting for a push button press
  if (wait_for_press)
  {
    // We are waiting for a button press
    if (digitalRead(PUSH_BUTTON_PIN) != LOW)
      return;

    // Ignore any press until MAX_BOUNCING_TIME is elapsed from the previous interrupt
    if (curr_time - prev_int_time > MAX_BOUNCING_TIME)
    {
      push_time = curr_time; // We need this time to calculate the elapsed time afterwards
      prev_int_time = curr_time;
      wait_for_press = false; // Next we are waiting for a push button release
    }
  } 
  // If waiting for a push button release
  else
  {
    // Ignore any release until MAX_BOUNCING_TIME is elapsed from the previous press
    if (curr_time - prev_int_time > MAX_BOUNCING_TIME)
    {
      if ((curr_time - push_time) > MIN_FACTORY_TIME)
        invalidate_eeprom = true;
      else
        restart = true;
      wait_for_press = true; // We are waiting for a new push button press
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Arduino setup fuction
///////////////////////////////////////////////////////////////////////////////
void setup()
{
  // Configure the serial port
  Serial.begin(115200);
  
  // Initialize the sensor
  initTempHumiditySensor();

  // Set factory defaults and initialize the EEPROM
  setFactoryDefaults(factory_default_params);
  if (!EEPROM.begin(sizeof(eeprom_params_t)))
    Serial.printf("Failed initializing EEPROM\n");

  // Read parameters from EEPROM
  EEPROM.readBytes(0, &eeprom_params, sizeof(eeprom_params_t));

  // If parameters are invalid
  if (eeprom_params.validation_code != factory_default_params.validation_code)
  {
    setFactoryDefaults(eeprom_params);

    // Set up run mode as access point and run the web server
    run_mode = RUN_AS_AP;
    configure_as_ap_webserver();
  }
  // If parameters are valid
  else
  {
    // Print parameters and set up run mode as station
    Serial.printf("Stored SSID: %s;\tpassword: %s;\tName: %s\n", 
      eeprom_params.ssid_sta, eeprom_params.password_sta, eeprom_params.name);
    run_mode = RUN_AS_STA;
  }

  // Install reset_button_isr() as push button interrupt routine, fired by push button pin changes
  pinMode(PUSH_BUTTON_PIN, INPUT);
  attachInterrupt(PUSH_BUTTON_PIN, reset_button_isr, CHANGE);
 
  // Read web pages from the file system for the given run mode
  cache_web_content(run_mode);  
}

///////////////////////////////////////////////////////////////////////////////
// Arduino loop function
///////////////////////////////////////////////////////////////////////////////
void loop()
{
  // After a short press of the push button
  if (restart)
  {
    Serial.println("\nRestarting in 2 seconds...");
    delay(2000);
    ESP.restart();
  }

  // After a long press of the push button
  if (invalidate_eeprom)
  {
      // Write invalid validation code into EEPROM
      eeprom_params.validation_code = factory_default_params.validation_code + 1;
      EEPROM.writeBytes(0, &eeprom_params, sizeof(eeprom_params));
      EEPROM.commit();

      Serial.println("\nInvalidating EEPROM and restarting as AP in 2 seconds...");
      delay(2000);
      ESP.restart();
  }

  // If running as access point, handle web clients
  if (run_mode == RUN_AS_AP)
  {
    // Status LED blinks with a period of 1 second
    if ((millis() / 500) % 2)
      setStatusLed(ON);
    else
      setStatusLed(OFF);

    server.handleClient();
  }
  // If running as station, connect to a WiFi AP, run the web server and perform the required job
  else
  {
    if (WiFi.status() != WL_CONNECTED)
    { 
      if (wifi_connection(eeprom_params.ssid_sta, eeprom_params.password_sta))
      {
        setStatusLed(ON);
        Serial.printf("\nConnected with IP address: %s\n", WiFi.localIP().toString().c_str());

        // Start the mDNS service
        if (MDNS.begin(eeprom_params.name)) 
          Serial.printf("mDNS service started with name %s\n", eeprom_params.name);

        // Start the web server
        server.on("/index.html", handleRoot_sta);
        server.onNotFound(handleNotFound);
        server.begin();
        Serial.printf("Web server started\n");
      }
    }

    if (WiFi.status() == WL_CONNECTED)
      server.handleClient();

	// Run temperature/humidity control
    control_temp_hum();  
  }
}