#include "WizFi310.h"

char ssid[] = "";    // your network SSID (name)
char pass[] = "";    // your network password
int status = WL_IDLE_STATUS;       // the Wifi radio's status

char server[] = "arduino.cc";

WiFiClient client;

void setup() {
    Serial.begin(115200);
    Serial3.begin(115200);
    WiFi.init(&Serial3);

    // check for the presence of the shield
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("WiFi shield not present");
        //SerialUSB.println("WiFi shield not present");
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
    // you're connected now, so print out the data
    Serial.println("You're connected to the network");

    printWifiStatus();

    Serial.println();
    Serial.println("Starting connection to server...");

    // if you get a connection, report back via serial:
    if (client.connectSSL(server, 443)) {
        Serial.println("Connected to server");
            // Make a HTTP request:
        client.println("GET /asciilogo.txt HTTP/1.1");
        client.println("Host: www.arduino.cc");
        client.println("Connection: close");
        client.println();
        Serial.println("Request sent");
    }
}

void loop() {
    // if there are incoming bytes available
    // from the server, read them and print them
    while (client.available()) {
        int c = client.read();
        if( c != -1 )
        {
            if ( (c >= 0x20 && c <= 0x7E) || c == 0x0d || c == 0x0a )
            {
                Serial.write((char)c);
            }
            else
            {
                Serial.print('{');
                Serial.print((char)c,HEX);
                Serial.print('}');
            }
        }
    }

    // if the server's disconnected, stop the client
    if (!client.connected()) {
        Serial.println();
        Serial.println("Disconnecting from server...");
        client.stop();

        // do nothing forevermore
        while (true);
    }
}

void printWifiStatus()
{
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

