#include <SPI.h>
#include <MySensor.h>
#include <readVcc.h>

#define DIGITAL_INPUT_SOIL_SENSOR 3   // Digital input did you attach your soil sensor.  
#define ANALOG_INPUT_LEAFWETNESS_SENSOR 0   // Digital input did you attach your soil sensor.

#define CHILD_ID_Digital 0   // Id of the sensor child
#define CHILD_ID_Analog 1   // Id of the sensor child
#define NODE_ID 23                      // ID of node

#define MIN_V 1900 // empty voltage (0%)
#define MAX_V 3200 // full voltage (100%)

MySensor gw;
unsigned long SLEEP_TIME = 5 * 1000; // sleep time between reads (seconds * 1000 milliseconds)
MyMessage msgDigital(CHILD_ID_Digital, V_TRIPPED);
MyMessage msgAnalog(CHILD_ID_Analog, V_LIGHT_LEVEL);
int lastSoilValue = -1;
int lastsoilValueAnalog = -1;
int oldBatteryPcnt;
int repeat = 1;

void setup()
{
  gw.begin(NULL, NODE_ID, false);

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Soil Moisture Sensor", "1.0");
  // sets the soil sensor digital pin as input
  pinMode(DIGITAL_INPUT_SOIL_SENSOR, INPUT);
  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID_Analog, S_MOTION);
  gw.present(CHILD_ID_Digital, S_LIGHT_LEVEL);
}

void loop()
{
  Serial.println("Waking up...");

  readMoistureDigital();
  readMoistureAnalog();


  Serial.println("Going to sleep...");
  Serial.println("");
  gw.sleep(DIGITAL_INPUT_SOIL_SENSOR - 2, CHANGE, SLEEP_TIME);
}

void readMoistureDigital()
{
  // Read digital soil value
  int soilValue = digitalRead(DIGITAL_INPUT_SOIL_SENSOR); // 1 = Not triggered, 0 = In soil with water
  if (soilValue != lastSoilValue) {
    resend(msgDigital.set(soilValue == 0 ? 1 : 0), repeat);
    lastSoilValue = soilValue;
    Serial.print("---------- Moisture : ");
    Serial.println(soilValue ? "dry" : "wet");

  }
}

void readMoistureAnalog()
{
  // Read analog soil value
  int soilValueAnalog = ((float)analogRead(ANALOG_INPUT_LEAFWETNESS_SENSOR) * 100 / 1023);
  if (soilValueAnalog != lastsoilValueAnalog) {
    Serial.println(soilValueAnalog);
    resend(msgAnalog.set(soilValueAnalog), repeat);  // Send the inverse to gw as tripped should be when no water in soil
    lastsoilValueAnalog = soilValueAnalog;
    Serial.print("---------- Moisture level : ");
    Serial.println(soilValueAnalog);
  }
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
