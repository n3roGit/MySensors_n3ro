#include <SPI.h>
#include <MySensor.h>  
#include <readVcc.h>

#define DIGITAL_INPUT_SOIL_SENSOR 3   // Digital input did you attach your soil sensor.  
#define INTERRUPT DIGITAL_INPUT_SOIL_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
#define CHILD_ID 0   // Id of the sensor child
#define NODE_ID 23                      // ID of node

#define MIN_V 1900 // empty voltage (0%)
#define MAX_V 3200 // full voltage (100%)

MySensor gw;
MyMessage msg(CHILD_ID, V_TRIPPED);
int lastSoilValue = -1;
int oldBatteryPcnt;
int repeat = 20;

void setup()  
{ 
  gw.begin(NULL, NODE_ID, false);

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Soil Moisture Sensor", "1.0");
  // sets the soil sensor digital pin as input
  pinMode(DIGITAL_INPUT_SOIL_SENSOR, INPUT);      
  // Register all sensors to gw (they will be created as child devices)  
  gw.present(CHILD_ID, S_MOTION);
}
 
void loop()     
{     
  // Read digital soil value
  int soilValue = digitalRead(DIGITAL_INPUT_SOIL_SENSOR); // 1 = Not triggered, 0 = In soil with water 
  if (soilValue != lastSoilValue) {
    Serial.println(soilValue);
    //gw.send(msg.set(soilValue==0?1:0));  // Send the inverse to gw as tripped should be when no water in soil
    resend(msg.set(soilValue==0?1:0), repeat);
    //Serial.println("---------- "soilValue);
    lastSoilValue = soilValue;
  }
  // Power down the radio and arduino until digital input changes.
  gw.sleep(INTERRUPT,CHANGE);
}

void resend(MyMessage &msg, int repeats)
{
  int repeat = 1;
  int repeatdelay = 0;
  boolean sendOK = false;

  while ((sendOK == false) and (repeat < repeats)) {
    if (gw.send(msg)) {
      sendOK = true;
    } else {
      sendOK = false;
      Serial.print("Send ERROR ");
      Serial.println(repeat);
      repeatdelay += 250;
    } repeat++; delay(repeatdelay);
  }
}

void sendBattery() // Measure battery
{
  int batteryPcnt = min(map(readVcc(), MIN_V, MAX_V, 0, 100), 100);
  if (batteryPcnt != oldBatteryPcnt) {
    gw.sendBatteryLevel(batteryPcnt); // Send battery percentage
    oldBatteryPcnt = batteryPcnt;
  }
  Serial.print("---------- Battery: ");
  Serial.println(batteryPcnt);
}
