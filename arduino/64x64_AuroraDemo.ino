#define VERSION "0.0.5"
#define MQTTDEVICEID "ESP32_AURORA1"
#define OTA_HOSTNAME "ESP32_AURORA1"


#define DEBUG 1
#define DEBUG_MQTT 1

#include "debug_print.h"
#include "wifi_mqtt_creds.h"

/* ------------------------- CUSTOM GPIO PIN MAPPING ------------------------- */
#define R1_PIN  25
#define G1_PIN  26
#define B1_PIN  27
#define R2_PIN  14
#define G2_PIN  12
#define B2_PIN  13
#define A_PIN   23
#define B_PIN   19 
#define C_PIN    5
#define D_PIN   17
#define E_PIN   32
#define LAT_PIN  4
#define OE_PIN  15
#define CLK_PIN 16

/* -------------------------- Display Config Initialisation -------------------- */

// MATRIX_WIDTH and MATRIX_HEIGHT *must* be changed in ESP32-HUB75-MatrixPanel-I2S-DMA.h
// If you are using Platform IO (you should), pass MATRIX_WIDTH and MATRIX_HEIGHT as a compile time option.
// Refer to: https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/issues/48#issuecomment-749402379


/* -------------------------- Class Initialisation -------------------------- */

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h> // provides WiFiClientSecure

// Callback function header
void mqttCallback(const char* topic, const byte* payload, unsigned int length);


MatrixPanel_I2S_DMA matrix;

WiFiClientSecure net;
PubSubClient mqttClient(net);

bool isWifiAvailable = false;
bool isMqttAvailable = false;

String ipAddr;
String dnsAddr;

const unsigned maxWifiWaitSeconds = 60;
const int maxMqttRetry = 5;

const char* mqttPattern       = MQTTDEVICEID "/pattern";
const char* mqttPatternSet    = MQTTDEVICEID "/pattern/set";
const char* mqttPalette       = MQTTDEVICEID "/palette";
const char* mqttPaletteSet    = MQTTDEVICEID "/palette/set";
//const char* mqttBrightness    = MQTTDEVICEID "/brightness";
const char* mqttBrightnessSet = MQTTDEVICEID "/brightness/set";
const char* mqttDurationSet   = MQTTDEVICEID "/duration/set";
const char* mqttModeSet       = MQTTDEVICEID "/mode/set";
const char* mqttMode          = MQTTDEVICEID "/mode";
const char* mqttState         = MQTTDEVICEID "/state";


/* 
 * Below is an is the 'legacy' way of initialising the MatrixPanel_I2S_DMA class.
 * i.e. MATRIX_WIDTH and MATRIX_HEIGHT are modified by compile-time directives.
 * By default the library assumes a single 64x32 pixel panel is connected.
 *
 * Refer to the example '2_PatternPlasma' on the new / correct way to setup this library
 * for different resolutions / panel chain lengths within the sketch 'setup()'.
 * 
 */

#include <FastLED.h>

#include "Effects.h"
Effects effects;

#include "Drawable.h"
#include "Playlist.h"
//#include "Geometry.h"
#include "Patterns.h"

Patterns patterns;

/* -------------------------- Some variables -------------------------- */
unsigned long fps = 0, fps_timer; // fps (this is NOT a matix refresh rate!)
unsigned int default_fps = 30, pattern_fps = 30;  // default fps limit (this is not a matix refresh conuter!)
unsigned long ms_animation_max_duration = 60000;  // 60 seconds
unsigned long last_frame = 0, ms_previous = 0;
boolean auto_change = true;

