#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

#define LED_BUILTIN 2 

#define AWS_IOT_PUBLISH_TOPIC   "$aws/things/ESP32_LED/shadow/update"
#define AWS_IOT_SUBSCRIBE_TOPIC "$aws/things/ESP32_LED/shadow/update/delta"

int msgReceived = 0;
String rcvdPayload;
char sndPayloadOff[512];
char sndPayloadOn[512];

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("");
  Serial.println("###################### Starting Execution ########################");
  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  client.onMessage(messageHandler);

  Serial.println("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void messageHandler(String &topic, String &payload) {
  msgReceived = 1;
  rcvdPayload = payload;
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  
  sprintf(sndPayloadOn,"{\"state\": { \"reported\": { \"status\": \"on\" } }}");
  sprintf(sndPayloadOff,"{\"state\": { \"reported\": { \"status\": \"off\" } }}");
  
  connectAWS();
  
  Serial.println("Setting Lamp Status to Off");
  client.publish(AWS_IOT_PUBLISH_TOPIC, sndPayloadOff);
  
  Serial.println("##############################################");
}

void loop() {
  if(msgReceived == 1)
    {
        delay(100);
        msgReceived = 0;
        Serial.print("Received Message:");
        Serial.println(rcvdPayload);
        StaticJsonDocument<200> doc;
        DeserializationError error_sensor = deserializeJson(doc, rcvdPayload);
        const char *stat = doc["state"]["status"];
 
        Serial.print("AWS Says:");
        Serial.println(stat); 
        
        if(strcmp(stat, "on") == 0)
        {
         Serial.println("IF CONDITION");
         Serial.println("Turning Lamp On");
         digitalWrite(LED_BUILTIN, HIGH);
         client.publish(AWS_IOT_PUBLISH_TOPIC, sndPayloadOn);
        }
        else 
        {
         Serial.println("ELSE CONDITION");
         Serial.println("Turning Lamp Off");
         digitalWrite(LED_BUILTIN, LOW);
         client.publish(AWS_IOT_PUBLISH_TOPIC, sndPayloadOff);
        }
		Serial.println("##############################################");
    }
  client.loop();
}