#include <MySensor.h>
#include <readVcc.h>
#include <SPI.h>
#include <Wire.h>
#include <SI7021.h>

#define NODE_ID 26                      // ID of node
#define SLEEP_TIME 1800000                 // Sleep time between reports (in milliseconds)
// 1h = 3600000
// 30m = 1800000
// 15m = 900000

#define CHILD_ID_PIR 1                   // ID of the sensor PIR
#define CHILD_ID_HUM 2                   // ID of the sensor HUM
#define CHILD_ID_TEMP 3                  // ID of the sensor TEMP
#define CHILD_ID_LIGHT 4                 // ID of the sensor LIGHT

#define PIR_SENSOR_DIGITAL 3             // PIR pin
#define HUMIDITY_SENSOR_DIGITAL_PIN 4    // DHT pin
#define LIGHT_SENSOR_ANALOG_PIN 0        // LDR pin
#define LED_PIN A1                       // LED connected PIN 
#define STEPUP_PIN 6                     // StepUp Transistor 
#define STEPUP_PIN2 7                    // StepUp Transistor

#define MIN_V 1900 // empty voltage (0%)
#define MAX_V 3200 // full voltage (100%)

MySensor gw;
// Initialize Variables
MyMessage msgPir(CHILD_ID_PIR, V_TRIPPED);
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgLight(CHILD_ID_LIGHT, V_LIGHT_LEVEL);

// testing
SI7021 si7021;


float lastTemp;
float lastHum;
boolean metric = true;
int oldBatteryPcnt;
int lastLightLevel;
int sentValue;
int repeatSending = 10;

void setup()
{
  gw.begin(NULL, NODE_ID, false);

  gw.sendSketchInfo("PIR,si7021,Light", "1.0");
  // Register all sensors to gateway (they will be created as child devices)
  gw.present(CHILD_ID_PIR, S_MOTION);
  gw.present(CHILD_ID_HUM, S_HUM);
  gw.present(CHILD_ID_TEMP, S_TEMP);
  gw.present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);

  // PIR PINS
  pinMode(PIR_SENSOR_DIGITAL, INPUT);
  //digitalWrite(PIR_SENSOR_DIGITAL, HIGH); //Use this if no external pullup is used for PIR

  //si7021 PINS
  si7021.begin();
  
  //STEPUP PINS
  pinMode(STEPUP_PIN, OUTPUT);       // sets the pin as output
  pinMode(STEPUP_PIN2, OUTPUT);      // sets the pin as output
  
  //STARTUP BLINK
  led(true,5,200);
  
  
}

void loop()
{
  Serial.println("Waking up...");
  sendPir();
  sendLight();
  sendTemp();
  sendHum();
  sendBattery();

  Serial.println("Going to sleep...");
  Serial.println("");
  gw.sleep(PIR_SENSOR_DIGITAL - 2, RISING, SLEEP_TIME);
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



void sendPir() // Get value of PIR
{
  int value = digitalRead(PIR_SENSOR_DIGITAL); // Get value of PIR
  if (value != sentValue) { // If status of PIR has changed
    //gw.send(msgPir.set(value == HIGH ? 1 : 0)); // Send PIR status to gateway
    resend((msgPir.set(value == HIGH ? 1 : 0)),repeatSending);
    sentValue = value;
  }
  Serial.print("---------- PIR: ");
  Serial.println(value ? "tripped" : "not tripped");
}



void sendTemp() // Get temperature
{
  float temperature = si7021.getCelsiusHundredths();
  if (isnan(temperature)) {
    Serial.println("Failed reading temperature from DHT");
  } else {
    if (temperature != lastTemp) {
      //gw.send(msgTemp.set(temperature, 1));
      resend((msgTemp.set(temperature, 1)),repeatSending);
      lastTemp = temperature;
    }
    Serial.print("---------- Temp: ");
    Serial.println(temperature);
  }
}


void sendHum() // Get humidity
{
  float humidity = si7021.getHumidityPercent();
  if (isnan(humidity)) {
    Serial.println("Failed reading humidity from DHT");
  } else {
    if (humidity != lastHum) {
      //gw.send(msgHum.set(humidity, 1));
      resend((msgHum.set(humidity, 1)),repeatSending);
      lastHum = humidity;
    }
    Serial.print("---------- Humidity: ");
    Serial.println(humidity);
  }
}


void sendLight() // Get light level
{
  int lightLevel = (1023 - analogRead(LIGHT_SENSOR_ANALOG_PIN)) / 10.23;
  if (lightLevel != lastLightLevel) {
    //gw.send(msgLight.set(lightLevel));
    resend((msgLight.set(lightLevel)),repeatSending);
    lastLightLevel = lightLevel;
  }
  Serial.print("---------- Light: ");
  Serial.println(lightLevel);
}

void led(boolean onoff, int blink, int time) // LED Signal
{
  pinMode(LED_PIN, OUTPUT);      // sets the pin as output
  Serial.print("---------- LED: ");
  if (blink == 0)
  {
    if (onoff == true)
    {
      Serial.println("ON");
      digitalWrite(LED_PIN, LOW);      // turn on
    }
    else
    {
      Serial.println("OFF");
      digitalWrite(LED_PIN, HIGH);       // turn off
    }
  }
  else
  {
    if (time == 0) {time = 100;}
    Serial.print("Blink ");
    Serial.print(blink);
    Serial.print(" Delay ");
    Serial.println(time);
    for (int count = 0; count < blink; count++)
    {
      digitalWrite(LED_PIN, LOW);      // turn on
      delay(time);
      digitalWrite(LED_PIN, HIGH);       // turn off
      delay(time);
    }
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
      led(true,1,5);
    } repeat++; delay(repeatdelay);
  }
}



