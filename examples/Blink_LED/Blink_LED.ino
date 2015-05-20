#define LED_PIN A1                 // LED connected PIN                  

void setup()
{
 
}

void loop()
{
  led(true, 0, 0);
  delay(4000);
  led(false, 0, 0);

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
    Serial.print("Blink "+blink);
    Serial.println(" Delay  "+time);
    for (int count = 0; count < blink; count++)
    {
      digitalWrite(LED_PIN, LOW);      // turn on
      delay(time);
      digitalWrite(LED_PIN, HIGH);       // turn off
      delay(time);
    }
  }
}




