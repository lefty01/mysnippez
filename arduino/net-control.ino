


int setupWifi() {
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("Connecting to wifi");

  unsigned retry_counter = 0;
  WiFi.begin(wifi_ssid, wifi_pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");

    retry_counter++;
    if (retry_counter > maxWifiWaitSeconds) {
      DEBUG_PRINTLN(" TIMEOUT!");
      return 1;
    }
  }
  ipAddr  = WiFi.localIP().toString();
  dnsAddr = WiFi.dnsIP().toString();

  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("WiFi connected");
  DEBUG_PRINTLN("IP address: ");
  DEBUG_PRINTLN(ipAddr);
  DEBUG_PRINTLN("DNS address: ");
  DEBUG_PRINTLN(dnsAddr);

  return 0;
}





/******************************************************************************
 *  MQTT
 */
int mqttConnect()
{
  DEBUG_PRINT("Attempting MQTT connection...");
  String connect_msg = "CONNECTED ";
  int rc = 0;
  boolean sub_rc;
  connect_msg += VERSION;

  // Attempt to connect
  //mqttClient.setKeepAlive(0); // "fix" timeouts
  if (mqttClient.connect(MQTTDEVICEID,
			 mqtt_user, mqtt_pass, mqttState, 1, 1, "OFFLINE")) {
    DEBUG_PRINTLN("connected");

    // Once connected, publish an announcement...
    mqttClient.publish(mqttState, connect_msg.c_str(), true);

    // topic subscriptions ... FIXME: check return code
    DEBUG_PRINT("subscripe to topic: ");
    DEBUG_PRINT(mqttPatternSet);
    sub_rc = mqttClient.subscribe(mqttPatternSet );
    DEBUG_PRINTF(" ... rc=%d\n", sub_rc);

    DEBUG_PRINT("subscribe to topic: ");
    DEBUG_PRINT(mqttPaletteSet);
    sub_rc = mqttClient.subscribe(mqttPaletteSet);
    DEBUG_PRINTF(" ... rc=%d\n", sub_rc);

    DEBUG_PRINT("subscribe to topic: ");
    DEBUG_PRINT(mqttBrightnessSet);
    sub_rc = mqttClient.subscribe(mqttBrightnessSet);
    DEBUG_PRINTF(" ... rc=%d\n", sub_rc);

    DEBUG_PRINT("subscribe to topic: ");
    DEBUG_PRINT(mqttModeSet);
    sub_rc = mqttClient.subscribe(mqttModeSet);
    DEBUG_PRINTF(" ... rc=%d\n", sub_rc);

    sub_rc = mqttClient.subscribe(mqttDurationSet);
    DEBUG_PRINTF("subscription to duration/set ... rc=%d\n", sub_rc);
    
    // mqttClient.subscribe();
  }
  else {
    DEBUG_PRINT("failed, mqttClient.state = ");
    DEBUG_PRINTLN(mqttClient.state());
    DEBUG_PRINT_MQTTSTATE(mqttClient.state());
    rc = 1;
  }
  delay(1000);
  return rc;
}

/*
  In order to republish this payload, a copy must be made
  as the orignal payload buffer will be overwritten whilst
  constructing the PUBLISH packet.
  Allocate the correct amount of memory for the payload copy

byte* p = (byte*)malloc(length);
// Copy the payload to the new buffer
memcpy(p,payload,length);
client.publish("outTopic", p, length);
// Free the memory
free(p);
*/
void mqttCallback(const char* topic, const byte* payload, unsigned int length)
{
  char value[length + 1];
  memcpy(value, payload, length);
  value[length] = '\0';

  DEBUG_PRINTF("Message arrived: [%s (len=%d)] %s (len=%d)\n",
	       topic, strlen(topic), value, length);

  // discard topic values greater than 128 bytes ...
  if (strlen(value) > 128) {
    DEBUG_PRINTLN("not allowing messages > 128 bytes");
    return;
  }

  if (0 == strcmp(topic, mqttPatternSet)) {
    DEBUG_PRINTLN("setting pattern via mqtt");
    // handle set pattern command ...
    // fixme: more sanity checks on payload
    String p(value);
    patterns.stop();

    bool ret = patterns.setPattern(p);
    if (!ret)
      DEBUG_PRINTLN("ERROR setting pattern");
    else
      DEBUG_PRINTLN("pattern set, now start");
    patterns.start();
  }

  if (0 == strcmp(topic, mqttPaletteSet)) {
    DEBUG_PRINTLN("setting palette via mqtt");

    // fixme: more sanity checks on payload
    String p(value);
    patterns.stop();

    bool ret = patterns.setPattern(p);
    if (!ret)
      DEBUG_PRINTLN("ERROR setting pattern");
    else
      DEBUG_PRINTLN("pattern set, now start");
    patterns.start();
  }

  if (0 == strcmp(topic, mqttModeSet)) {
    DEBUG_PRINTLN("setting auto change mode via mqtt");

    if (0 == strcmp(value, "change_auto")) {
      auto_change = true;
      mqttClient.publish(mqttMode, "auto", true);
    } else if (0 == strcmp(value, "change_manual")) {
      auto_change = false;
      mqttClient.publish(mqttMode, "manual", true);
    }
  }


  if (0 == strcmp(topic, mqttDurationSet)) {
    DEBUG_PRINTLN("setting auto change duration via mqtt");
    // convert duration string to integer also sanity check ...
    
  }
  //   if (isMqttAvailable) mqttClient.publish(mqttOpmode, mode2str(opMode), true);
  // }

}

