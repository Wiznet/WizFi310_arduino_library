#include "WizFi310.h"

char ssid[] = "DIR-815_Wiznet";    // your network SSID (name)
char pass[] = "12345678";          // your network password
int status = WL_IDLE_STATUS;       // the Wifi radio's status

int reqCount = 0;                // number of requests received

WiFiServer server(80);

void setup()
{
    SerialUSB.begin(115200);
    delay(6000);
    //while( !SerialUSB );

    Serial5.begin(9600);
    while( !Serial5 );
    
    SerialUSB.println("START APPLICATION");
    WiFi.init(&Serial5);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    SerialUSB.println("WiFi shield not present");
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

  SerialUSB.println("You're connected to the network");
  printWifiStatus();
  
  // start the web server on port 80
  server.begin();
}


void loop()
{
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    SerialUSB.println("New client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        SerialUSB.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          SerialUSB.println("Sending response");
          
          // send a standard http response header
          // use \r\n instead of many println statements to speedup data send
          client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  // the connection will be closed after completion of the response
            "Refresh: 20\r\n"        // refresh the page automatically every 20 sec
            "\r\n");
          client.print("<!DOCTYPE HTML>\r\n");
          client.print("<html>\r\n");
          client.print("<h1>Hello World!</h1>\r\n");
          client.print("Requests received: ");
          client.print(++reqCount);
          client.print("<br>\r\n");
          client.print("Analog input A0: ");
          client.print(analogRead(0));
          client.print("<br>\r\n");
          client.print("</html>\r\n");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(10);

    // close the connection:
    client.stop();
    SerialUSB.println("Client disconnected");
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
  
  // print where to go in the browser
  SerialUSB.println();
  SerialUSB.print("To see this page in action, open a browser to http://");
  SerialUSB.println(ip);
  SerialUSB.println();
}