void setup()
{
  // Setup serial interface
  Serial.begin(115200);
  delay(250);
  matrix.begin(R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN );  // setup the LED matrix
  /**
   * this demos runs pretty fine in fast-mode which gives much better fps on large matrixes (>128x64)
   * see comments in the lib header on what does that means
   */
  //matrix.setFastMode(true);

  // SETS THE BRIGHTNESS HERE. MAX value is MATRIX_WIDTH, 2/3 OR LOWER IDEAL, default is about 50%
  // matrix.setPanelBrightness(30);
  /* another way to change brightness is to use
   * matrix.setPanelBrightness8(uint8_t brt);	// were brt is within range 0-255
   * it will recalculate to consider matrix width automatically
   */
  matrix.setBrightness8(180);


  isWifiAvailable = setupWifi() ? false : true;
  net.setCACert(server_crt_str);
  net.setCertificate(client_crt_str);
  net.setPrivateKey(client_key_str);

  if (isWifiAvailable) {
    mqttClient.setServer(mqtt_host, mqtt_port);
    mqttClient.setCallback(mqttCallback);
    isMqttAvailable = mqttConnect() ? false : true;
  }

  // ***************************************************************************
  // over-the-air update
  if (isWifiAvailable) {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.onStart([]() {
			 String type;
			 if (ArduinoOTA.getCommand() == U_FLASH) {
			   type = "sketch";
			 } else { // U_FS
			   type = "filesystem";
			 }
			 // NOTE: if updating FS this would be the place to unmount FS using FS.end()
			 DEBUG_PRINTLN("Start updating " + type);
			 patterns.stop();
			 matrix.fillScreen(matrix.color444(0, 0, 0));
			 matrix.setTextSize(2);    // size 1 == 8 pixels high
			 matrix.setCursor(16, 0);  // start at top left, with 8 pixel of spacing
			 matrix.println("OTA");
			 matrix.println("FLASH");
			 
		       });
    ArduinoOTA.onEnd([]() {
		       DEBUG_PRINTLN("\nEnd");
		     });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
			    unsigned int percent = progress / (total / 100);
			    //unsigned long pixel = ((unsigned long)(percent * 1.92)) - 1;
			    // fill from bottom row to top ... we have 24 rows
			    unsigned row = ((unsigned long)(percent * 64/100));
			    //fillRow(row);
			    matrix.drawLine(0,                  matrix.height() - 1 - row,
					    matrix.width() - 1, matrix.height() - 1 - row,
					    matrix.color444(1, 0, 14));
			    
			    // fixme overlay percentage value ...
			  });
    ArduinoOTA.onError([](ota_error_t error) {
			 //Serial.printf("Error[%u]: ", error);
			 // tft.fillScreen(TFT_BLACK);
			 // tft.setTextColor(TFT_WHITE);
			 // tft.setTextFont(2);
			 // tft.println("***  OTA ERROR !!!");
			 delay(1000);

			 if (error == OTA_AUTH_ERROR) {
			   DEBUG_PRINTLN("Auth Failed");
			   //tft.println("***  AUTH !!!");
			 } else if (error == OTA_BEGIN_ERROR) {
			   DEBUG_PRINTLN("Begin Failed");
			   //tft.println("***  BEGIN failed !!!");
			 } else if (error == OTA_CONNECT_ERROR) {
			   DEBUG_PRINTLN("Connect Failed");
			   //tft.println("***  connect failed !!!");
			 } else if (error == OTA_RECEIVE_ERROR) {
			   DEBUG_PRINTLN("Receive Failed");
			   //tft.println("***  receive failed !!!");
			 } else if (error == OTA_END_ERROR) {
			   DEBUG_PRINTLN("End Failed");
			   //tft.println("***  END failed !!!");
			 }
			 delay(5000);
		       });
    ArduinoOTA.begin();
  }

  // ***************************************************************************

  Serial.println("**************** Starting Aurora Effects Demo ****************");

   // setup the effects generator
  effects.Setup();

  delay(500);
  Serial.println("Effects being loaded: ");
  listPatterns();
  listPalettes();

  patterns.moveRandom(1); // start from a random pattern

  Serial.print("Starting with pattern: ");
  Serial.println(patterns.getCurrentPatternName());
  if (isMqttAvailable)
    mqttClient.publish(mqttPattern, patterns.getCurrentPatternName());

  patterns.start();
  ms_previous = millis();
  fps_timer = millis();
}

void loop()
{
  if (isWifiAvailable) ArduinoOTA.handle();
  
  if (isWifiAvailable && (false == (isMqttAvailable = mqttClient.loop()))) {
    DEBUG_PRINTLN("mqtt connection lost ... try re-connect");
    isMqttAvailable = mqttConnect() ? false : true;
  }
  // ***************************************************************************

  // menu.run(mainMenuItems, mainMenuItemCount);  

  if (auto_change && ((millis() - ms_previous) > ms_animation_max_duration)) {
    patterns.stop();
    patterns.moveRandom(1);
    //patterns.move(1);
    patterns.start();
       
    Serial.print("Changing pattern to:  ");
    Serial.println(patterns.getCurrentPatternName());
    if (isMqttAvailable)
      mqttClient.publish(mqttPattern, patterns.getCurrentPatternName());
 
    ms_previous = millis();

    // Select a random palette as well
    //effects.RandomPalette();
  }
 
  if (1000 / pattern_fps + last_frame < millis()) {
    last_frame = millis();
    pattern_fps = patterns.drawFrame();
    if (!pattern_fps)
      pattern_fps = default_fps;

    ++fps;
  }

  if (fps_timer + 1000 < millis()) {
    Serial.printf_P(PSTR("Effect fps: %ld\n"), fps);
    fps_timer = millis();
    fps = 0;
  }

}


void listPatterns()
{
  patterns.listPatterns();
}

void listPalettes()
{
  effects.listPalettes();
}
