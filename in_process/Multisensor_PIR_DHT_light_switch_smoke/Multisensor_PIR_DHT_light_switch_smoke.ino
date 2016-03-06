#include <MySensor.h>
#include <SPI.h>
#include <DHT.h>
#include <Wire.h>

#define NODE_ID 20                       // ID of node
#define READ_TIME 6000                 // Sleep time between reports (in milliseconds)

#define CHILD_ID_PIR 1                   // ID of the sensor PIR
#define CHILD_ID_HUM 2                   // ID of the sensor HUM
#define CHILD_ID_TEMP 3                  // ID of the sensor TEMP
#define CHILD_ID_LIGHT 4                 // ID of the sensor LIGHT
#define CHILD_ID_SWITCH 5                // ID of the sensor SWITCH
#define CHILD_ID_MQ 6                    // ID of the sensor MQ


#define PIR_SENSOR_DIGITAL 3             // PIR pin
#define HUMIDITY_SENSOR_DIGITAL_PIN 4    // DHT pin
#define LIGHT_SENSOR_ANALOG_PIN A0       // LDR pin
#define BEEP_PIN 7                // BEEPER pin
#define LED_PIN A1                       // LED connected pin

// MQ Settings
#define MQ_SENSOR_ANALOG_PIN A2           //define which analog input channel you are going to use
#define RL_VALUE 5                       //define the load resistance on the board, in kilo ohms
#define RO_CLEAN_AIR_FACTOR 9.83         //RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,
//which is derived from the chart in datasheet
/***********************Software Related Macros************************************/
#define         CALIBARAION_SAMPLE_TIMES     (50)    //define how many samples you are going to take in the calibration phase
#define         CALIBRATION_SAMPLE_INTERVAL  (500)   //define the time interal(in milisecond) between each samples in the
//cablibration phase
#define         READ_SAMPLE_INTERVAL         (50)    //define how many samples you are going to take in normal operation
#define         READ_SAMPLE_TIMES            (5)     //define the time interal(in milisecond) between each samples in 
//normal operation
/**********************Application Related Macros**********************************/
#define         GAS_LPG                      (0)
#define         GAS_CO                       (1)
#define         GAS_SMOKE                    (2)
#define ENABLE_ALARM_TIME  7200000              // Reenable Alarm after 2h


MySensor gw;
// Initialize Variables
MyMessage msgPir(CHILD_ID_PIR, V_TRIPPED);
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgLight(CHILD_ID_LIGHT, V_LIGHT_LEVEL);
MyMessage msgMQ(CHILD_ID_MQ, V_VAR1);
MyMessage msgSwitch(CHILD_ID_SWITCH, V_LIGHT);


DHT dht;
float lastTemp;
float lastHum;
boolean metric = true;
int lastLightLevel;
int sentValue;
unsigned long uptime;
unsigned long alarmtimer;
boolean beepState = true;
int repeat = 10;

float Ro = 10000.0;    // this has to be tuned 10K Ohm
int val = 0;           // variable to store the value coming from the sensor
float valMQ = 0.0;
float lastMQ = 0.0;
float           LPGCurve[3]  =  {2.3, 0.21, -0.47}; //two points are taken from the curve.
//with these two points, a line is formed which is "approximately equivalent"
//to the original curve.
//data format:{ x, y, slope}; point1: (lg200, 0.21), point2: (lg10000, -0.59)
float           COCurve[3]  =  {2.3, 0.72, -0.34};  //two points are taken from the curve.
//with these two points, a line is formed which is "approximately equivalent"
//to the original curve.
//data format:{ x, y, slope}; point1: (lg200, 0.72), point2: (lg10000,  0.15)
float           SmokeCurve[3] = {2.3, 0.53, -0.44}; //two points are taken from the curve.
//with these two points, a line is formed which is "approximately equivalent"
//to the original curve.
//data format:{ x, y, slope}; point1: (lg200, 0.53), point2:(lg10000,-0.22)


