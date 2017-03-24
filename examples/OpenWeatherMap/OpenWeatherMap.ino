#include <WizFi310.h>


#define API_key      "API_key_value"  // your API key

char ssid[] = "Network_SSID";       // your network SSID (name)
char pass[] = "Network_password";        // your network password
int status = WL_IDLE_STATUS;       // the Wifi radio's status

char server[] = "api.openweathermap.org"; //server address

// Initialize the client object
WiFiClient client;



char city[]="Seoul";  //your city
char country[]="kr";  //your country


unsigned long lastConnectionTime = 0;         // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 5000L;  // delay between updates, in milliseconds
                                              // wait 5sec
boolean getIsConnected = false;




void httpRequest();
void printWifiStatus();

void setup()
{
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
}

void loop() {

  String tempString="", weaString="", rcvbuf="";
  boolean tempComp=false, weaComp=false;
  boolean tempFlag=false,weaFlag=false;
  float tempVal=0;
  String weaVal="";
  char c;


  //Peridodically request connection to server only connection is closed. 
  if (((millis() - lastConnectionTime) > postingInterval) && (!getIsConnected)) {
    httpRequest();
  }

  //Close the connection in case that connection is maintained for a long time.
  else if((millis() - lastConnectionTime)> 20000L) {
    Serial.println("Time out! connection is closed......");
    client.stop();
    getIsConnected=false;
  }

  //Read the data after the connection to the server succeeds
  while (client.available()) {

      //Get the flag to extract Weather data
      if ( rcvbuf.endsWith("\"description\":\"")) {
            weaFlag=true;
            weaString="";
        }

    // Get the flag to extract temperature data
    if ( rcvbuf.endsWith("\"temp\":")) {
      tempFlag = true;
      tempString = "";
    }

    //Read the data of client object and store to buffer
    c = client.read();
    if ( c != NULL ) {
      rcvbuf += c;
    }



    if (weaFlag) {
      // Store the data into weaString until client.read data is '"'
      // Between "description":" and '"' is weather data.
      if (c != '\"' ) {
        weaString += c;
      }
      else {
        weaFlag = false;
        weaVal = weaString;
        weaComp=true;
      }
    }


    if (tempFlag) {
      // Store the data into tempString until client.read data is ','
      // Between \"temp\": and ',' is temperature data.
      if (c != ',' ) {
        tempString += c;
      }
      else {
        tempFlag = false;
        tempVal = tempString.toFloat() - 273.0;
        tempComp=true;
      }
    }
  }

  //Print the temperature data on serial monitor
  //It is executed only when temperature value is read successfully.
  if(tempComp && weaComp) {
      Serial.println("");
      Serial.println(F("==================="));
      Serial.print(F("Weather : "));
      Serial.println(weaVal);
      Serial.print(F("Temperature : "));
      Serial.println(tempVal);
      Serial.println(F("==================="));

      //Change the value to check if the value is updated
      tempVal=9999;
      weaVal="????";

      tempComp=false;
      weaComp=false;
      rcvbuf="";

      // close any connection before send a new request
      // this will free the socket on the WiFi shield
      client.stop();
      getIsConnected = false;

  }
}


// this method makes a HTTP connection to the server
void httpRequest() {
  Serial.println();

  // if there's a successful connection
  if (client.connect(server, 80)) {
    Serial.println("Connected...");

    // send the HTTP PUT request
    client.print("GET /data/2.5/weather?q=");
    client.print(city);
    client.print(",");
    client.print(country);
    client.print("&appid=");
    client.print(API_key);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();

    // note the time that the connection was made
    lastConnectionTime = millis();
    getIsConnected = true;
  }
  else {
    // if you couldn't make a connection
    Serial.println("Connection failed");
    getIsConnected = false;
  }
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
