#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
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
#include <HttpClient.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char token[] = "h09-9d8fji4mp";
const char backend[] = "siwatinc.com";
const int kNetworkTimeout = 30*1000;
Adafruit_MCP23017 mcp;
long tslr = 0;
WiFiClient espClient;
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
float volume1=0;
float volume2=0;
float cuvolume1=0;
float cuvolume2=0;
String apitoken=null;
volatile boolean awakenByInterrupt = false;
LiquidCrystal_I2C lcd(0x3F, 16, 2);
int menu;
HttpClient http(espClient);
bool shouldSaveConfig = false;
void retrieveVOL(){

}
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
void writelcd(String line1, String line2){
  lcd.clear();
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
}
void updatelcd(){
   volume1=cuvolume1+sensor1.getTotalVolume();
  volume2=cuvolume2+sensor2.getTotalVolume();
  writelcd("1: "+String((int)sensor1.getCurrentFlowrate())+"L/h "+String(volume1)+"L","2:"+String((int)sensor2.getCurrentFlowrate())+"L/h "+String(volume2)+"L");
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
  //UPDATE USING MySQL

}
void enterconfig(){
    writelcd("FlowOS","Wizard Running");
}
void setup() {
  lcd.init();
  writelcd(" Siwat INC (tm) ","  Waterish OS");
  delay(1000);
  SPIFFS.begin()
  if (SPIFFS.exists("/config.json")) {
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          strcpy(mqtt_server, json["token"]);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  WiFiManagerParameter token("token", "token", token, 40);
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&token);
  strcpy(apitoken, token.getValue());
  int connectionattempt = 0;
  online=true;
  wifiManager.setAPCallback(enterconfig);
  wifiManager.autoConnect("FlowOSnode");
  String wifiname(ssid);
  if(online)writelcd(" WiFi Connected",wifiname);
  writelcd("Boot Sequence P3"," Loading Kernel");
  Serial.begin(115200);
  pinMode(14, INPUT_PULLUP);
  pinMode(12, INPUT);
  pinMode(13, INPUT);
  attachInterrupt(digitalPinToInterrupt(12), read1, RISING);
  attachInterrupt(digitalPinToInterrupt(13), read2, RISING);
  writelcd("Boot Sequence P3","Waking Processor");
  writelcd("   SETTINGS.H","co-processor:OFF");
  mcp.begin();
  for (int i = 0; i <= 15; i++)
  {
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, LOW);
    mcp.setupInterruptPin(i, RISING);
  }
    if (shouldSaveConfig) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["token"] = token;
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
  }
  writelcd("Boot Sequence P3","    Success!");
  delay(500);
  writelcd("Waterish OS s6.4","Reading  Sensors");
  delay(1000);
  //MySQL Persist Object Retrival
  retrieveVOL();
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
  threadControl.run();
}