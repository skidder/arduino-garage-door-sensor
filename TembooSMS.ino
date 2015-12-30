#include <SPI.h>
#include <Adafruit_CC3000.h>
#include <Adafruit_CC3000_Server.h>
#include <ccspi.h>
#include <Client.h>
#include <Temboo.h>
#include "TembooAccount.h" // Contains Temboo account information

#define ADAFRUIT_CC3000_IRQ 2
#define ADAFRUIT_CC3000_VBAT 7
#define ADAFRUIT_CC3000_CS 10

#define DOOR_OPEN 1
#define DOOR_CLOSED 2

#define SWITCH_DOOR_OPEN LOW
#define SWITCH_DOOR_CLOSED HIGH

Adafruit_CC3000 cc3k = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT);

Adafruit_CC3000_Client client;

int doorState = DOOR_CLOSED;

void setup() {
  Serial.begin(115200);

  // For debugging, wait until the serial console is connected
  delay(4000);

  status_t wifiStatus = STATUS_DISCONNECTED;
  while (wifiStatus != STATUS_CONNECTED) {
    Serial.print("WiFi:");
    if (cc3k.begin()) {
      if (cc3k.connectToAP(WIFI_SSID, WPA_PASSWORD, WLAN_SEC_WPA2)) {
        wifiStatus = cc3k.getStatus();
      }
    }
    if (wifiStatus == STATUS_CONNECTED) {
      Serial.println("OK");
    } else {
      Serial.println("FAIL");
    }
    delay(5000);
  }

  cc3k.checkDHCP();
  delay(1000);

  // Initialize pin
  pinMode(8, INPUT);
  Serial.println("Setup complete.\n");
}

void loop() {
  int sensorValue = digitalRead(8);
  bool sendSMS = false;

  if (sensorValue == SWITCH_DOOR_OPEN && doorState == DOOR_CLOSED) {
    doorState = DOOR_OPEN;
    sendSMS = true;
    Serial.println("Door opened, sending SMS");
  } else if (sensorValue == SWITCH_DOOR_CLOSED && doorState == DOOR_OPEN) {
    doorState = DOOR_CLOSED;
    sendSMS = true;
    Serial.println("Door closed, sending SMS");
  }

  if (sendSMS) {
    TembooChoreo SendSMSChoreo(client);

    // Invoke the Temboo client
    SendSMSChoreo.begin();

    // Set Temboo account credentials
    SendSMSChoreo.setAccountName(TEMBOO_ACCOUNT);
    SendSMSChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
    SendSMSChoreo.setAppKey(TEMBOO_APP_KEY);

    // Set profile to use for execution
    if (doorState == DOOR_OPEN) {
      SendSMSChoreo.setProfile("TwilioGarageOpenedSMS");
    } else {
      SendSMSChoreo.setProfile("TwilioGarageClosedSMS");
    }

    // Identify the Choreo to run
    SendSMSChoreo.setChoreo("/Library/Twilio/SMSMessages/SendSMS");

    // Run the Choreo; when results are available, print them to serial
    SendSMSChoreo.run();

    while (SendSMSChoreo.available()) {
      char c = SendSMSChoreo.read();
      Serial.print(c);
    }
    SendSMSChoreo.close();

    Serial.println("\nWaiting...\n");
    delay(5000); // wait 5 seconds between SendSMS calls
  }
}
