#include <LiquidCrystal_I2C.h>
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
#include "private.h"
#include <FlowMeter.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Thread.h>
#include <ThreadController.h>
Adafruit_MCP23017 mcp;
long tslr = 0;
WiFiClient espClient;
PubSubClient client("siwatsystem.com", 1883, espClient);
PubSubClientTools mqtt(client);
ThreadController threadControl = ThreadController();
Thread mqttupdater = Thread();
Thread datacollector = Thread();
Thread lcdmanager = Thread();
boolean online = true;
boolean sensorstate[12];
boolean lastsensorstate[12];
boolean firstrun = true;
FlowMeter sensor1 = FlowMeter(12);
FlowMeter sensor2 = FlowMeter(13);
volatile boolean awakenByInterrupt = false;
LiquidCrystal_I2C lcd(0x3F, 16, 2);
int menu;
void writelcd(String line1, String line2){
  lcd.clear();
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
}
void updatelcd()
{
  writelcd("1: "+String((int)sensor1.getCurrentFlowrate())+"L/h "+String(sensor1.getTotalVolume())+"L","2:"+String((int)sensor2.getCurrentFlowrate())+"L/h "+String(sensor2.getTotalVolume())+"L");
}
void ICACHE_RAM_ATTR read1() {
  sensor1.count();
}
void ICACHE_RAM_ATTR read2() {
  sensor2.count();
}
void collectdata() {
  sensor1.tick(1000);
  sensor2.tick(1000);
}
void updatemqtt() {
  mqtt.publish("/waterishos/node" + nodename + "/flowrate1", String(sensor1.getCurrentFlowrate()));
  mqtt.publish("/waterishos/node" + nodename + "/volume1", String(sensor1.getTotalVolume()));
  mqtt.publish("/waterishos/node" + nodename + "/flowrate2", String(sensor2.getCurrentFlowrate()));
  mqtt.publish("/waterishos/node" + nodename + "/volume2", String(sensor2.getTotalVolume()));
}
void setup() {
  lcd.begin();
  writelcd(" Siwat INC (tm) ","  Waterish OS");
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int connectionattempt = 0;
  while (WiFi.status() != WL_CONNECTED && online)
  {
    //wait for it ... (Wait for Wifi Connection)
    writelcd("WiFi Connecting","   Attempt "+String(connectionattempt));
    connectionattempt++;
    delay(500);
    if (connectionattempt >= 60) {
      writelcd(" Cannot Connect"," Going Offline!");
      online = false;
    }
  }
  String wifiname(ssid);
  if(online)writelcd(" WiFi Connected",wifiname);
  delay(3000);
  writelcd("Boot Sequence P3"," Loading Kernel");
  Serial.begin(115200);
  pinMode(14, INPUT_PULLUP);
  pinMode(12, INPUT);
  pinMode(13, INPUT);
  attachInterrupt(digitalPinToInterrupt(12), read1, RISING);
  attachInterrupt(digitalPinToInterrupt(13), read2, RISING);
  delay(1000);
  writelcd("Boot Sequence P3","Waking Processor");
  delay(1000);
  writelcd("   SETTINGS.H","co-processor:OFF");
  delay(3000);
  mcp.begin();
  for (int i = 0; i <= 15; i++)
  {
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, LOW);
    mcp.setupInterruptPin(i, RISING);
  }
  delay(1000);
  writelcd("Boot Sequence P3","    Success!");
  delay(2000);
  writelcd("Waterish OS a3.9","Reading  Sensors");
  delay(1000);
  writelcd(" Telemetry Node","siwatsystem.com");
  if (client.connect("waterishos",telemetryuser,telemetrykey)) {
    writelcd(" Telemetry Node","Connected");
    delay(1000);
  } else {
    writelcd(" Telemetry Node"," Failed Offline");
    online=false;
    delay(3000);
  }
  datacollector.onRun(collectdata);
  datacollector.setInterval(1000);
  if(online)mqttupdater.onRun(updatemqtt);
  if(online)mqttupdater.setInterval(1000);
  lcdmanager.onRun(updatelcd);
  lcdmanager.setInterval(250);
  threadControl.add(&datacollector);
  threadControl.add(&lcdmanager);
  if(online)threadControl.add(&mqttupdater);
  if(online)updatemqtt();
  
}

void loop() {
  if(online)client.loop();
  threadControl.run();
}
