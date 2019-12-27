#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <EEPROM.h>

// SEARCH A SENSOR ON luftdaten.info
// CLICK IT
// LOOK FOR SENSOR ID IN THE TABLE APPEARING RIGHT
// FILL IT IN DURING INITAL WIFIMANAGER SETUP
const char* version = "2019-11-30 A persistant Wifi Portal\n";
const char* host = "data.sensor.community";
const char* sensor = "/airrohr/v1/sensor/%s/";
const int httpsPort = 443;

#define SAVEDATA_MAGIC  0xDEADBEEF

typedef struct {
    char luftdatenid[16];
    uint32_t magic;
} savedata_t;

// SHA1 fingerprint of the certificate
const char fingerprint[] PROGMEM = "DF:14:13:1B:DF:BD:6E:DA:54:57:5C:41:E7:B4:FE:7F:40:B7:F9:84";
static WiFiManager wifiManager;

static WiFiManagerParameter luftdatenIdParam("luftdatenid", "Luftdaten ID", "", sizeof(savedata_t));
static WiFiManagerParameter use_brightness_param("USE_BRIGHTNESS", "Use the brightness sensor", "", sizeof(1));

// If you have a v2.0 board, put this to true
#define USE_BRIGHTNESS true

// Time between two updates, in seconds
#define UPDATE_TIMEOUT 300
// Time in seconds between retries when failed. Should be a multiple of 4
#define RETRY_TIMEOUT 12

// ---------------------------------------- PIN DEFINTIONS FOR LEDS ------------------------------------------

#define BRIGHTNESS_PIN A0

#define RIGHTH D0
#define RIGHTL D1

#define LEFTH D3
#define LEFTL D2

#define GREEN D5
#define YELLOW D6
#define RED D7

#define LEFT 0
#define RIGHT 1

#define posPinL 4
// should be HIGH to work
int posPins[posPinL] = {LEFTH, LEFTL, RIGHTH, RIGHTL}; 
#define negPinL 3
// should be low to work
int negPins[negPinL] = {RED, YELLOW, GREEN}; 
// ---------------------- WIFI STUFF

ESP8266WiFiMulti WiFiMulti;

// -------------------------- Status tracking

#define CONNECTING 0
#define UP_TO_DATE 1

#define NOT_CONNECTED 2
#define HTTP_ERR 3
#define PARSE_ERR 4

// Color encodings
#define EXTREMELY_GOOD 0 // Double green
#define VERY_GOOD 1 // single, lowest green
#define GOOD 2 // single, highest green

#define LOW_AVG 3 // lowest yellow
#define AVG 4 // TWO_YELLOWS
#define HIGH_AVG 5 // Highest yellow

#define BAD 6 // lowest red
#define VERY_BAD 7// highest red
#define EXTREMELY_BAD 8// two red

#define OFF 9

#define USE_SERIAL Serial

/*
 * As per http://www.who.int/mediacentre/factsheets/fs313/en/
 * and per http://ec.europa.eu/environment/air/quality/standards.htm
 * WHO allows a daily mean of 25µg, and a yearly mean of 10µg (PM2.5)
 * EU allows a yearly mean of 25µg (PM2.5)
 * 
 * IN practice: seldom over 50
 * 
 * WHO allows a daily mean of 50µg and a yearly mean of 20µg (PM10)
 * EU allows a daily mean of 50µg, and a yearly mean of 40µg (PM10)
 * 
 * The color coding: over the yearly WHO norm -> yellow; over the yearly EU-norm -> red

 */

float* barema25 = new float[9] {0, /*GOOD:*/5,  7.5, 10, /*AVG*/ 15,   20,   25,/*BAD*/ 35, 45};
float* barema10 = new float[9] {0, /*GOOD:*/7.5, 15, 20, /*AVG*/ 26.6, 33.3, 40,/*BAD*/ 50, 80};

// The current brightness. This value floats slowly (~20/sec max)
int brightness = 1000;

float pm10 = 0;
float pm25 = 0;

static savedata_t savedata;

// ------------------------------------------- MAIN PROGRAM --------------------------------

static void wifiManagerCallback(void)
{
  strcpy(savedata.luftdatenid, luftdatenIdParam.getValue());
  savedata.magic = SAVEDATA_MAGIC;
  
  bool useBrightness = use_brightness_param.getValue();
  printf("Use brightness is '%d'\n", useBrightness);
  

  printf("Saving data to EEPROM: luftdatenid='%s'\n", savedata.luftdatenid);
  EEPROM.put(0, savedata);
  EEPROM.commit();
}

