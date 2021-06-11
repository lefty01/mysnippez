#ifndef _wifi_mqtt_creds_h_
#define _wifi_mqtt_creds_h_

#ifndef NOMQTTCERTS
#include "server_mqtt.crt.h"
#include "client.crt.h"
#include "client.key.h"
#endif

#ifdef USE_ANDROID_AP
const char* wifi_ssid = "TETHERING_AP";
const char* wifi_pass = "********";
#else
const char* wifi_ssid = "MY_HOME_AP";
const char* wifi_pass = "********";
#endif

const char*    mqtt_host = "********";
const char*    mqtt_user = "********";
const char*    mqtt_pass = "********";
const unsigned mqtt_port = 123;

#endif
