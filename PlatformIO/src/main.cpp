#include <Arduino.h>
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
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
String token="h09-9d8fji4mp";
Adafruit_MCP23017 mcp;
long tslr = 0;
WiFiClient espClient;
MySQL_Connection conn((Client *)&espClient);
MySQL_Cursor cur = MySQL_Cursor(&conn);
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
volatile boolean awakenByInterrupt = false;
LiquidCrystal_I2C lcd(0x3F, 16, 2);
int menu;
void retrieveVOL(){
  char query[] = "SELECT volume1 FROM  WHERE token = '"+token+"' ORDER BY id DESC LIMIT 1";
  row_values *row = NULL;
  cur.execute(query);
  cur.get_columns();
  while (row != NULL) {
    row = cur.get_next_row();
    if (row != NULL) cuvolume1 = atol(row->values[0]);
  }
  query[] = "SELECT volume2 FROM  WHERE token = '"+token+"' ORDER BY id DESC LIMIT 1";
  *row = NULL;
  cur.execute(query);
  cur.get_columns();
  while (row != NULL) {
    row = cur.get_next_row();
    if (row != NULL)cuvolume2 = atol(row->values[0]);
  }
}
void writelcd(String line1, String line2){
  lcd.clear();
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
}
void updatelcd()
{
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
  char query[] = "INSERT INTO 'flowlog'(a,b,c) VALUES ('a','b','c')";
  cur.execute(query);
}
void setup() {
  lcd.init();
  writelcd(" Siwat INC (tm) ","  Waterish OS");
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int connectionattempt = 0;
  while (WiFi.status() != WL_CONNECTED && online)
  {
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
  writelcd("Boot Sequence P3","    Success!");
  delay(500);
  writelcd("Waterish OS s6.4","Reading  Sensors");
  delay(1000);
  writelcd(" Telemetry Node","siwatsystem.com");
  if (conn.connect(server_addr, 3306, mysqluser, mysqlpassword)) {
    writelcd(" Telemetry Node","Connected");
    delay(1000);
  } else {
    writelcd(" Telemetry Node"," Failed Offline");
    online=false;
    delay(3000);
  }
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
  if(online)client.loop();
  threadControl.run();
}