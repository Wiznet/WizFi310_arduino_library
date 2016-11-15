#include "WizFi310.h"

char ssid[] = "DIR-815_Wiznet";    // your network SSID (name)
char pass[] = "12345678";          // your network password
int status = WL_IDLE_STATUS;       // the Wifi radio's status

char server[] = "arduino.cc";

// Initialize the Ethernet client object
WiFiClient client;

void printWifiStatus();

void setup()
{
    SerialUSB.begin(115200);
    while( !SerialUSB );

    Serial.begin(115200);
    Serial5.begin(9600);
    WiFi.init(&Serial5);

    // check for the presence of the shield
    if (WiFi.status() == WL_NO_SHIELD) {
        SerialUSB.println("WiFi shield not present");
        //SerialUSB.println("WiFi shield not present");
        // don't continue
        while (true);
    }

    // attempt to connect to WiFi network
    while ( status != WL_CONNECTED) {
        SerialUSB.print("Attempting to connect to WPA SSID: ");
        SerialUSB.println(ssid);
        // Connect to WPA/WPA2 network
        status = WiFi.begin(ssid, pass);
    }

    // you're connected now, so print out the data
    SerialUSB.println("You're connected to the network");
    
    printWifiStatus();
    
    SerialUSB.println();
    SerialUSB.println("Starting connection to server...");
    
    // if you get a connection, report back via serial
    if (client.connect(server, 80)) {
        SerialUSB.println("Connected to server");
        // Make a HTTP request
        client.println("GET /asciilogo.txt HTTP/1.1");
        client.println("Host: arduino.cc");
        client.println("Connection: close");
        client.println();
    }
}

void loop()
{
    // if there are incoming bytes available
    // from the server, read them and print them
    while (client.available()) {
        char c = client.read();
        SerialUSB.write(c);
    }

    // if the server's disconnected, stop the client
    if (!client.connected()) {
    SerialUSB.println();
    SerialUSB.println("Disconnecting from server...");
    client.stop();

    // do nothing forevermore
        while (true);
    }
}

void printWifiStatus()
{
    // print the SSID of the network you're attached to
    SerialUSB.print("SSID: ");
    SerialUSB.println(WiFi.SSID());
    
    // print your WiFi shield's IP address
    IPAddress ip = WiFi.localIP();
    SerialUSB.print("IP Address: ");
    SerialUSB.println(ip);
    
    // print the received signal strength
    long rssi = WiFi.RSSI();
    SerialUSB.print("Signal strength (RSSI):");
    SerialUSB.print(rssi);
    SerialUSB.println(" dBm");
}

