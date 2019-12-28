#include <FastLED.h>
#include <math.h>

#define LED_PIN     7
#define NUM_LEDS    90
#define BRIGHTNESS  100
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define WAVE_LENGTH 12


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
    double waves[NUM_LEDS];
    while(true){
        clear();

        animate(waves);

        FastLED.show();
        FastLED.delay(50);
        time += 1;
    }
}

void animate(double* waves) {
    if (random(4) == 0) {
        for (int i = 0; i < WAVE_LENGTH; i++) {
            waves[i] = 1.0;
        }
    }
    for (int i = NUM_LEDS; i >= 0; i--) {
        setLed(i, 1., 1.0 - (((double) i) / (double) NUM_LEDS), 0.0);
        if (i + WAVE_LENGTH < NUM_LEDS){
            waves[i + WAVE_LENGTH] = waves[i] * ((double) random(10)) / 10 ;
        } else {
            waves[i] = 0;
        }
    }
}

void clear(){
    for(int i = 0; i < NUM_LEDS; i++){
        leds[i] = CRGB(0.,0.,0.);
    }
}

void setLed(int i, double r, double g, double b){
    double brightness = (double) BRIGHTNESS;
    int ir = (int) r*brightness;
    int ig = (int) g*brightness;
    int ib = (int) b*brightness;
    i = NUM_LEDS - 1 - i; // The led is placed in reverse
    if(i < 0 || i >= NUM_LEDS){
        return;
    }
    leds[i] = CRGB(ir, ig, ib);

}

