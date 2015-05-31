#include <MySensor.h>
#include <SPI.h>
#include <readVcc.h>


#define NODE_ID 21                      // ID of node
#define CHILD_ID 1                      // Id of the sensor child

#define MAILBOX_FRONT_PIN 2             // Arduino Digital I/O pin for button/reed switch
#define MAILBOX_BACK_PIN 3              // Arduino Digital I/O pin for button/reed switch

#if (MAILBOX_FRONT_PIN < 2 || MAILBOX_FRONT_PIN > 3)
#error MAILBOX_FRONT_PIN must be either 2 or 3 for interrupts to work
#endif
#if (MAILBOX_BACK_PIN < 2 || MAILBOX_BACK_PIN > 3)
#error MAILBOX_BACK_PIN must be either 2 or 3 for interrupts to work
#endif
#if (MAILBOX_FRONT_PIN == MAILBOX_BACK_PIN)
#error MAILBOX_FRONT_PIN and BUTTON_PIN2 cannot be the same
#endif

#define MIN_V 1900 // empty voltage (0%)
#define MAX_V 3200 // full voltage (100%)


MySensor gw;
// Initialize motion message
MyMessage msg(CHILD_ID, V_TRIPPED);

boolean post = false;
boolean lastpost = false;
int oldBatteryPcnt;;
int repeat = 20;

void setup()
{
  gw.begin(NULL, NODE_ID, false);

  // Setup the buttons
  pinMode(MAILBOX_FRONT_PIN, INPUT);
  pinMode(MAILBOX_BACK_PIN, INPUT);

  // Activate internal pull-ups
  digitalWrite(MAILBOX_FRONT_PIN, HIGH);
  digitalWrite(MAILBOX_BACK_PIN, HIGH);

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Mailbox Alert", "1.0");

  gw.present(CHILD_ID, S_MOTION);
  
  Serial.println("---------- set Mailbox empty");
  resend(msg.set(0), repeat);

}

// Loop will iterate on changes on the BUTTON_PINs
void loop()
{
  uint8_t value;
  static uint8_t sentValue = 2;
  static uint8_t sentValue2 = 2;

  // Short delay to allow buttons to properly settle
  gw.sleep(5);

  if ((digitalRead(MAILBOX_FRONT_PIN)) == 0)
  {
    post = true;
    Serial.println("---------- New Mail");
  }

  if ((digitalRead(MAILBOX_BACK_PIN)) == 1)
  {
    post = false;
    Serial.println("---------- Mailbox emptied");
  }
  
  if (post != lastpost)
  {
    Serial.print("---------- Send Mailboxstate ");
    Serial.println(post ? "full" : "empty");
    resend((msg.set(post ? "1" : "0")),repeat);
    lastpost = post;
    sendBattery();
  }
  
  // Sleep until something happens with the sensor
  gw.sleep(MAILBOX_FRONT_PIN - 2, CHANGE, MAILBOX_BACK_PIN - 2, CHANGE, 0);
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

