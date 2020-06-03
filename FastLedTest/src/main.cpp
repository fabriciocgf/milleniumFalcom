#include <FastLED.h>
#include <Arduino.h>
#include "ESP8266WiFi.h"

// How many leds in your strip?
#define NUM_LEDS 16
void falcom();

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
#define DATA_PIN D3

#define FIRST_COLOR 0x14a9fa
#define SECOND_COLOR 0x9706cc 

// Define the array of leds
CRGB leds[NUM_LEDS];

void setup() { 
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed
    FastLED.setBrightness(14);
    delay(50);
    pinMode(D2,OUTPUT);
    pinMode(D1,OUTPUT);
    digitalWrite(D2,HIGH);
    digitalWrite(D1   ,HIGH);

    
}

void loop() {
  // // Turn the LED on, then pause
  // for (size_t i = 0; i < NUM_LEDS/2; i++)
  // {
  //   leds[i] = CRGB(FIRST_COLOR); //cor fabi 123457
  //   leds[15-i] = CRGB(FIRST_COLOR);
  //   FastLED.show();
  //   delay(50);
  // }
  
  // // Now turn the LED off, then pause
  // for (size_t i = 0; i < NUM_LEDS/2; i++)
  // {
  //   leds[i] = CRGB(SECOND_COLOR);
  //   leds[15-i] = CRGB(SECOND_COLOR);
  //   FastLED.show();
  //   delay(50);
  // }  

  // // Turn the LED on, then pause
  // for (size_t i = 0; i < NUM_LEDS/2; i++)
  // {
  //   leds[7-i] = CRGB(FIRST_COLOR); //cor fabi 123457
  //   leds[i+8] = CRGB(FIRST_COLOR);
  //   FastLED.show();
  //   delay(50);
  // }

  // // Now turn the LED off, then pause
  // for (size_t i = 0; i < NUM_LEDS/2; i++)
  // {
  //   leds[7-i] = CRGB(SECOND_COLOR);
  //   leds[i+8] = CRGB(SECOND_COLOR);
  //   FastLED.show();
  //   delay(50);
  // }

  falcom();
  FastLED.show();   

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