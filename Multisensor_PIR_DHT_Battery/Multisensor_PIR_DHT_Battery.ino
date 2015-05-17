#include <MySensor.h>  
#include <readVcc.h>
#include <SPI.h>
#include <DHT.h> 

unsigned long SLEEP_TIME = 1200; // Sleep time between reports (in milliseconds)
#define DIGITAL_INPUT_SENSOR 3   // The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt!)
#define INTERRUPT DIGITAL_INPUT_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
#define CHILD_ID 1   // Id of the sensor child
#define CHILD_ID_HUM 2
#define CHILD_ID_TEMP 3
#define HUMIDITY_SENSOR_DIGITAL_PIN 2
#define NODE_ID 11          // ID of node

MySensor gw;
// Initialize motion message
MyMessage msg(CHILD_ID, V_TRIPPED);
DHT dht;
float lastTemp;
float lastHum;
boolean metric = true; 
int oldBatteryPcnt;
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);


int MIN_V = 2700; // empty voltage (0%)
int MAX_V = 3200; // full voltage (100%)

void setup()  
{  
  gw.begin(NULL,NODE_ID,false);

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Motion Sensor", "1.0");

  pinMode(DIGITAL_INPUT_SENSOR, INPUT);      // sets the motion sensor digital pin as input
  digitalWrite(DIGITAL_INPUT_SENSOR, HIGH);
  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID, S_MOTION);
  
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 

  // Send the Sketch Version Information to the Gateway
  gw.sendSketchInfo("Humidity", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID_HUM, S_HUM);
  gw.present(CHILD_ID_TEMP, S_TEMP);
  
  metric = gw.getConfig().isMetric;
  
}

void loop()     
{     
  // Read digital motion value
  boolean tripped = digitalRead(DIGITAL_INPUT_SENSOR) == HIGH; 
        
  Serial.println(tripped);
  gw.send(msg.set(tripped?"1":"0"));  // Send tripped value to gw 
 

  
  
  // Measure battery
  float batteryV = readVcc();
  int batteryPcnt = (((batteryV - MIN_V) / (MAX_V - MIN_V)) * 100 );
  if (batteryPcnt > 100) {
    batteryPcnt = 100;
  }
  
   if (batteryPcnt != oldBatteryPcnt) {
    gw.sendBatteryLevel(batteryPcnt); // Send battery percentage
    oldBatteryPcnt = batteryPcnt;
  }
  
    delay(dht.getMinimumSamplingPeriod());

  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT");
  } else if (temperature != lastTemp) {
    lastTemp = temperature;
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    gw.send(msgTemp.set(temperature, 1));
    Serial.print("T: ");
    Serial.println(temperature);
  }
  
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT");
  } else if (humidity != lastHum) {
      lastHum = humidity;
      gw.send(msgHum.set(humidity, 1));
      Serial.print("H: ");
      Serial.println(humidity);
  }
    // Sleep until interrupt comes in on motion sensor. Send update every two minute. 
  gw.sleep(INTERRUPT,CHANGE, SLEEP_TIME);
}
