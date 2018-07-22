#include <WizFi310.h>
#include <WizFi310Udp.h>
#include <coap.h>


char ssid[] = "Network_SSID";       // your network SSID (name)
char pass[] = "Network_password";        // your network password
int status = WL_IDLE_STATUS;       // the Wifi radio's status


// CoAP client response callback
void callback_response(CoapPacket &packet, IPAddress ip, int port);

// UDP and CoAP class
WiFiUDP Udp;
Coap coap(Udp);

void printWifiStatus();

// CoAP client response callback
void callback_response(CoapPacket &packet, IPAddress ip, int port) {
  Serial.println("[Coap Response got]");

  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;

  Serial.println(p);
}

void setup() {
   // initialize serial for debugging
  Serial.begin(115200);
  Serial3.begin(115200);
  Serial.println(F("\r\nSerial Init"));

  WiFi.init(&Serial3);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  Serial.println("You're connected to the network");

  printWifiStatus();
  Serial.println();

  
  // client response callback.
  // this endpoint is single callback.
  Serial.println("Setup Response Callback");
  coap.response(callback_response);

  // start coap server/client
  coap.start();
}

void loop() {
  // send GET or PUT coap request to CoAP server.
  // To test, use libcoap, microcoap server...etc
  int msgid;
  int cdsVal=analogRead(A1);
  Serial.print("Cds value : ");
  Serial.println(cdsVal);
  if(cdsVal>=250) {
    msgid = coap.put(IPAddress(192, 168, 0, 70), 5683, "light", "1");
  }
  else {
    msgid = coap.put(IPAddress(192, 168, 0, 70), 5683, "light", "0");
  }
  
  Serial.println("Send Request");
  //int msgid = coap.get(IPAddress(XXX, XXX, XXX, XXX), 5683, "time");

  delay(1000);
  coap.loop();
}

void printWifiStatus() {
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

