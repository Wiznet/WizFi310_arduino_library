#include <Arduino.h>
#include "WizFi310.h"
#include "PubSubClient.h"

//char  ssid[] = "Matthew";         //your network SSID
//char  pass[] = "00000000";        //your network password

char  ssid[] = "Matthew";         //your network SSID
char  pass[] = "00000000";        //your network password
int   status = WL_IDLE_STATUS;    //the WiFi radio's status

//Connect server
char  server[] = "broker.mqtt-dashboard.com";

WiFiClient  WizFiClient;
PubSubClient client(WizFiClient);

long    lastMSG = 0;
char    msg[50];
int     value = 0;

void  setup_WiFi();
void  callback(char* topic, byte* payload, unsigned int length);
void  reconnect();

void setup() {
  Serial.begin(9600);
  Serial3.begin(115200);
  delay(6000);
  Serial.print("Starting...");
  Serial.println("WiFi initiallize");
  WiFi.init(&Serial3);

  if(WiFi.status() == 0)
  {
    Serial.println("Failed to configure WiFi");
    while(true)
    ;
  }

  while(status !=WL_CONNECTED)
  {
    Serial.print("[WiFi] Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    //Connect to WPA/WPA2 network
//  Serial.print(pass);
    status = WiFi.begin(ssid, pass);
//    Serial.println(status);
  }
  Serial.println("moyo");
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);
  delay(1000);
  
//  setup_WiFi();
//  Serial.println("moyo");
  client.setServer(server, 1883);
  client.setCallback(callback);
}

void loop() {

  if(!client.connected())
  {
    reconnect();
  }
  client.loop();

  long now = millis();
  if(now - lastMSG > 2000)
  {
    lastMSG = now;
    ++value;
    snprintf(msg, 75, "Start WIZFI310 Hello world #%ld", value);
    Serial.print("Publish MeSsaGe: ");
    Serial.println(msg);
    client.publish("outTopic", msg);    
  }
}

void  callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("MeSsaGe arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if((char)payload[0] == '1')
  {
    digitalWrite(9, LOW);
  }
  else
  {
    digitalWrite(9, HIGH);
  }  
}

void reconnect()
{
  while(!client.connected())
  {
    Serial.print("Attempting MQTT connection.....");
    //Attempt to connect
    if(client.connect("WizFiClient")){
      Serial.println("connected");
      
      //Publish and subscribe
      client.publish("outTopic", "Start WIZFI310 Hello world");
      client.subscribe("inTopic");
    }
    else
    {
      Serial.println(client.connected());
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);      
    }
  }
}

