# 1 "/tmp/tmp7203fqqi"
#include <Arduino.h>
# 1 "/home/pietervdvn/git/LedRolschaats/LedRolschaats.ino"
#define FASTLED_ESP8266_RAW_PIN_ORDER 
#include <FastLED.h>
#include <math.h>

#define LED_PIN D1
#define NUM_LEDS 30
#define BRIGHTNESS 100
#define LED_TYPE WS2811
#define COLOR_ORDER GRB


CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100
# 36 "/home/pietervdvn/git/LedRolschaats/LedRolschaats.ino"
void setup();
void loop();
void animate(double time);
void interpolateOutrunFromTo(double start, double end);
void interpolateOutrun(int ledToApply, double interpolationLeft, double interpolationRight);
inline double soften(double v);
void clear();
void setLed(int i, int r, int g, int b);
void safety(int brightness);
#line 36 "/home/pietervdvn/git/LedRolschaats/LedRolschaats.ino"
void setup() {
 Serial.begin(115200);
 Serial.print("Booting...\n");
 Serial.print(LED_PIN);
    delay( 1000 );
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness( BRIGHTNESS );
}



void loop()
{
 double timeElapsed = 0.0;
 while(true){
  clear();

  animate(timeElapsed);

  safety(140);


  FastLED.show();
  FastLED.delay(10);
  timeElapsed += 0.10;
    }
}


void animate(double time){




 time *= 1.3;

 double ledStart = time - ((int) (time / (NUM_LEDS + 16))) * (NUM_LEDS + 16);

 interpolateOutrunFromTo(ledStart - 15 , ledStart);



}

void interpolateOutrunFromTo(double start, double end){
        for(int i = (int) (start - 1); i < end + 1; i++){
            interpolateOutrun(i, start, end);
        }

}


void interpolateOutrun(int ledToApply, double interpolationLeft, double interpolationRight){

    double factor = 1.0 * (ledToApply - interpolationLeft) / (interpolationRight - interpolationLeft);




    double redFactor = soften((-fabs(1.1*(0.30 - factor)) + 0.33) * 3.0);
    double blueFactor = soften((-fabs(1.3*(0.60 - factor)) + 0.33) * 3.0);


    int red = redFactor * (double) BRIGHTNESS;
 int blue = blueFactor *(double) BRIGHTNESS;
    setLed(ledToApply, red, 0, blue);
}

inline double soften(double v){
 if(v < 0){
  return 0;
 }
 return sqrt(v);
}


void clear(){
    for(int i = 0; i < NUM_LEDS; i++){
        leds[i] = CRGB(0,0,0);
    }
}

void setLed(int i, int r, int g, int b){
 i = NUM_LEDS - 1 - i;
 if(i < 0 || i >= NUM_LEDS){
  return;
 }
 leds[i] = CRGB(r, g, b);

}

void safety(int brightness){

 leds[0] = CRGB(brightness, 0,0);
 leds[1] = CRGB(brightness, 0,0);
 leds[2] = CRGB(brightness, 0,0);
 leds[3] = CRGB(brightness, 0,0);
 leds[4] = CRGB(brightness, 0,0);

 leds[25] = CRGB(brightness, 255,255);
 leds[26] = CRGB(brightness, 255,255);
 leds[27] = CRGB(brightness, 255,255);
    leds[28] = CRGB(brightness, 255,255);
    leds[29] = CRGB(brightness, 255,255);

}