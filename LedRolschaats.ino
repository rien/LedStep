// define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
#include <math.h> 

#define LED_PIN     7
#define NUM_LEDS    30
#define BRIGHTNESS  100
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB


CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

// This example shows several ways to set up and use 'palettes' of colors
// with FastLED.
//
// These compact palettes provide an easy way to re-colorize your
// animation on the fly, quickly, easily, and with low overhead.
//
// USING palettes is MUCH simpler in practice than in theory, so first just
// run this sketch, and watch the pretty lights as you then read through
// the code.  Although this sketch has eight (or more) different color schemes,
// the entire sketch compiles down to about 6.5K on AVR.
//
// FastLED provides a few pre-configured color palettes, and makes it
// extremely easy to make up your own color schemes with palettes.
//
// Some notes on the more abstract 'theory and practice' of
// FastLED compact palettes are at the bottom of this file.




void setup() {
    Serial.begin(115200);
    Serial.print("Booting...\n");
    Serial.print(LED_PIN);
    delay( 1000 ); // power-up safety delay
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(  BRIGHTNESS );
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

    // Move one pixel per second.
    // AT time 0, we are at - 12

    time *= 1.3;

    double ledStart = time - ((int) (time / (NUM_LEDS + 16))) * (NUM_LEDS + 16);

    interpolateOutrunFromTo(ledStart - 15 , ledStart);



}

void interpolateOutrunFromTo(double start, double end){
    for(int i = (int) (start - 1); i < end + 1; i++){
        interpolateOutrun(i, start, end);
    }

}

// Interpolates black -> red -> blue -> black
void interpolateOutrun(int ledToApply, double interpolationLeft, double interpolationRight){

    double factor = 1.0 * (ledToApply - interpolationLeft) / (interpolationRight - interpolationLeft);

    // red peaks at 0.33
    // blue peaks at 0.66

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
    i = NUM_LEDS - 1 - i; // The led is placed in reverse
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
