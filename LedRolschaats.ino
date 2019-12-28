#include <FastLED.h>
#include <math.h>

#define LED_PIN     7
#define NUM_LEDS    90
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
    int time = 0;
    while(true){
        clear();

        animate(time);

        FastLED.show();
        FastLED.delay(10);
        time += 1;
    }
}

void animate(int time) {
    int led = time % NUM_LEDS;
    for (int i = 0; i < NUM_LEDS; i++) {
        int a = activation(i, led);
        setLed(i, a, a/2, 0);
    }
}

int activation(int current, int main) {
    int distance;
    if (current > main) {
        distance = NUM_LEDS + main - current;
    } else {
        distance = main - current;
    }
    int diff = NUM_LEDS - distance;
    return ((int) (((double) BRIGHTNESS) * sqrt((double) diff)));
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

