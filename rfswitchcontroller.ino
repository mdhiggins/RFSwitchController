#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <CallbackFunction.h>
#include <WemoManager.h>
#include <WemoSwitch.h>
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>          // useful to access to ESP by hostname.local
#include <RCSwitch.h>             // RCSwitch Library for transmitting
#include <ArduinoJson.h>          // JSON handler
#include <Ticker.h>               // For LED status

const int configpin = 13;         // GPIO13 (D7 on D1 Mini)
const char* wifi_config_name = "RFSwitch Configuration";
char host_name[40] = "";
char s1name[40] = "";
char s2name[40] = "";
char s3name[40] = "";
char s4name[40] = "";
char s5name[40] = "";
const int port = 80;

WemoManager wemoManager;
WemoSwitch *wemoSwitch1 = NULL;
WemoSwitch *wemoSwitch2 = NULL;
WemoSwitch *wemoSwitch3 = NULL;
WemoSwitch *wemoSwitch4 = NULL;
WemoSwitch *wemoSwitch5 = NULL;

RCSwitch rcSwitch = RCSwitch();

Ticker ticker;
bool shouldSaveConfig = false;    //flag for saving data

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// Toggle state
void tick()
{
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

// Turn off the Led after timeout
void disableLed()
{
  Serial.println("Turning off the LED to save power.");
  digitalWrite(BUILTIN_LED, HIGH);     // Shut down the LED
  ticker.detach();                     // Stopping the ticker
}


// Gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  ticker.attach(0.2, tick);
}

// First setup of the Wifi.
// If return true, the Wifi is well connected.
// Should not return false if Wifi cannot be connected, it will loop
bool setupWifi(bool resetConf) {
  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  if (resetConf)
    wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strncpy(host_name, json["hostname"], 40);
          strncpy(s1name, json["s1name"], 40);
          strncpy(s2name, json["s2name"], 40);
          strncpy(s3name, json["s3name"], 40);
          strncpy(s4name, json["s4name"], 40);
          strncpy(s5name, json["s5name"], 40);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  WiFiManagerParameter custom_hostname("hostname", "Choose a hostname to this RFtransmitter", host_name, 40);
  wifiManager.addParameter(&custom_hostname);
  WiFiManagerParameter custom_switch1("s1name", "Choose a name for switch 1", s1name, 40);
  wifiManager.addParameter(&custom_switch1);
  WiFiManagerParameter custom_switch2("s2name", "Choose a name for switch 2", s2name, 40);
  wifiManager.addParameter(&custom_switch2);
  WiFiManagerParameter custom_switch3("s3name", "Choose a name for switch 3", s3name, 40);
  wifiManager.addParameter(&custom_switch3);
  WiFiManagerParameter custom_switch4("s4name", "Choose a name for switch 4", s4name, 40);
  wifiManager.addParameter(&custom_switch4);
  WiFiManagerParameter custom_switch5("s5name", "Choose a name for switch 5", s5name, 40);
  wifiManager.addParameter(&custom_switch5);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(wifi_config_name)) {
    Serial.println("failed to connect and hit timeout");
    // reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  strncpy(host_name, custom_hostname.getValue(), 40);
  strncpy(s1name, custom_switch1.getValue(), 40);
  strncpy(s2name, custom_switch2.getValue(), 40);
  strncpy(s3name, custom_switch3.getValue(), 40);
  strncpy(s4name, custom_switch4.getValue(), 40);
  strncpy(s5name, custom_switch5.getValue(), 40);

  Serial.println("WiFi connected! User chose hostname '" + String(host_name) + "'");
  Serial.println(s1name);
  Serial.println(s2name);
  Serial.println(s3name);
  Serial.println(s4name);
  Serial.println(s5name);

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println(" config...");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["hostname"] = host_name;
    json["s1name"] = s1name;
    json["s2name"] = s2name;
    json["s3name"] = s3name;
    json["s4name"] = s4name;
    json["s5name"] = s5name;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    Serial.println("");
    json.printTo(configFile);
    configFile.close();
    //end save
  }
  ticker.detach();

  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);
  return true;
}

void setup() {
  // serial init
  Serial.begin(115200);

  if (!setupWifi(digitalRead(configpin) == LOW))
    return;

  WiFi.hostname(host_name);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  wifi_set_sleep_type(LIGHT_SLEEP_T);
  digitalWrite(BUILTIN_LED, LOW);
  // Turn off the led in 5s
  ticker.attach(5, disableLed);

  // Configure mDNS
  if (MDNS.begin(host_name)) {
    Serial.println("mDNS started. Hostname is set to " + String(host_name) + ".local");
  }
  MDNS.addService("http", "tcp", port); // Anounce the ESP as an HTTP service

  // Setup the WeMo emulator
  wemoManager.begin();
  wemoSwitch1 = new WemoSwitch(String(s1name), 81, switchOneOn, switchOneOff);
  wemoSwitch2 = new WemoSwitch(String(s2name), 82, switchTwoOn, switchTwoOff);
  wemoSwitch3 = new WemoSwitch(String(s3name), 83, switchThreeOn, switchThreeOff);
  wemoSwitch4 = new WemoSwitch(String(s4name), 84, switchFourOn, switchFourOff);
  wemoSwitch5 = new WemoSwitch(String(s5name), 85, switchFiveOn, switchFiveOff);
  if (String(s1name) !=  "") {
    wemoManager.addDevice(*wemoSwitch1);
  }
  if (String(s2name) != "") {
    wemoManager.addDevice(*wemoSwitch2);
  }

  if (String(s3name) != "") {
    wemoManager.addDevice(*wemoSwitch3);
  }

  if (String(s4name) != "") {
    wemoManager.addDevice(*wemoSwitch4);
  }

  if (String(s5name) != "") {
    wemoManager.addDevice(*wemoSwitch5);
  }

  rcSwitch.enableTransmit(4); // Pin with which to transmit on

  Serial.println("Awaiting commands");
}

void loop() {
  wemoManager.serverLoop();
}

void switchOneOn() {
  rcSwitch.setProtocol(1);
  rcSwitch.send(15261967, 24);
}

void switchOneOff() {
  rcSwitch.setProtocol(1);
  rcSwitch.send(15261966, 24);
}

void switchTwoOn() {
  rcSwitch.setProtocol(1);
  rcSwitch.send(15261965, 24);
}

void switchTwoOff() {
  rcSwitch.setProtocol(1);
  rcSwitch.send(15261964, 24);
}

void switchThreeOn() {
  rcSwitch.setProtocol(1);
  rcSwitch.send(15261963, 24);
}

void switchThreeOff() {
  rcSwitch.setProtocol(1);
  rcSwitch.send(15261962, 24);
}

void switchFourOn() {
  rcSwitch.setProtocol(1);
  rcSwitch.send(15261959, 24);
}

void switchFourOff() {
  rcSwitch.setProtocol(1);
  rcSwitch.send(15261958, 24);
}

void switchFiveOn() {
  rcSwitch.setProtocol(1);
  rcSwitch.send(15261957, 24);
}

void switchFiveOff() {
  rcSwitch.setProtocol(1);
  rcSwitch.send(15261956, 24);
}

// 15261954 (all on)
// 15261953 (all off)
