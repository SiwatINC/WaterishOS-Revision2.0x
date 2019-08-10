#include <PubSubClient.h>
#include <MqttWildcard.h>
#include <PubSubClientTools.h>
#include <BearSSLHelpers.h>
#include <CertStoreBearSSL.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureAxTLS.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiServer.h>
#include <WiFiServerSecureAxTLS.h>
#include <WiFiServerSecureBearSSL.h>
#include <WiFiUdp.h>
#include "config.h"
#include <FlowMeter.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Thread.h>
#include <ThreadController.h>
Adafruit_MCP23017 mcp;
long tslr=0;
WiFiClient espClient;
PubSubClient client("siwatsystem.com", 1883, espClient);
PubSubClientTools mqtt(client);
ThreadController threadControl = ThreadController();
Thread mqttupdater = Thread();
Thread datacollector = Thread();
boolean online=true;
FlowMeter sensorA[6]= FlowMeter(1);
FlowMeter sensorB[6]= FlowMeter(3);
volatile boolean awakenByInterrupt = false;
void readA(){
  uint8_t pin=mcp.getLastInterruptPin();
  uint8_t val=mcp.getLastInterruptPinValue();
  for(int sid=0;sid<=5;sid++)sensorA[sid].count();
}
void readB(){
  uint8_t pin=mcp.getLastInterruptPin();
  uint8_t val=mcp.getLastInterruptPinValue();
  for(int sid=8;sid<13;sid++)sensorB[sid-8].count();
}
void collectdata(){
    for(int sid=0;sid<=5;sid++)sensorA[sid].tick(1000);
    for(int sid=8;sid<13;sid++)sensorB[sid-8].tick(1000);
}
void updatemqtt(){
    for(int counter;counter<=6;counter++)mqtt.publish("/waterishos/node"+nodename+"/flowrateA/"+counter,String(sensorA[counter].getCurrentFlowrate()));
    for(int counter;counter<=6;counter++)mqtt.publish("/waterishos/node"+nodename+"/volumeA/"+counter,String(sensorA[counter].getTotalVolume()));
    for(int counter;counter<=6;counter++)mqtt.publish("/waterishos/node"+nodename+"/flowrateB/"+counter,String(sensorB[counter].getCurrentFlowrate()));
    for(int counter;counter<=6;counter++)mqtt.publish("/waterishos/node"+nodename+"/volumeB/"+counter,String(sensorB[counter].getTotalVolume()));}
void setup(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int connectionattempt=0;
  while(WiFi.status() != WL_CONNECTED)
  {
    //wait for it ... (Wait for Wifi Connection)
    connectionattempt++;
    delay(500);
    if(connectionattempt>=20){
      online=false;
      break;
    }
  }
  attachInterrupt(digitalPinToInterrupt(1),readA,FALLING);
  attachInterrupt(digitalPinToInterrupt(3),readB,FALLING);
  pinMode(1, FUNCTION_3);
  pinMode(3, FUNCTION_3);
  pinMode(1,INPUT);
  pinMode(3,INPUT);
  mcp.begin();
  mcp.setupInterrupts(true,false,LOW);
  for(int i=0;i<=15;i++)
  {
    mcp.pinMode(i, INPUT);
    mcp.setupInterruptPin(i,RISING);
    }
  datacollector.onRun(collectdata);
  datacollector.setInterval(1000);
  mqttupdater.onRun(updatemqtt);
  mqttupdater.setInterval(1000);
  threadControl.add(&datacollector);
  threadControl.add(&mqttupdater);
  }

void loop(){
   client.loop();
   threadControl.run();
}