void setup()
{
  gw.begin(incomingMessage, NODE_ID, true);

  gw.sendSketchInfo("PIR,DHT,Light,MQ,Alarm", "1.0");
  // Register all sensors to gateway (they will be created as child devices)
  gw.present(CHILD_ID_PIR, S_MOTION);
  gw.present(CHILD_ID_HUM, S_HUM);
  gw.present(CHILD_ID_TEMP, S_TEMP);
  gw.present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
  gw.present(CHILD_ID_SWITCH, S_LIGHT);
  gw.present(CHILD_ID_MQ, S_AIR_QUALITY);

  pinMode(PIR_SENSOR_DIGITAL, INPUT);
  digitalWrite(PIR_SENSOR_DIGITAL, HIGH);

  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN);

  Ro = MQCalibration(MQ_SENSOR_ANALOG_PIN);         //Calibrating the sensor. Please make sure the sensor is in clean air
  //when you perform the calibration


  //gw.send(msgSwitch.set(true), true); // Set Beeper enabled in GW
  resend(msgSwitch.set(true), repeat);

  led(true, 3, 100);
}

void loop()
{
  gw.process();
  if ( millis() - uptime >= READ_TIME )  //use UNSIGNED SUBRTACTION im your millis() timers to avoid rollover issues later on down the line
  {
    Serial.println("Reading Sensors...");
    sendLight();
    sendTemp();
    sendHum();
    sendMQ();
    Serial.println("Finished with reading Sensors...");
    Serial.println("");
    uptime = millis();
  }
  if ( millis() - alarmtimer >= ENABLE_ALARM_TIME and beepState == false )  //use UNSIGNED SUBRTACTION im your millis() timers to avoid rollover issues later on down the line
  {
    Serial.println("---------- BEEPER: enabled after Timeout");
    resend(msgSwitch.set(true), repeat);
    beepState = true;
    beep(true, 3, 100);
  }
  sendPir();

}



void sendPir() // Get value of PIR
{
  int value = digitalRead(PIR_SENSOR_DIGITAL); // Get value of PIR
  if (value != sentValue) { // If status of PIR has changed
    //gw.send(msgPir.set(value == HIGH ? 1 : 0)); // Send PIR status to gateway
    resend((msgPir.set(value == HIGH ? 1 : 0)), repeat);
    sentValue = value;
    Serial.print("---------- PIR: ");
    Serial.println(value ? "tripped" : "not tripped");
  }

}



void sendTemp() // Get temperature
{
  //delay(dht.getMinimumSamplingPeriod());  // Use the delay if DHT cant read
  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
    Serial.println("Failed reading temperature from DHT");
  } else {
    if (temperature != lastTemp) {
      //gw.send(msgTemp.set(temperature, 1));
      resend((msgTemp.set(temperature, 1)), repeat);
      lastTemp = temperature;
    }
    Serial.print("---------- Temp: ");
    Serial.println(temperature);
  }
}


void sendHum() // Get humidity
{
  //delay(dht.getMinimumSamplingPeriod()); // Use the delay if DHT cant read
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed reading humidity from DHT");
  } else {
    if (humidity != lastHum) {
      //gw.send(msgHum.set(humidity, 1));
      resend((msgHum.set(humidity, 1)), repeat);
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
    resend((msgLight.set(lightLevel)), repeat);
    lastLightLevel = lightLevel;
  }
  Serial.print("---------- Light: ");
  Serial.println(lightLevel);
}

void incomingMessage(const MyMessage &message) //Turn Alarm on/off
{
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type == V_LIGHT)
  {
    beepState = message.getBool();
    if (beepState == false)
    {
      Serial.println("---------- BEEPER: disabled");
      beep(false, 0, 0);
      beep(true, 2, 100);
      alarmtimer = millis();
    }
    else
    {
      Serial.println("---------- BEEPER: enabled");
      beep(true, 2, 200);
    }
  }
}