void setup() {
    
  USE_SERIAL.begin(115200);
  EEPROM.begin(sizeof(savedata));
  EEPROM.get(0, savedata);

  initLeds();
  startAnimation();
  setLed(true, 5,LEFT);
  setLed(true, 0,RIGHT);
  USE_SERIAL.print("\n\n");
  USE_SERIAL.print( "/--------------\\\n");
  USE_SERIAL.print ("| LUCHTLICHTJE |\n");
  USE_SERIAL.print("\\--------------/\n\n");
  
  USE_SERIAL.print("Luchtlichtje visualizes Particulate Matter measurements by luftdaten.info\nMade by pietervdvn@posteo.net\nSourcecode and hardware files can be found on github\n\nVersion: ");
  USE_SERIAL.print(version);
  if(USE_BRIGHTNESS){
    USE_SERIAL.print("Brightness sensor is enabled\n");
    }else{
        USE_SERIAL.print("Brightness sensor is disabled\n");
    }
  
  wifiManager.addParameter(&luftdatenIdParam);
  wifiManager.setSaveConfigCallback(wifiManagerCallback);
  wifiManager.setConfigPortalTimeout(120);

  if (savedata.magic == SAVEDATA_MAGIC) {
      wifiManager.autoConnect("ESP-LUCHTLICHTJE");
  } else {
      wifiManager.startConfigPortal("ESP-LUCHTLICHTJE");
  }
}


void loop() {
  USE_SERIAL.println("Updating");

  int status = NOT_CONNECTED;
  int retries = 8;
  
  while(status !=  UP_TO_DATE && retries > 0){
    USE_SERIAL.printf("Attempting update. %d retries left\n", retries);
    status = update();
    if(status != UP_TO_DATE){
      showError(status, 8-retries);
      delay(1000);
    }
    retries --;
  }

  if(status != UP_TO_DATE){
    USE_SERIAL.write("Update failed\n");
    for(int i = 0; i < RETRY_TIMEOUT; i++){
      showError(status,i);
      delay(1000);
      USE_SERIAL.printf("Retrying in %d\n", RETRY_TIMEOUT - i);
    }   
    return;
  }

  USE_SERIAL.printf("PM2.5: %d; PM10: %d\n",(int) pm25,(int) pm10);
  // We are up to date. Lets show the bars!
  // We draw one of them, show it for 5ms and hide it again. Then, we show the other bar.
  // The inner loop does this 200 times (one second)

  allLedsOff();

  for(int s = 0; s < UPDATE_TIMEOUT; s++){
      if(s % 10 == 0){
        USE_SERIAL.printf("Next update in %d sec\n", UPDATE_TIMEOUT - 1 - s);
         USE_SERIAL.printf("Current brightness is %d\n", brightness);
      }

      // The fraction of time that a single led should be powered
      // Number of milliseconds that a led should be on
      int cycletime = 7;
      for(int ms = 0; ms < 100; ms++){
          
        int msOn = (int) (cycletime * (brightness-100) / 800);
        if(cycletime < msOn){
            msOn = cycletime;
        }
        
        if(msOn > cycletime){
          showEncoding(mapState(pm25, barema25), LEFT);
          delay(cycletime);
        }
        if(msOn < 1){
          msOn = 1;
        }
        allLedsOff();
        delay(cycletime - msOn);
        showEncoding(mapState(pm25, barema25), LEFT);
        delay(msOn);
        
        
        allLedsOff();
        delay(cycletime - msOn);
        showEncoding(mapState(pm10, barema10), RIGHT);
        delay(msOn);
        updateBrightness();
      }
    }
  
  // Power both rails while updating...
  digitalWrite(LEFTH, HIGH);
  digitalWrite(LEFTL, HIGH);
  
}

// ----------------------------------------------------------- READ BRIGHTNESS -------------------------------------------


void updateBrightness(){
  int readBrightness = analogRead(BRIGHTNESS_PIN);
  if(!USE_BRIGHTNESS){
      // No brightness sensor installed or bad connection - do not update the brightness
    return;
  }
  if(brightness > readBrightness){
    brightness -= 5;
  }else if (brightness < readBrightness){
    brightness += 5;
  }
}




// ------------------------------------------------------------- UPDATE DATA -------------------------------------------------


