#include <FlowMeter.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>
Adafruit_MCP23017 mcp;
byte ledPin=13;
byte InteruptPinA=1;
byte InteruptPinB=3;
byte arduinoInterrupt=1;
FlowMeter sensorA[6]= FlowMeter(1);
FlowMeter sensorB[6]= FlowMeter(3);
volatile boolean awakenByInterrupt = false;
void setup(){
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
  }
  // We will setup a pin for flashing from the int routine
  pinMode(ledPin, OUTPUT);
}

// The int handler will just signal that the int has happen
// we will do the work from the main loop.
void intCallBack(){
  awakenByInterrupt=true;
}

void handleInterrupt(){
  
  // Get more information from the MCP from the INT
  uint8_t pin=mcp.getLastInterruptPin();
  uint8_t val=mcp.getLastInterruptPinValue();
  for(int sid=0;sid<=5;sid++)sensorA[sid].count();
  for(int sid=6;sid<11;sid++)sensorA[sid-6].count();
  cleanInterrupts();

// handy for interrupts triggered by buttons
// normally signal a few due to bouncing issues
void loop(){
  attachInterrupt(arduinoInterrupt,intCallBack,RISING);
}
