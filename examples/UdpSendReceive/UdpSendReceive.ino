#include "WizFi310.h"
#include "WizFi310Udp.h"

char ssid[] = "DIR-815_Wiznet";    // your network SSID (name)
char pass[] = "12345678";          // your network password
int status = WL_IDLE_STATUS;       // the Wifi radio's status

unsigned int localPort = 10002;  // local port to listen on

char packetBuffer[255];          // buffer to hold incoming packet
char ReplyBuffer[] = "ACK";      // a string to send back

WiFiUDP Udp;

void printWifiStatus();

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
    Udp.begin(localPort);

    Serial.print("Listening on port ");
    Serial.println(localPort);
}

void loop() {
    // if there's data available, read a packet
    int packetSize = Udp.parsePacket();
    if(packetSize)
    {
        Serial.print("Received packet of size ");
        Serial.println(packetSize);
        Serial.print("From ");
        IPAddress remoteIp = Udp.remoteIP();
        Serial.print(remoteIp);
        Serial.print(", port ");
        Serial.println(Udp.remotePort());

        int len = Udp.read(packetBuffer, 200);
        if ( len > 0 ){
            packetBuffer[len] = '\0';
        }

        Serial.println(packetBuffer);

        // send a reply, to the IP address and port that sent us the packet we received
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        Udp.write(ReplyBuffer);
        Udp.endPacket();
    }
}


void printWifiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}


