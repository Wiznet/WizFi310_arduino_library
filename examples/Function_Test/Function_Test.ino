#include "WizFi310.h"

void setup() {
  // put your setup code here, to run once:
  SerialUSB.begin(115200);
  while(!SerialUSB)
  {
    ;
  }
  Serial5.begin(115200);
  WiFi.init(&Serial5);

  SerialUSB.println(WiFi.firmwareVersion());
  SerialUSB.println(WiFi.macAddress());
  
  WiFi.begin("DIR-815_Wiznet","12345678");
  
  SerialUSB.println(WiFi.localIP());
  SerialUSB.println(WiFi.subnetMask());
  SerialUSB.println(WiFi.gatewayIP());
}

void loop() {
  // put your main code here, to run repeatedly:
}
