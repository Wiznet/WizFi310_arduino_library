#include <Arduino.h>
#include "WizFi310.h"

char ssid[] = "Network SSID";                //your network SSID
char pass[] = "Network Password";               //your network password
int  status = WL_IDLE_STATUS;           //the Wifi radio's status

// Ubidots My Source Variable ID
String TEST = ""; // TEST Variable ID

// Ubidots My token ID
String token = "Your ubidots default token"; // default token

// Connect server
char server[] = "things.ubidots.com";
#define  REMOTE_PORT    80

WiFiClient client;

// getRequest String buffer
String Stringbuffer = ""; 

int TEST_pin = 13; // Arduino LED D13 pin

void setup() {
  Serial.begin(9600);
  Serial3.begin(115200);
  
  delay(6000);
  Serial.print("Starting...");
  Serial.println("WiFi initiallize");
  WiFi.init(&Serial3);
  
  if (WiFi.status() == 0) { 
    Serial.println("Failed to configure WiFi");
    // don't continue
    while(true)
    ;
  }
  //attempt to connect to WiFi network
  while (status != WL_CONNECTED)
  {
    Serial.print("[WiFi] Attempting to connect to WPA SSID: ");
    Serial.print(ssid);
    //Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }
  pinMode(TEST_pin, OUTPUT);
  digitalWrite(TEST_pin, LOW);
  delay(1000);
}

void loop() {
  sendValue();
  delay(20000);
}

void sendValue(void)
{
  Serial.println("Sensor value Sending...");
  int bodySize = 0;
  delay(20000);
  
  String varString = "[{\"variable\": \"" + TEST +"\", \"value\":" + String(TEST_pin) + "}]";
  Serial.print(varString);
  bodySize = varString.length();
  delay(20000);

  if (client.connect(server, REMOTE_PORT))
  {
    client.print("POST /api/v1.6/collections/values/?force=true HTTP/1.1\r\n");
    client.print("Host: things.ubidots.com\r\n");
    client.print("User-Agent: Arduino-Ethernet/1.0\r\n");
    client.print("X-Auth-Token: ");
    client.print(token);
    client.print("\r\n");
    client.print("Content-Length:");
    client.print(String(bodySize));
    client.print("\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print("Connection: close\r\n");
    client.print("\r\n");
    client.print(varString);
    client.print("\r\n\r\n");
  }
  else
  {
    Serial.println("connection failed");
  }
  while (!client.available());
    //Serial3.println("Reading..");
  while (client.available())
  {
    char c = client.read();
    Serial.print(c); // Response Monitoring
  }
  client.flush();
  client.stop();
}
