#include <MySensor.h>
#include <SPI.h>

#define SENSOR_INFO "Test sensor"
#define NODE_ID 200
#define CHILD_ID 1
#define OPEN 1
#define CLOSE 0

MySensor gw;
MyMessage msg(CHILD_ID, V_TRIPPED);

int repeat = 0;
boolean sendOK = false;
int repeatdelay=0;

void setup()
{
  gw.begin(NULL, NODE_ID, false);
  gw.sendSketchInfo(SENSOR_INFO, "1.0");
  gw.present(CHILD_ID, S_DOOR);
}

void loop()
{
  while ((sendOK == false) and (repeat < 10)) {
    if (gw.send(msg.set(OPEN))) {
      sendOK = true;
    } else {
      sendOK = false;
      Serial.print("FEHLER ");
      Serial.println(repeat);
      repeatdelay+=100;
    } repeat++;delay(repeatdelay);
  } repeat = 0; sendOK = false;

  delay(2000); // Wait 10 seconds
}
/*
void gwack(char code[])
{
  int repeat = 0;
  boolean sendOK = false;
  Serial.println(code);

  while ((sendOK == false) or (repeat <= 10))
  {
    if (1 == 1)
    {
      Serial.println("OK");
      sendOK = true;
    }
    else
    {
      Serial.println("NOT OK!!");
      sendOK = false;
    }
    repeat++;
  }
}
*/
