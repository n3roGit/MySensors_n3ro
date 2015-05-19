#include <MySensor.h>
#include <SPI.h>
#include <DHT.h>
#include <Wire.h>

#define NODE_ID 10                       // ID of node
#define READ_TIME 300000                 // Sleep time between reports (in milliseconds)

#define CHILD_ID_PIR 1                   // ID of the sensor PIR
#define CHILD_ID_HUM 2                   // ID of the sensor HUM
#define CHILD_ID_TEMP 3                  // ID of the sensor TEMP
#define CHILD_ID_LIGHT 4                 // ID of the sensor LIGHT
#define CHILD_ID_SWITCH 5                // ID of the sensor SWITCH
#define CHILD_ID_MQ 6


#define PIR_SENSOR_DIGITAL 3             // PIR pin
#define HUMIDITY_SENSOR_DIGITAL_PIN 4    // DHT pin
#define LIGHT_SENSOR_ANALOG_PIN 0        // LDR pin
#define BEEP_SENSOR_ANALOG_PIN 2         // BEEPER pin

// MQ Settings
#define MQ_SENSOR_ANALOG_PIN 1           //define which analog input channel you are going to use
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


MySensor gw;
// Initialize Variables
MyMessage msgPir(CHILD_ID_PIR, V_TRIPPED);
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgLight(CHILD_ID_LIGHT, V_LIGHT_LEVEL);
MyMessage msgMQ(CHILD_ID_MQ, V_VAR1);
MyMessage msgSwitch(CHILD_ID_SWITCH,V_LIGHT);


DHT dht;
float lastTemp;
float lastHum;
boolean metric = true;
int lastLightLevel;
int sentValue;
unsigned long uptime;
boolean lastBeep = false;
boolean beepState = true;

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
  gw.begin(setSwitchState, NODE_ID, true);

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

  pinMode(BEEP_SENSOR_ANALOG_PIN, OUTPUT);
  
  gw.send(msgSwitch.set(true),true);
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
  sendPir();
  getSwitchState();
  

  
 
}



void sendPir() // Get value of PIR
{
  int value = digitalRead(PIR_SENSOR_DIGITAL); // Get value of PIR
  if (value != sentValue) { // If status of PIR has changed
    gw.send(msgPir.set(value == HIGH ? 1 : 0)); // Send PIR status to gateway
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
      gw.send(msgTemp.set(temperature, 1));
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
      gw.send(msgHum.set(humidity, 1));
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
    gw.send(msgLight.set(lightLevel));
    lastLightLevel = lightLevel;
  }
  Serial.print("---------- Light: ");
  Serial.println(lightLevel);
}

void setSwitchState(const MyMessage &message) //Turn Alarm on/off
{

  // We only expect one type of message from controller. But we better check anyway.
  if (message.type == V_LIGHT)
  {
    beepState = message.getBool();
    if (message.getBool() == false)
    {
      beep(false);
    }
    else
    {
      Serial.println("---------- Beeper: enabled");
      //gw.send(msgSwitch.set(message.getBool()?false:true), true);
      //gw.send(msgSwitch.set(true));
    }
  }
}
void getSwitchState()
{
 gw.read(msgSwitch.get(message.getBool()));
}

void sendMQ() // Get AirQuality Level
{
  //uint16_t valMQ = MQGetGasPercentage(MQRead(MQ_SENSOR_ANALOG_PIN) / Ro, GAS_CO);
  //Serial.println(val);

  Serial.print("---------- LPG: ");
  Serial.print(MQGetGasPercentage(MQRead(MQ_SENSOR_ANALOG_PIN) / Ro, GAS_LPG) );
  Serial.println( "ppm" );
  Serial.print("---------- CO: ");
  Serial.print(MQGetGasPercentage(MQRead(MQ_SENSOR_ANALOG_PIN) / Ro, GAS_CO) );
  Serial.println( "ppm" );
  Serial.print("---------- SMOKE: ");
  Serial.print(MQGetGasPercentage(MQRead(MQ_SENSOR_ANALOG_PIN) / Ro, GAS_SMOKE) );
  Serial.println( "ppm" );


  if (valMQ != lastMQ)
  {
    gw.send(msgMQ.set((int)ceil(valMQ)));
    lastMQ = ceil(valMQ);
  }
  if (MQGetGasPercentage(MQRead(MQ_SENSOR_ANALOG_PIN) / Ro, GAS_SMOKE) >= 100)
  {
    beep(true);
  }
  else
  {
    beep(false);
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

void beep(boolean onoff) // Make BEEEEEEP
{
  Serial.print("---------- Beeper: ");
  if (beepState == true)
  {
    if (onoff != lastBeep)
    {
      if (onoff == true)
      {
        Serial.println("ON");
        analogWrite(BEEP_SENSOR_ANALOG_PIN, 20);      // Almost any value can be used except 0 and 255
        lastBeep = true;
      }
      else
      {
        Serial.println("OFF");
        analogWrite(BEEP_SENSOR_ANALOG_PIN, 0);       // 0 turns it off
        lastBeep = false;
      }
    }
    else
    {

      Serial.println(onoff ? "still ON" : "still OFF");
    }
  }
  else
  {
    Serial.println("disabled");
    analogWrite(BEEP_SENSOR_ANALOG_PIN, 0);       // 0 turns it off
    lastBeep = false;
  }
}


