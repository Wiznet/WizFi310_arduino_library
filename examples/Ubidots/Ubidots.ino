#include <Ethernet2.h>
#include <SPI.h>
#include <SoftwareSerial.h>

// Ubidots My Source Variable ID
String TEST = "581acebd7625422cc55daf8d"; // TEST Variable ID

// Ubidots My token ID
String token = "UCdf332BAQMfXfSJjbBovWePelFU3M"; // default token

// MAC address
byte mac[] = {0x00, 0x08, 0xDC, 0x03, 0x04, 0x02};

// 접속할 서버
char server[] = "things.ubidots.com";
#define  REMOTE_PORT    80

IPAddress ip(222,98,173, 193); // TCP/IP 네트워크에 있는 호스트를 식별하는 숫자
IPAddress gateway(222,98,173, 254); // 다른 네트워크에 있는 호스트와 통신할 때 사용
IPAddress subnet(255, 255, 255, 192); // 호스트가 로컬 or 원격 네트워크에 있는지 확인할 때 사용
IPAddress myDns(8, 8, 8, 8); // google puble dns

EthernetClient client;

// getRequest String buffer
String Stringbuffer = ""; 

int TEST_pin = 13; // Arduino LED D13 pin

void setup() {
  SerialUSB.begin(9600);
  
  delay(6000);
  SerialUSB.print("Starting...");
  if (Ethernet.begin(mac) == 0) { // 공유기 DHCP 사용
    SerialUSB.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns, gateway, subnet);
  }
  for (byte i = 0; i < 4 ; i++) {
    SerialUSB.print(Ethernet.localIP()[i], DEC);
    SerialUSB.print(".");
  } 
  pinMode(TEST_pin, OUTPUT);
  digitalWrite(TEST_pin, LOW);
  delay(1000);
}

void loop() {
  sendValue();
  delay(2000);
}

void sendValue(void)
{
  SerialUSB.println("Sensor value Sending...");
  int bodySize = 0;
  delay(2000);
  
  String varString = "[{\"variable\": \"" + TEST +"\", \"value\":" + String(TEST_pin) + "}]";
  //SerialUSB.print(varString);
  bodySize = varString.length();
  delay(2000);
  //SerialUSB.println("Connecting...");

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
    SerialUSB.println("connection failed");
  }
  while (!client.available());
    //SerialUSB.println("Reading..");
  while (client.available())
  {
    char c = client.read();
    SerialUSB.print(c); // Response Monitoring
  }
  client.flush();
  client.stop();
}
