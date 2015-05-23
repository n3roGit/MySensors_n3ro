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
int repeatdelay = 0;

void setup()
{
  gw.begin(NULL, NODE_ID, false);
  gw.sendSketchInfo(SENSOR_INFO, "1.0");
  gw.present(CHILD_ID, S_DOOR);
}

void loop()
{
  resend(msg.set(OPEN), 5);

}
void resend(MyMessage &msg, int repeats)
{
  int repeat = 1;
  int repeatdelay = 1;
  boolean sendOK = false;

  while ((sendOK == false) and (repeat < repeats)) {
    if (gw.send(msg)) {
      sendOK = true;
    } else {
      sendOK = false;
      Serial.print("FEHLER ");
      Serial.println(repeat);
      repeatdelay += 100;
    } repeat++; delay(repeatdelay);
  }
}


