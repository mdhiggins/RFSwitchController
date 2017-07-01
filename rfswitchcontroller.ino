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
#include <ESP8266WebServer.h>     // Webserver

const int configpin = 13;         // GPIO13 (D7 on D1 Mini)
const char* wifi_config_name = "RFSwitch Configuration";
const int pinT = 4;
const int pinR = 5;
char host_name[40] = "";
char s1name[40] = "switch 1";
char s2name[40] = "switch 2";
char s3name[40] = "switch 3";
char s4name[40] = "switch 4";
char s5name[40] = "switch 5";
char s1code[20] = "15261967";
char s2code[20] = "15261965";
char s3code[20] = "15261963";
char s4code[20] = "15261959";
char s5code[20] = "15261957";
char s1off[20] = "15261966";
char s2off[20] = "15261964";
char s3off[20] = "15261962";
char s4off[20] = "15261958";
char s5off[20] = "15261956";

char protocol[10] = "1";
char bits[10] = "24";
char pulse[10] = "187";
char saveCode = 0;
char onoff = 0;
const int port = 80;

ESP8266WebServer server(port);

WemoManager wemoManager;
WemoSwitch *wemoSwitch1 = NULL;
WemoSwitch *wemoSwitch2 = NULL;
WemoSwitch *wemoSwitch3 = NULL;
WemoSwitch *wemoSwitch4 = NULL;
WemoSwitch *wemoSwitch5 = NULL;

RCSwitch rcSwitch = RCSwitch();
RCSwitch rcSwitch2 = RCSwitch();

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

void saveConfig() {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["hostname"] = host_name;
    json["s1name"] = s1name;
    json["s2name"] = s2name;
    json["s3name"] = s3name;
    json["s4name"] = s4name;
    json["s5name"] = s5name;
    json["s1code"] = s1code;
    json["s2code"] = s2code;
    json["s3code"] = s3code;
    json["s4code"] = s4code;
    json["s5code"] = s5code;
    json["s1off"] = s1off;
    json["s2off"] = s2off;
    json["s3off"] = s3off;
    json["s4off"] = s4off;
    json["s5off"] = s5off;
    json["protocol"] = protocol;
    json["pulse"] = pulse;
    json["bits"] = bits;

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

// Stop listening for codes
void stopListening()
{
  saveCode = 0;
  onoff = 0;
  disableLed();
}

void upd(int p, int pr, int b) {
  sprintf(pulse, "%d", p);
  sprintf(protocol, "%d", pr);
  sprintf(bits, "%d", b);
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
          strncpy(s1code, json["s1code"], 20);
          strncpy(s2code, json["s2code"], 20);
          strncpy(s3code, json["s3code"], 20);
          strncpy(s4code, json["s4code"], 20);
          strncpy(s5code, json["s5code"], 20);
          strncpy(s1off, json["s1off"], 20);
          strncpy(s2off, json["s2off"], 20);
          strncpy(s3off, json["s3off"], 20);
          strncpy(s4off, json["s4off"], 20);
          strncpy(s5off, json["s5off"], 20);
          strncpy(pulse, json["pulse"], 10);
          strncpy(protocol, json["protocol"], 10);
          strncpy(protocol, json["bits"], 10);
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

  Serial.println("WiFi connected! User chose hostname '" + String(host_name) + "'");

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    saveConfig();
  }
  ticker.detach();

  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);
  return true;
}

