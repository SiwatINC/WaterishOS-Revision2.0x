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

// Two pins at the MCP (Ports A/B where some buttons have been setup.)
// Buttons connect the pin to grond, and pins are pulled up.
byte mcpPinA=7;
byte mcpPinB=15;

void setup(){
  pinMode(1, FUNCTION_3); 
  pinMode(3, FUNCTION_3); 

  pinMode(arduinoIntPin,INPUT);
  mcp.begin();      // use default address 0
  mcp.setupInterrupts(true,false,LOW);
  // configuration for a button on port A
  mcp.pinMode(mcpPinA, INPUT);
 // mcp.pullUp(mcpPinA, HIGH);  // turn on a 100K pullup internally
  mcp.setupInterruptPin(mcpPinA,RISING); 
  mcp.pinMode(mcpPinB, INPUT);
 // mcp.pullUp(mcpPinB, HIGH);  // turn on a 100K pullup internall
  mcp.setupInterruptPin(mcpPinB,RISING);

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
  
  // We will flash the led 1 or 2 times depending on the PIN that triggered the Interrupt
  // 3 and 4 flases are supposed to be impossible conditions... just for debugging.
  for(int sid=0;sid<=7)s
  // simulate some output associated to this
  for(int i=0;i<flashes;i++){  
    delay(100);
    digitalWrite(ledPin,HIGH);
    delay(100);
    digitalWrite(ledPin,LOW);
  }

  // we have to wait for the interrupt condition to finish
  // otherwise we might go to sleep with an ongoing condition and never wake up again.
  // as, an action is required to clear the INT flag, and allow it to trigger again.
  // see datasheet for datails.
  while( ! (mcp.digitalRead(mcpPinB) && mcp.digitalRead(mcpPinA) ));
  // and clean queued INT signal
  cleanInterrupts();
}

// handy for interrupts triggered by buttons
// normally signal a few due to bouncing issues
void cleanInterrupts(){
  EIFR=0x01;
  awakenByInterrupt=false;
}  
/**
 * main routine: sleep the arduino, and wake up on Interrups.
 * the LowPower library, or similar is required for sleeping, but sleep is simulated here.
 * It is actually posible to get the MCP to draw only 1uA while in standby as the datasheet claims,
 * however there is no stadndby mode. Its all down to seting up each pin in a way that current does not flow.
 * and you can wait for interrupts while waiting.
 */
void loop(){
  attachInterrupt(arduinoInterrupt,intCallBack,FALLING);
  while(!awakenByInterrupt);
  // disable interrupts while handling them.
  detachInterrupt(arduinoInterrupt);
  if(awakenByInterrupt) handleInterrupt();
}