int update(){
  char sensorpath[128];

  if(WiFi.status() != WL_CONNECTED) {
    USE_SERIAL.print("[HTTP] wifi ");
    USE_SERIAL.print(" not connected...\n");
    return NOT_CONNECTED;
  }
  USE_SERIAL.print("[HTTP] Wifi should be connected: ");
  USE_SERIAL.print(WiFi.status());
  USE_SERIAL.print("\n\n");
  
  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  client.setFingerprint(fingerprint);

  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return HTTP_ERR;
  }

  sprintf(sensorpath, sensor, savedata.luftdatenid); 
   client.print(String("GET ") + sensorpath + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  client.readStringUntil('\n'); // Read the length and discard it
  String payload = client.readString();
  
  USE_SERIAL.println("Got the payload");
  DynamicJsonBuffer jsonBuffer;
  JsonArray& root = jsonBuffer.parseArray(payload);
  if (!root[0].success()) {
    USE_SERIAL.println("parseObject() failed");
    USE_SERIAL.println(payload);
    return PARSE_ERR;
  }

  float newPm10 = (root[0]["sensordatavalues"][0]["value"].as<float>() + root[1]["sensordatavalues"][0]["value"].as<float>()) / 2;
  float newPm25 = (root[0]["sensordatavalues"][1]["value"].as<float>() + root[1]["sensordatavalues"][1]["value"].as<float>()) / 2;
  if(pm10 == 0 && pm25 == 0){
        pm10 = newPm10;
        pm25 = newPm25; 
        USE_SERIAL.printf("(Initial assignment) Read: %f, new value is %f\n", newPm10, pm10);
  }else{
      USE_SERIAL.printf("Read: %f, old value is %f, ", newPm10, pm10);
        // The new value is calculated by taking into account the previous value as well, to have less effect of outliers
        // The shown sum equals n0/2 + n1/4 + n2/8 + n3/16 ... (the lowest index is the last measurement)
        pm10 = (newPm10 + pm10)/2.0; 
        pm25 = (newPm25 + pm25)/2.0;
        USE_SERIAL.printf("New value is %f\n", pm10);
  }
    

  return UP_TO_DATE;
}

// Blinks the leds in an error-indicating sequence
// Takes 3 seconds to complete
void showError(int status, int phase){

  allLedsOff();
  int baseLed = 4;

  if(status == NOT_CONNECTED){
    baseLed = 2;
  }
  if(status == HTTP_ERR){
    baseLed = 0;
  }
  if(status == PARSE_ERR){
    baseLed = 1;
  }

  switch(phase % 4){
    case(0): setLed(true, baseLed, LEFT);break;
    case(1): setLed(true, baseLed, RIGHT);break;
    case(2): setLed(true, baseLed+1, RIGHT);break;
    case(3): setLed(true, baseLed+1, LEFT);break;
    
  }
}


// ----------------------------------------- LED CONTROL --------------------------------------------

/**
 * This function calculates the state of the PM, given the barema
 */
int mapState(float value, float* barema){
  for(int state = 1; state < OFF; state++){
    if(barema[state] > value){
       return state - 1;
    }
  }
  return EXTREMELY_BAD;
}



void showEncoding(int encoding, int series){
  switch(encoding){
    case(EXTREMELY_GOOD): setLed(true, 4, series); setLed(true, 5, series); break;
    case(VERY_GOOD): setLed(true, 5, series); break;
    case(GOOD): setLed(true, 4, series); break;
    
    case(LOW_AVG): setLed(true, 3, series); break;
    case(AVG): setLed(true, 3, series); setLed(true, 2, series); break;
    case(HIGH_AVG): setLed(true, 2, series); break;
    
    case(BAD): setLed(true, 1, series); break;
    case(VERY_BAD): setLed(true, 0, series); break;
    case(EXTREMELY_BAD): setLed(true, 0, series); setLed(true, 1, series); break;
  }
}


// ----------------------------------------- LED CONTROL --------------------------------------------

/** Number: the number of the led, with 0 being red, 2 being yellow and 4 being green
 *  Series: 0 = left; 1 = right
 */
void setLed(boolean status, int number, int series){
  series = (number % 2) == 0 ?  posPins[series*2] : posPins[series*2 + 1];
  int pin = negPins[number/2];

  digitalWrite(pin, status ? LOW : HIGH);
  digitalWrite(series, status ? HIGH : LOW); 
}

void initLeds(){
  for(int i = 0; i < negPinL; i++){
    pinMode(negPins[i], OUTPUT);
  }
  for(int i = 0; i < posPinL; i++){
    pinMode(posPins[i], OUTPUT);
  }
  allLedsOff();
}

void allLedsOff(){
  for(int i = 0; i < 6; i++){
    setLed(false, i, LEFT);
    setLed(false, i, RIGHT);
  }
}

void startAnimation(){
  for(int i = 0; i < 6; i ++){
    setLed(true, i,LEFT);
    setLed(true, i,RIGHT);
    delay(750);
    setLed(false, i,LEFT);
    setLed(false, i,RIGHT);
  }
}




