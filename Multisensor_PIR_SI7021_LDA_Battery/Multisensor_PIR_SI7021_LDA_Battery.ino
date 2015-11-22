#include <MySensor.h>
#include <readVcc.h>
#include <SPI.h>
#include <Wire.h>
#include <SI7021.h>

//#define DEBUG                        //Enable or Disable Debugging
#ifdef DEBUG
#define DEBUG_SERIAL(x) Serial.begin(x)
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_SERIAL(x)
#define DEBUG_PRINT(x) 
#define DEBUG_PRINTLN(x) 
#endif

#define NODE_ID 25                      // ID of node
#define SLEEP_TIME 1800000                 // Sleep time between reports (in milliseconds)
// 1h = 3600000
// 30m = 1800000
// 15m = 900000

#define FORCE_TRANSMIT_CYCLE 36  // 5min*12=1/hour, 5min*36=1/3hour 
#define BATTERY_REPORT_CYCLE 2880   // Once per 5min   =>   12*24*7 = 2016 (one report/week)


//Node interrupt wakeup modes
#define wakeUpMode RISING //RISING, CHANGE or FALLING

#define CHILD_ID_PIR 1                   // ID of the sensor PIR
#define CHILD_ID_HUM 2                   // ID of the sensor HUM
#define CHILD_ID_TEMP 3                  // ID of the sensor TEMP
#define CHILD_ID_LIGHT 4                 // ID of the sensor LIGHT

#define PIR_SENSOR_DIGITAL 3             // PIR pin
//#define HUMIDITY_SENSOR_DIGITAL_PIN 4    // DHT pin
#define LIGHT_SENSOR_ANALOG_PIN 0        // LDR pin
#define LED_PIN A1                       // LED connected PIN 
//#define STEPUP_PIN 6                     // StepUp Transistor 
//#define STEPUP_PIN2 7                    // StepUp Transistor


// Temp and Hum
SI7021 si7021;
#define HUMI_TRANSMIT_THRESHOLD 3.0  // THRESHOLD tells how much the value should have changed since last time it was transmitted.
#define TEMP_TRANSMIT_THRESHOLD 0.5
#define AVERAGES 2

#define MIN_V 1900 // empty voltage (0%)
#define MAX_V 3200 // full voltage (100%)

MySensor gw;
// Initialize Variables
MyMessage msgPir(CHILD_ID_PIR, V_TRIPPED);
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgLight(CHILD_ID_LIGHT, V_LIGHT_LEVEL);






float lastTemp = -100;
float lastHum = -100;
int batteryReportCounter = BATTERY_REPORT_CYCLE - 1;  // to make it report the first time.
int measureCount = 0;
//boolean metric = true;
int oldBatteryPcnt;
int lastLightLevel = -100;
int lastPIR = -1;
int repeatSending = 10;


void setup()
{
  DEBUG_SERIAL(115200);    
  DEBUG_PRINTLN("Serial started");
  
  DEBUG_PRINT("Voltage: ");
  DEBUG_PRINT(readVcc()); 
  DEBUG_PRINTLN(" mV");

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

  //si7021 initial
  si7021.begin();
  
  //STEPUP PINS
  //pinMode(STEPUP_PIN, OUTPUT);       // sets the pin as output
  //pinMode(STEPUP_PIN2, OUTPUT);      // sets the pin as output
  
  //STARTUP BLINK
  led(true,5,200); 
}

void loop()
{
  DEBUG_PRINTLN("Waking up...");
  measureCount ++;
  batteryReportCounter ++;
  bool forceTransmit = false;
  
  if (measureCount > FORCE_TRANSMIT_CYCLE) {
    forceTransmit = true; 
    measureCount = 0;
  }
  
  sendPir(forceTransmit);
  sendLight(forceTransmit);
  sendTemp(forceTransmit);
  sendHum(forceTransmit);
  
  sendBattery();
  
  DEBUG_PRINTLN("Going to sleep...");
  DEBUG_PRINTLN("");
  gw.sleep(5000);
  gw.sleep(PIR_SENSOR_DIGITAL - 2, wakeUpMode, SLEEP_TIME);
}


