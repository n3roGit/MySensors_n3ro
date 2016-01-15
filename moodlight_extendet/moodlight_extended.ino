#include <MySensor.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 4
#define NODE_ID 10
int LEDCount = 24;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDCount, LED_PIN, NEO_GRB + NEO_KHZ800);

int R;
int G;
int B;

int Rold;
int Gold;
int Bold;



int currentLevel = 0; // Dimlevel
long RGB_values[3] = {0, 0, 0}; // Colors
int dimmSpeed = 4;

MySensor gw;
MyMessage dimmerMsg(0, V_DIMMER);

void setup() {
  strip.begin();
  strip.setBrightness(10);
  strip.show();
  
   

  gw.begin(incomingMessage, NODE_ID, false);
  gw.sendSketchInfo("Mood Light", "3.0");
  gw.present(0, S_CUSTOM);

  // colorChange(RED, GREEN, BUE, FADE)
  colorChange(255, 255, 255, true); // white
  colorChange(255, 0, 0, true); // red
  colorChange(0, 255, 0, true); // green
  colorChange(0, 0, 255, true); // blue
  colorChange(255, 255, 0, true); 
  colorChange(255, 0, 255, true); 
  colorChange(0, 255, 255, true); 
  colorChange(255, 0, 255, true);
  colorChange(0, 0, 0, true);     // Off
  /*colorWipe(strip.Color(0,0,0), 25); // Black
  colorWipe(strip.Color(64, 0, 0), 100); // Red
  colorWipe(strip.Color(0, 64, 0), 100); // Green
  colorWipe(strip.Color(0, 0, 64), 100); // Blue
  colorWave(75);
  colorWipe(strip.Color(0,0,0), 100); // Black
  rainbow(15);
  colorWipe(strip.Color(0,0,0), 100); // Black
  rainbowCycle(15);
  colorWipe(strip.Color(0,0,0), 100); // Black
  colorWave(30);*/
}

void loop() {
  gw.process();
}

void incomingMessage(const MyMessage &message) {

  if (message.type == V_VAR1) {
    String hexstring = message.getString();

    // Check if contains hex character

      // Remove the character
      hexstring.remove(0, 1);
    
    Serial.println(hexstring);
    long number = (long) strtol( &hexstring[0], NULL, 16);
    RGB_values[0] = number >> 16;
    RGB_values[1] = number >> 8 & 0xFF;
    RGB_values[2] = number & 0xFF;
    
    R = RGB_values[0];
    G = RGB_values[1];
    B = RGB_values[2];
    colorChange(R, G, B, true);

    // Write some debug info
    Serial.print("Red is " );
    Serial.println(RGB_values[0]);
    Serial.print("Green is " );
    Serial.println(RGB_values[1]);
    Serial.print("Blue is " );
    Serial.println(RGB_values[2]);
  }

  if (message.type == V_DIMMER) {
    // Get the dimlevel
    int reqLevel = atoi( message.data );
    // Clip incoming level to valid range of 0 to 100
    reqLevel = reqLevel > 100 ? 100 : reqLevel;
    reqLevel = reqLevel < 0   ? 0   : reqLevel;

    // Set brightness
    Serial.print("Dimming to ");
    Serial.println(reqLevel);
    fadeToLevel(reqLevel);
    
  }

}

// CHANGE TO COLOR
void colorChange(uint32_t R, uint32_t G, uint32_t B, uint32_t fade) {
  if (!fade) {
    Rold = R;
    Bold = B;
    Gold = G;
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(Rold, Gold, Bold));
    }
    strip.show();
  } else {
    for (int t = 0; t < 256; t++) {
      if (R > Rold) {
        Rold++;
      }
      if (R < Rold) {
        Rold--;
      }
      if (G > Gold) {
        Gold++;
      }
      if (G < Gold) {
        Gold--;
      }
      if (B > Bold) {
        Bold++;
      }
      if (B < Bold) {
        Bold--;
      }
      for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(Rold, Gold, Bold));
      }
      strip.show();
      delay(dimmSpeed);
    }
  }
}

// DIMMER FADE
void fadeToLevel( int toLevel ) {

  int delta = ( toLevel - currentLevel ) < 0 ? -1 : 1;

  while ( currentLevel != toLevel ) {
    currentLevel += delta;
    strip.setBrightness(currentLevel);
    if(currentLevel == 1){
      for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(Rold, Gold, Bold));
      }
    }
    strip.show();
    delay(dimmSpeed);
  }
}
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

/**
 *      ^   ^   ^  
 * ~~~~~ ColorWave ~~~~~
 *        V   V   V   
 */
void colorWave(uint8_t wait) {
  int i, j, stripsize, cycle;
  float ang, rsin, gsin, bsin, offset;

  static int tick = 0;
  
  stripsize = strip.numPixels();
  cycle = stripsize * 25; // times around the circle...

  while (++tick % cycle) {
    offset = map2PI(tick);

    for (i = 0; i < stripsize; i++) {
      ang = map2PI(i) - offset;
      rsin = sin(ang);
      gsin = sin(2.0 * ang / 3.0 + map2PI(int(stripsize/6)));
      bsin = sin(4.0 * ang / 5.0 + map2PI(int(stripsize/3)));
      strip.setPixelColor(i, strip.Color(trigScale(rsin), trigScale(gsin), trigScale(bsin)));
    }

    strip.show();
    delay(wait);
  }

}
/**
 * Scale a value returned from a trig function to a byte value.
 * [-1, +1] -> [0, 254] 
 * Note that we ignore the possible value of 255, for efficiency,
 * and because nobody will be able to differentiate between the
 * brightness levels of 254 and 255.
 */
byte trigScale(float val) {
  val += 1.0; // move range to [0.0, 2.0]
  val *= 127.0; // move range to [0.0, 254.0]

  return int(val) & 255;
}

/**
 * Map an integer so that [0, striplength] -> [0, 2PI]
 */
float map2PI(int i) {
  return PI*2.0*float(i) / float(strip.numPixels());
}