String ipToString(IPAddress ip)
{
  String s="";
  for (int i=0; i<4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
}

//+=============================================================================
// Wrap header and footer into content
//
String wrapPage(String &content) {
  String wrap = "<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en'><head><meta http-equiv='refresh' content='300' />";
  wrap += "<meta name='viewport' content='width=device-width, initial-scale=0.75' />";
  wrap += "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css' />";
  wrap += "<title>ESP8266 RF Controller (" + String(host_name) + ")</title></head><body>";
  wrap += "<div class='container'>";
  wrap +=   "<h1>ESP8266 RF Controller</h1>";
  wrap +=   "<div class='row'>";
  wrap +=     "<div class='col-md-12'>";
  wrap +=       "<ul class='nav nav-pills'>";
  wrap +=         "<li class='active'>";
  wrap +=           "<a href='http://" + String(host_name) + ".local" + ":" + String(port) + "'>Hostname <span class='badge'>" + String(host_name) + ".local" + ":" + String(port) + "</span></a></li>";
  wrap +=         "<li class='active'>";
  wrap +=           "<a href='http://" + ipToString(WiFi.localIP()) + ":" + String(port) + "'>Local <span class='badge'>" + ipToString(WiFi.localIP()) + ":" + String(port) + "</span></a></li>";
  wrap +=         "<li class='active'>";
  wrap +=           "<a href='#'>MAC <span class='badge'>" + String(WiFi.macAddress()) + "</span></a></li>";
  wrap +=       "</ul>";
  wrap +=     "</div>";
  wrap +=   "</div><hr />";
  wrap += content;
  wrap +=   "<div class='row'><div class='col-md-12'><em>" + String(millis()) + "ms uptime</em></div></div>";
  wrap += "</div>";
  wrap += "</body></html>";
  return wrap;
}

//+=============================================================================
// Generate info page HTML
//
String getPage(String message, String header, int type = 0) {
  String page = "";
  if (type == 1)
  page +=   "<div class='row'><div class='col-md-12'><div class='alert alert-success'><strong>" + header + "!</strong> " + message + "</div></div></div>";
  if (type == 2)
  page +=   "<div class='row'><div class='col-md-12'><div class='alert alert-warning'><strong>" + header + "!</strong> " + message + "</div></div></div>";
  if (type == 3)
  page +=   "<div class='row'><div class='col-md-12'><div class='alert alert-danger'><strong>" + header + "!</strong> " + message + "</div></div></div>";
  page +=   "<div class='row'>";
  page +=     "<div class='col-md-12'>";
  page +=       "<h3>Switches</h3>";
  page +=       "<table class='table table-striped' style='table-layout: fixed;'>";
  page +=         "<thead><tr><th>Switch Name</th><th>On Command</th><th>Off Command</th></tr></thead>";
  page +=         "<tbody>";
  page +=           "<tr><td><form class='form-inline' method='post' action='/config/switch'><div class='form-group'><input type='text' name='name' class='form-control' value='" + String(s1name) + "' /><button type='submit' class='btn btn-default'>&#8635;</button><input type='hidden' name='switch' value='1' /></div></form></td><td><a href='/config/switch?switch=1&amp;state=1'><code>" + String(s1code) + "</code></a></td><td><a href='/config/switch?switch=1&amp;state=0'><code>" + String(s1off) + "</code></a></td></tr>";
  page +=           "<tr><td><form class='form-inline' method='post' action='/config/switch'><div class='form-group'><input type='text' name='name' class='form-control' value='" + String(s2name) + "' /><button type='submit' class='btn btn-default'>&#8635;</button><input type='hidden' name='switch' value='2' /></div></form></td><td><a href='/config/switch?switch=2&amp;state=1'><code>" + String(s2code) + "</code></a></td><td><a href='/config/switch?switch=2&amp;state=0'><code>" + String(s2off) + "</code></a></td></tr>";
  page +=           "<tr><td><form class='form-inline' method='post' action='/config/switch'><div class='form-group'><input type='text' name='name' class='form-control' value='" + String(s3name) + "' /><button type='submit' class='btn btn-default'>&#8635;</button><input type='hidden' name='switch' value='3' /></div></form></td><td><a href='/config/switch?switch=3&amp;state=1'><code>" + String(s3code) + "</code></a></td><td><a href='/config/switch?switch=3&amp;state=0'><code>" + String(s3off) + "</code></a></td></tr>";
  page +=           "<tr><td><form class='form-inline' method='post' action='/config/switch'><div class='form-group'><input type='text' name='name' class='form-control' value='" + String(s4name) + "' /><button type='submit' class='btn btn-default'>&#8635;</button><input type='hidden' name='switch' value='4' /></div></form></td><td><a href='/config/switch?switch=4&amp;state=1'><code>" + String(s4code) + "</code></a></td><td><a href='/config/switch?switch=4&amp;state=0'><code>" + String(s4off) + "</code></a></td></tr>";
  page +=           "<tr><td><form class='form-inline' method='post' action='/config/switch'><div class='form-group'><input type='text' name='name' class='form-control' value='" + String(s5name) + "' /><button type='submit' class='btn btn-default'>&#8635;</button><input type='hidden' name='switch' value='5' /></div></form></td><td><a href='/config/switch?switch=5&amp;state=1'><code>" + String(s5code) + "</code></a></td><td><a href='/config/switch?switch=5&amp;state=0'><code>" + String(s5off) + "</code></a></td></tr>";
  page +=         "</tbody></table>";
  page +=     "</div></div>";
  page +=   "<div class='row'>";
  page +=     "<div class='col-md-12'>";
  page +=       "<ul class='list-unstyled'>";
  page +=         "<li><span class='badge'>GPIO " + String(pinR) + "</span> Receiving </li>";
  page +=         "<li><span class='badge'>GPIO " + String(pinT) + "</span> Transmitting </li>";
  page +=         "<li><span class='badge'>" + String(bits) + "</span> Bits </li>";
  page +=         "<li><span class='badge'>" + String(pulse) + "</span> Pulse </li>";
  page +=         "<li><span class='badge'>" + String(protocol) + "</span> Protocol </li>";
  page +=       "</ul>";
  page +=     "</div>";
  page +=   "</div>";
  return wrapPage(page);
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

  rcSwitch.enableTransmit(pinT); // Pin with which to transmit on
  rcSwitch2.enableReceive(pinR);  // Pin to receive

  server.on("/config/switch", []() {
    Serial.println("Connection received");
    if (server.hasArg("switch") && server.hasArg("state")) {
      saveCode = server.arg("switch").toInt();
      onoff = server.arg("state").toInt() + 1;
      digitalWrite(BUILTIN_LED, LOW);
      ticker.attach(10, stopListening);
      server.send(200, "text/html", getPage("Listening for new code for 10 seconds", "Alert", 2));
    } else if (server.hasArg("switch") && server.hasArg("name")) {
      String ns = server.arg("name");
      const char* nm = ns.c_str();
      switch (server.arg("switch").toInt()) {
        case 1:
          strncpy(s1name, nm, 20); break;
        case 2:
          strncpy(s2name, nm, 20); break;
        case 3:
          strncpy(s3name, nm, 20); break;
        case 4:
          strncpy(s4name, nm, 20); break;
        case 5:
          strncpy(s5name, nm, 20); break;
        default:
          break;
      }
      saveConfig();
      server.send(200, "text/html", getPage("Switch name updated", "Success", 1));
    }
  });

  server.on("/config", []() {
    Serial.println("Connection received");
    if (server.hasArg("pulse")) {
      strncpy(pulse, server.arg("pulse").c_str(), 10);
    }
    if (server.hasArg("protocol")) {
      strncpy(protocol, server.arg("protocol").c_str(), 10);
    }
    if (server.hasArg("bits")) {
      strncpy(bits, server.arg("bits").c_str(), 10);
    }
    saveConfig();
    server.send(200, "text/html", getPage("Global settings updated", "Success", 1));
  });

  server.on("/", []() {
    Serial.println("Connection received");
    server.send(200, "text/html", getPage("", "", 0));
  });

  server.on("/send", []() {
    Serial.println("Connection received");
    int prot = server.arg("protocol").toInt();
    int code = server.arg("code").toInt();
    int b = server.arg("bits").toInt();
    int pulse = server.arg("pulse").toInt();
    rcSwitch.setProtocol(prot);
    rcSwitch.setPulseLength(pulse);
    rcSwitch.send(code, b);
    server.send(200, "text/html", getPage("Sending code", "Success", 1));
  });

  server.begin();
  Serial.println("HTTP Server started on port " + String(port));
  Serial.println("Awaiting commands");
}

void loop() {
  wemoManager.serverLoop();
  server.handleClient();

  if (rcSwitch2.available()) {

    int value = rcSwitch2.getReceivedValue();

    if (value == 0) {
      Serial.print("Unknown encoding");
    } else {
      int pr = rcSwitch2.getReceivedProtocol();
      int p = rcSwitch2.getReceivedDelay();
      int b = rcSwitch2.getReceivedBitlength();
      switch (onoff) {
        case 0: break;
        case 1:
          switch (saveCode) {
            case 1: sprintf(s1off, "%d", value); upd(p, pr, b); saveConfig(); stopListening(); break;
            case 2: sprintf(s2off, "%d", value); upd(p, pr, b); saveConfig(); stopListening(); break;
            case 3: sprintf(s3off, "%d", value); upd(p, pr, b); saveConfig(); stopListening(); break;
            case 4: sprintf(s4off, "%d", value); upd(p, pr, b); saveConfig(); stopListening(); break;
            case 5: sprintf(s5off, "%d", value); upd(p, pr, b); saveConfig(); stopListening(); break;
            default: break;
          }
          break;
        case 2:
          switch (saveCode) {
            case 1: sprintf(s1code, "%d", value); upd(p, pr, b); saveConfig(); stopListening(); break;
            case 2: sprintf(s2code, "%d", value); upd(p, pr, b); saveConfig(); stopListening(); break;
            case 3: sprintf(s3code, "%d", value); upd(p, pr, b); saveConfig(); stopListening(); break;
            case 4: sprintf(s4code, "%d", value); upd(p, pr, b); saveConfig(); stopListening(); break;
            case 5: sprintf(s5code, "%d", value); upd(p, pr, b); saveConfig(); stopListening(); break;
            default: break;
          }
          break;
      }
      Serial.print("Received ");
      Serial.print(rcSwitch2.getReceivedValue());
      Serial.print(" / ");
      Serial.print(rcSwitch2.getReceivedBitlength());
      Serial.print("bit ");
      Serial.print("Protocol: ");
      Serial.println(rcSwitch2.getReceivedProtocol());
      Serial.print("Delay: ");
      Serial.println(rcSwitch2.getReceivedDelay());
    }
    rcSwitch2.resetAvailable();
  }
}

void switchOneOn() {
  rcSwitch.setProtocol(atoi(protocol));
  rcSwitch.setPulseLength(atoi(pulse));
  rcSwitch.send(atoi(s1code), atoi(bits));
  //rcSwitch.send(16165135, 24);
}

void switchOneOff() {
  rcSwitch.setProtocol(atoi(protocol));
  rcSwitch.setPulseLength(atoi(pulse));
  rcSwitch.send(atoi(s1off), atoi(bits));
  //rcSwitch.send(16165134, 24);
}

void switchTwoOn() {
  rcSwitch.setProtocol(atoi(protocol));
  rcSwitch.setPulseLength(atoi(pulse));
  rcSwitch.send(atoi(s2code), atoi(bits));
  //rcSwitch.send(16165133, 24);
}

void switchTwoOff() {
  rcSwitch.setProtocol(atoi(protocol));
  rcSwitch.setPulseLength(atoi(pulse));
  rcSwitch.send(atoi(s2off), atoi(bits));
  //rcSwitch.send(16165132, 24);
}

void switchThreeOn() {
  rcSwitch.setProtocol(atoi(protocol));
  rcSwitch.setPulseLength(atoi(pulse));
  rcSwitch.send(atoi(s3code), atoi(bits));
  //rcSwitch.send(16165131, 24);
}

void switchThreeOff() {
  rcSwitch.setProtocol(atoi(protocol));
  rcSwitch.setPulseLength(atoi(pulse));
  rcSwitch.send(atoi(s3off), atoi(bits));
  //rcSwitch.send(16165130, bits);
}

void switchFourOn() {
  rcSwitch.setProtocol(atoi(protocol));
  rcSwitch.setPulseLength(atoi(pulse));
  rcSwitch.send(atoi(s4code), atoi(bits));
  //rcSwitch.send(16165127, 24);
}

void switchFourOff() {
  rcSwitch.setProtocol(atoi(protocol));
  rcSwitch.setPulseLength(atoi(pulse));
  rcSwitch.send(atoi(s4off), atoi(bits));
  //rcSwitch.send(16165126, 24);
}

void switchFiveOn() {
  rcSwitch.setProtocol(atoi(protocol));
  rcSwitch.setPulseLength(atoi(pulse));
  rcSwitch.send(atoi(s5code), atoi(bits));
  //rcSwitch.send(16165125, 24);
}

void switchFiveOff() {
  rcSwitch.setProtocol(atoi(protocol));
  rcSwitch.setPulseLength(atoi(pulse));
  rcSwitch.send(atoi(s5off), atoi(bits));
  //rcSwitch.send(16165124, 24);
}

// 15261954 (all on)
// 15261953 (all off)