void sendBattery() // Measure battery
{
  bool force = false;
  int batteryPcnt = min(map(readVcc(), MIN_V, MAX_V, 0, 100), 100);
  if (batteryReportCounter >= BATTERY_REPORT_CYCLE) {
    force = true;
    batteryReportCounter = 0;
    }
  if (batteryPcnt != oldBatteryPcnt or force == true) {
    DEBUG_PRINT(force ? "forced " : "normal ");
    gw.sendBatteryLevel(batteryPcnt); // Send battery percentage
    oldBatteryPcnt = batteryPcnt;
  }
  DEBUG_PRINT("---------- Battery: ");
  DEBUG_PRINTLN(batteryPcnt);
}



void sendPir(bool force) // Get value of PIR
{
  int pir = digitalRead(PIR_SENSOR_DIGITAL); // Get value of PIR
  if (pir != lastPIR or force == true) { // If status of PIR has changed
    //gw.send(msgPir.set(pir == HIGH ? 1 : 0)); // Send PIR status to gateway
    resend((msgPir.set(pir == HIGH ? 1 : 0)),repeatSending,force);
    lastPIR = pir;
  }
  DEBUG_PRINT("---------- PIR: ");
  DEBUG_PRINTLN(pir ? "tripped" : "not tripped");
}



void sendTemp(bool force) // Get temperature
{
  float temperature = si7021.getCelsiusHundredths();
  temperature = temperature / 100;
  float diffTemp = abs(lastTemp - temperature);
  if (isnan(temperature)) {
    DEBUG_PRINTLN("Failed reading temperature from si7021");
  } else {
    if (diffTemp > TEMP_TRANSMIT_THRESHOLD or force == true) {
      //gw.send(msgTemp.set(temperature, 1));
      resend((msgTemp.set(temperature, 1)),repeatSending,force);
      lastTemp = temperature;
    }
    DEBUG_PRINT("---------- Temp: ");
    DEBUG_PRINTLN(temperature);
  }
}


void sendHum(bool force) // Get humidity
{
  float humidity = si7021.getHumidityPercent();
  float diffHum = abs(lastHum - humidity);
  if (isnan(humidity)) {
    DEBUG_PRINTLN("Failed reading humidity from si7021");
  } else {
    if (diffHum > HUMI_TRANSMIT_THRESHOLD or force == true) {
      //gw.send(msgHum.set(humidity, 1));
      resend((msgHum.set(humidity, 1)),repeatSending,force);
      lastHum = humidity;
    }
    DEBUG_PRINT("---------- Humidity: ");
    DEBUG_PRINTLN(humidity);
  }
}


void sendLight(bool force) // Get light level
{
  int lightLevel = (1023 - analogRead(LIGHT_SENSOR_ANALOG_PIN)) / 10.23;
  if (lightLevel != lastLightLevel or force == true) {
    //gw.send(msgLight.set(lightLevel));
    resend((msgLight.set(lightLevel)),repeatSending,force);
    lastLightLevel = lightLevel;
  }
  DEBUG_PRINT("---------- Light: ");
  DEBUG_PRINTLN(lightLevel);
}

void led(boolean onoff, int blink, int time) // LED Signal
{
  pinMode(LED_PIN, OUTPUT);      // sets the pin as output
  DEBUG_PRINT("---------- LED: ");
  if (blink == 0)
  {
    if (onoff == true)
    {
      DEBUG_PRINTLN("ON");
      digitalWrite(LED_PIN, LOW);      // turn on
    }
    else
    {
      DEBUG_PRINTLN("OFF");
      digitalWrite(LED_PIN, HIGH);       // turn off
    }
  }
  else
  {
    if (time == 0) {time = 100;}
    DEBUG_PRINT("Blink ");
    DEBUG_PRINT(blink);
    DEBUG_PRINT(" Delay ");
    DEBUG_PRINTLN(time);
    for (int count = 0; count < blink; count++)
    {
      digitalWrite(LED_PIN, LOW);      // turn on
      delay(time);
      digitalWrite(LED_PIN, HIGH);       // turn off
      delay(time);
    }
  }
}

void resend(MyMessage &msg, int repeats, bool force)
{
  int repeat = 1;
  int repeatdelay = 0;
  boolean sendOK = false;

  while ((sendOK == false) and (repeat < repeats)) {
    DEBUG_PRINT(force ? "forced " : "normal ");
    if (gw.send(msg)) {
      sendOK = true;
    } else {
      sendOK = false;
      DEBUG_PRINT("Send ERROR ");
      DEBUG_PRINTLN(repeat);
      repeatdelay += 250;
      led(true,1,5);
    } repeat++; delay(repeatdelay);
  }
}