void sendMQ() // Get AirQuality Level
{
  uint16_t valMQ = MQGetGasPercentage(MQRead(MQ_SENSOR_ANALOG_PIN) / Ro, GAS_CO);
  Serial.println(val);

  Serial.print("---------- LPG: ");
  Serial.print(MQGetGasPercentage(MQRead(MQ_SENSOR_ANALOG_PIN) / Ro, GAS_LPG) );
  Serial.println( "ppm" );
  Serial.print("---------- CO: ");
  Serial.print(MQGetGasPercentage(MQRead(MQ_SENSOR_ANALOG_PIN) / Ro, GAS_CO) );
  Serial.println( "ppm" );
  Serial.print("---------- SMOKE: ");
  Serial.print(MQGetGasPercentage(MQRead(MQ_SENSOR_ANALOG_PIN) / Ro, GAS_SMOKE) );
  Serial.println( "ppm" );


  //if (valMQ != lastMQ)
  //{
    //gw.send(msgMQ.set((int)ceil(valMQ)));
    resend((msgMQ.set((int)ceil(valMQ))), repeat);
     resend((msgMQ.set((int)ceil((MQGetGasPercentage(MQRead(MQ_SENSOR_ANALOG_PIN) / Ro, GAS_SMOKE)))), repeat);
    lastMQ = ceil(valMQ);
  //}
  if (MQGetGasPercentage(MQRead(MQ_SENSOR_ANALOG_PIN) / Ro, GAS_SMOKE) >= 100 and beepState == true)
  {
    Serial.println("---------- Starting Alarm");
    beep(true, 0, 0);
  }
  else
  {
    Serial.println("---------- Stopping Alarm");
    beep(false, 0, 0);
  }
}
float MQResistanceCalculation(int raw_adc)
{
  return ( ((float)RL_VALUE * (1023 - raw_adc) / raw_adc));
}
float MQCalibration(int mq_pin)
{
  int i;
  float val = 0;
  for (i = 0; i < CALIBARAION_SAMPLE_TIMES; i++) {      //take multiple samples
    val += MQResistanceCalculation(analogRead(mq_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val = val / CALIBARAION_SAMPLE_TIMES;                 //calculate the average value

  val = val / RO_CLEAN_AIR_FACTOR;                      //divided by RO_CLEAN_AIR_FACTOR yields the Ro
  //according to the chart in the datasheet
  return val;
}
float MQRead(int mq_pin)
{
  int i;
  float rs = 0;

  for (i = 0; i < READ_SAMPLE_TIMES; i++) {
    rs += MQResistanceCalculation(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
  }
  rs = rs / READ_SAMPLE_TIMES;
  return rs;
}

int MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
  if ( gas_id == GAS_LPG ) {
    return MQGetPercentage(rs_ro_ratio, LPGCurve);
  } else if ( gas_id == GAS_CO ) {
    return MQGetPercentage(rs_ro_ratio, COCurve);
  } else if ( gas_id == GAS_SMOKE ) {
    return MQGetPercentage(rs_ro_ratio, SmokeCurve);
  }

  return 0;
}
int  MQGetPercentage(float rs_ro_ratio, float *pcurve)
{
  return (pow(10, ( ((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0])));
}

void beep(boolean onoff, int repeat, int time) // Beep Signal
{
  pinMode(BEEP_PIN, OUTPUT);      // sets the pin as output
  Serial.print("---------- BEEPER: ");
  if (repeat == 0)
  {
    if (onoff == true)
    {
      Serial.println("ON");
      digitalWrite(BEEP_PIN, HIGH);      // turn on
    }
    else
    {
      Serial.println("OFF");
      digitalWrite(BEEP_PIN, LOW);       // turn off
    }
  }
  else
  {
    if (time == 0) {
      time = 100;
    }
    Serial.print("Beep ");
    Serial.print(repeat);
    Serial.print(" Delay ");
    Serial.println(time);
    for (int count = 0; count < repeat; count++)
    {
      digitalWrite(BEEP_PIN, HIGH);      // turn on
      delay(time);
      digitalWrite(BEEP_PIN, LOW);       // turn off
      delay(time);
    }
  }
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
    if (time == 0) {
      time = 100;
    }
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
      led(true, 1, 5);
    } repeat++; delay(repeatdelay);
  }
}

