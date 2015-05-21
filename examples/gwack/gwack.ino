#include <MySensor.h>
#include <SPI.h>

#define SENSOR_INFO "Test sensor"
#define NODE_ID 200
#define CHILD_ID 1
#define OPEN 1
#define CLOSE 0

MySensor gw;
MyMessage msg(CHILD_ID, V_TRIPPED);

void setup()
{
  gw.begin(NULL, NODE_ID, false);
  gw.sendSketchInfo(SENSOR_INFO, "1.0");
  gw.present(CHILD_ID, S_DOOR);
}

void loop()
{
//gw.send(msg.set(OPEN));
  gwresend("msg.set","OPEN");
  delay(500);
}
void gwresend(char msgcode[],char option[])
{
  int repeat = 0;
  boolean sendOK = false;

  while ((sendOK == false) or (repeat <= 10))
  {
    if (gw.send(msgcode(option)) // execute code
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
