#include <Arduino.h>
#include "ESP8266WiFi.h"
#include "fauxmoESP.h"
#include <FastLED.h>

void falcom();
void Fire2012();
#define FRAMES_PER_SECOND 28
bool gReverseDirection = true;


// How many leds in your strip?
#define NUM_LEDS 16

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
#define DATA_PIN D3
#define BRIGHTNESS 50

// Define the array of leds
CRGB leds[NUM_LEDS];

#define SERIAL_BAUDRATE 74880

#define WIFI_SSID "Casa 2G"
#define WIFI_PASS "012996009191"

#define DEV_1 "Falcom"
#define DEV_2 "Lareira"


fauxmoESP fauxmo;

int flag = 0;
int brightness = 254;

// Wi-Fi Connection
void wifiSetup() {
  // Set WIFI module to STA mode
  WiFi.mode(WIFI_STA);

  // Connect
  Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Wait
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  // Connected!
  Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

void setup() {

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed
  FastLED.setBrightness(  BRIGHTNESS );

  // Init serial port and clean garbage
  Serial.begin(SERIAL_BAUDRATE);
  Serial.println();

  // Wi-Fi connection
  wifiSetup();

  pinMode(D2,OUTPUT);
  pinMode(D1,OUTPUT);
  digitalWrite(D2,HIGH);
  digitalWrite(D1   ,HIGH);

  // By default, fauxmoESP creates it's own webserver on the defined port
  // The TCP port must be 80 for gen3 devices (default is 1901)
  // This has to be done before the call to enable()
  fauxmo.createServer(true); // not needed, this is the default value
  fauxmo.setPort(80); // This is required for gen3 devices

  // You have to call enable(true) once you have a WiFi connection
  // You can enable or disable the library at any moment
  // Disabling it will prevent the devices from being discovered and switched
  fauxmo.enable(true);
  // You can use different ways to invoke alexa to modify the devices state:
  // "Alexa, turn lamp two on"

  // Add virtual devices
  fauxmo.addDevice(DEV_1);
  fauxmo.addDevice(DEV_2);
  
  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
    // Callback when a command from Alexa is received. 
    // You can use device_id or device_name to choose the element to perform an action onto (relay, LED,...)
    // State is a boolean (ON/OFF) and value a number from 0 to 255 (if you say "set kitchen light to 50%" you will receive a 128 here).
    // Just remember not to delay too much here, this is a callback, exit as soon as possible.
    // If you have to do something more involved here set a flag and process it in your main loop.
        
    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
    if ( (strcmp(device_name, DEV_1) == 0) ) {
      // this just sets a variable that the main loop() does something about
      if (state) {
        flag = 1;
        brightness = value;
      } else {
        flag = 0;
      }
    }
    if ( (strcmp(device_name, DEV_2) == 0) ) {
      // this just sets a variable that the main loop() does something about
      if (state) {
        flag = 2;
        brightness = value;
      } else {
        flag = 0;
      }
    }
  });

}

void loop() {
  // fauxmoESP uses an async TCP server but a sync UDP server
  // Therefore, we have to manually poll for UDP packets
  fauxmo.handle();

  if (flag == 1)
  {
    falcom();
    FastLED.show();
    digitalWrite(D2,HIGH);
    digitalWrite(D1   ,HIGH);
    
  } else if (flag == 2)
  { 
    Fire2012();
    FastLED.show();
    digitalWrite(D2,HIGH);
    digitalWrite(D1   ,HIGH);
    FastLED.delay(1000 / FRAMES_PER_SECOND);

  } else if (flag == 0)
  {
    for (size_t i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CRGB::Black;
      FastLED.show();
      delay(100);
    }
    digitalWrite(D2,LOW);
    digitalWrite(D1   ,LOW);
  }
  
  
  // static unsigned long last = millis();
  // if (millis() - last > 5000) {
  //   last = millis();
  //   Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
  // }
}

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//// 
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation, 
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  50

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 50



void Fire2012()
{
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      CRGB color = HeatColor( heat[j]);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;
    }
}

 void falcom() 
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
 
  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 254);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = beatsin88(11300, 33000, 40960);
  
  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  uint16_t brightnesstheta16 = sPseudotime;
  
  for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    CRGB newcolor = CHSV( hue8, sat8, bri8);
    
    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS-1) - pixelnumber;
    
    nblend( leds[pixelnumber], newcolor, 64);
  }
}