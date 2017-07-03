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

          if (json.containsKey("hostname")) strncpy(host_name, json["hostname"], 40);
          if (json.containsKey("s1name")) strncpy(s1name, json["s1name"], 40);
          if (json.containsKey("s2name")) strncpy(s2name, json["s2name"], 40);
          if (json.containsKey("s3name")) strncpy(s3name, json["s3name"], 40);
          if (json.containsKey("s4name")) strncpy(s4name, json["s4name"], 40);
          if (json.containsKey("s5name")) strncpy(s5name, json["s5name"], 40);
          if (json.containsKey("s1code")) strncpy(s1code, json["s1code"], 20);
          if (json.containsKey("s2code")) strncpy(s2code, json["s2code"], 20);
          if (json.containsKey("s3code")) strncpy(s3code, json["s3code"], 20);
          if (json.containsKey("s4code")) strncpy(s4code, json["s4code"], 20);
          if (json.containsKey("s5code")) strncpy(s5code, json["s5code"], 20);
          if (json.containsKey("s1off")) strncpy(s1off, json["s1off"], 20);
          if (json.containsKey("s2off")) strncpy(s2off, json["s2off"], 20);
          if (json.containsKey("s3off")) strncpy(s3off, json["s3off"], 20);
          if (json.containsKey("s4off")) strncpy(s4off, json["s4off"], 20);
          if (json.containsKey("s5off")) strncpy(s5off, json["s5off"], 20);
          if (json.containsKey("pulse")) strncpy(pulse, json["pulse"], 10);
          if (json.containsKey("protocol")) strncpy(protocol, json["protocol"], 10);
          if (json.containsKey("bits")) strncpy(bits, json["bits"], 10);
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
// Stream header HTML
//
void sendHeader() {
  sendHeader(200, false);
}

void sendHeader(int httpcode, bool redirect) {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(httpcode, "text/html; charset=utf-8", "");
  server.sendContent("<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n");
  server.sendContent("<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en'>\n");
  server.sendContent("  <head>\n");
  if (redirect)
  server.sendContent("    <meta http-equiv='refresh' content='10;URL=/' />\n");
  server.sendContent("    <meta name='viewport' content='width=device-width, initial-scale=0.75' />\n");
  server.sendContent("    <link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css' />\n");
  server.sendContent("    <style>@media (max-width: 767px) {.nav-pills>li {float: none; margin-left: 0; margin-top: 5px; text-align: center;}}</style>\n");
  server.sendContent("    <title>ESP8266 RF Controller (" + String(host_name) + ")</title>\n");
  server.sendContent("  </head>\n");
  server.sendContent("  <body>");
  server.sendContent("    <div class='container'>\n");
  server.sendContent("      <h1><a href='https://github.com/mdhiggins/RFSwitchController'>ESP8266 RF Controller</a></h1>\n");
  server.sendContent("      <div class='row'>\n");
  server.sendContent("        <div class='col-md-12'>\n");
  server.sendContent("          <ul class='nav nav-pills'>\n");
  server.sendContent("            <li class='active'>\n");
  server.sendContent("              <a href='http://" + String(host_name) + ".local" + ":" + String(port) + "'>Hostname <span class='badge'>" + String(host_name) + ".local" + ":" + String(port) + "</span></a></li>\n");
  server.sendContent("            <li class='active'>\n");
  server.sendContent("              <a href='http://" + ipToString(WiFi.localIP()) + ":" + String(port) + "'>Local <span class='badge'>" + ipToString(WiFi.localIP()) + ":" + String(port) + "</span></a></li>\n");
  server.sendContent("            <li class='active'>\n");
  server.sendContent("              <a href='#'>MAC <span class='badge'>" + String(WiFi.macAddress()) + "</span></a></li>\n");
  server.sendContent("          </ul>\n");
  server.sendContent("        </div>\n");
  server.sendContent("      </div><hr />\n");
}

void sendFooter() {
  server.sendContent("      <div class='row'><div class='col-md-12'><em>" + String(millis()) + "ms uptime</em></div></div>\n");
  server.sendContent("    </div>\n");
  server.sendContent("  </body>\n");
  server.sendContent("</html>\n");
  server.client().stop();
}

//+=============================================================================
// Stream main page HTML
//
void sendPage(String message, String header) {
  sendPage(message, header, 0);
}

void sendPage(String message, String header, int type) {
  sendPage(message, header, type, "", String(bits), String(protocol), String(pulse));
}

void sendPage(String message, String header, int type, int httpheader, bool redirect) {
  sendPage(message, header, type, "", String(bits), String(protocol), String(pulse), httpheader, redirect);
}

void sendPage(String message, String header, int type, String lc, String lb, String lpr, String lpu) {
  sendPage(message, header, type, lc, lb, lpr, lpu, 200, false);
}

void sendPage(String message, String header, int type, String lc, String lb, String lpr, String lpu, int httpheader, bool redirect) {
  sendHeader(httpheader, redirect);
  if (type == 1)
  server.sendContent("      <div class='row'><div class='col-md-12'><div class='alert alert-success'><strong>" + header + "!</strong> " + message + "</div></div></div>\n");
  if (type == 2)
  server.sendContent("      <div class='row'><div class='col-md-12'><div class='alert alert-warning'><strong>" + header + "!</strong> " + message + "</div></div></div>\n");
  if (type == 3)
  server.sendContent("      <div class='row'><div class='col-md-12'><div class='alert alert-danger'><strong>" + header + "!</strong> " + message + "</div></div></div>\n");
  server.sendContent("      <div class='row'>\n");
  server.sendContent("        <div class='col-md-12'>\n");
  server.sendContent("          <h3>Switches</h3>\n");
  server.sendContent("          <form method='post' action='/config/name'>\n");
  server.sendContent("            <table class='table table-striped' style='table-layout: fixed;'>\n");
  server.sendContent("              <thead><tr><th>Switch Name</th><th>On Command</th><th>Off Command</th></tr></thead>\n");
  server.sendContent("              <tbody>\n");
  server.sendContent("                <tr><td><div class='form-group' style='padding-right: 15%;'><input type='text' name='s1name' class='form-control' value='" + String(s1name) + "' /></div></td><td><a href='/config/switch?switch=1&amp;state=1'><code>" + String(s1code) + "</code></a></td><td><a href='/config/switch?switch=1&amp;state=0'><code>" + String(s1off) + "</code></a></td></tr>\n");
  server.sendContent("                <tr><td><div class='form-group' style='padding-right: 15%;'><input type='text' name='s2name' class='form-control' value='" + String(s2name) + "' /></div></td><td><a href='/config/switch?switch=2&amp;state=1'><code>" + String(s2code) + "</code></a></td><td><a href='/config/switch?switch=2&amp;state=0'><code>" + String(s2off) + "</code></a></td></tr>\n");
  server.sendContent("                <tr><td><div class='form-group' style='padding-right: 15%;'><input type='text' name='s3name' class='form-control' value='" + String(s3name) + "' /></div></td><td><a href='/config/switch?switch=3&amp;state=1'><code>" + String(s3code) + "</code></a></td><td><a href='/config/switch?switch=3&amp;state=0'><code>" + String(s3off) + "</code></a></td></tr>\n");
  server.sendContent("                <tr><td><div class='form-group' style='padding-right: 15%;'><input type='text' name='s4name' class='form-control' value='" + String(s4name) + "' /></div></td><td><a href='/config/switch?switch=4&amp;state=1'><code>" + String(s4code) + "</code></a></td><td><a href='/config/switch?switch=4&amp;state=0'><code>" + String(s4off) + "</code></a></td></tr>\n");
  server.sendContent("                <tr><td><div class='form-group' style='padding-right: 15%;'><input type='text' name='s5name' class='form-control' value='" + String(s5name) + "' /></div></td><td><a href='/config/switch?switch=5&amp;state=1'><code>" + String(s5code) + "</code></a></td><td><a href='/config/switch?switch=5&amp;state=0'><code>" + String(s5off) + "</code></a></td></tr>\n");
  server.sendContent("              </tbody>\n");
  server.sendContent("            </table>\n");
  server.sendContent("            <div class='form-group'>\n");
  server.sendContent("              <button type='submit' class='btn btn-default'>Update Names and Reboot</button></div>\n");
  server.sendContent("          </form>\n");
  server.sendContent("        </div>\n");
  server.sendContent("      </div>\n");
  server.sendContent("      <hr />\n");
  server.sendContent("      <div class='row'>\n");
  server.sendContent("        <div class='col-md-4 col-sm-12 col-xs-12'>\n");
  server.sendContent("          <div class='panel panel-default'>\n");
  server.sendContent("            <div class='panel-heading'><h3 class='panel-title'>Send Code</h3></div>\n");
  server.sendContent("            <div class='panel-body'>\n");
  server.sendContent("              <form class='form' method='post' action='/send'>\n");
  server.sendContent("                <div class='form-group'>\n");
  server.sendContent("                  <label for='lc'>Code</label>\n");
  server.sendContent("                  <input type='text' id='lc' name='code' class='form-control' value='" + lc + "' /> </div>\n");
  server.sendContent("                <div class='form-group'>\n");
  server.sendContent("                  <label for='lb'>Bits</label>\n");
  server.sendContent("                  <input type='text' id='lb' name='bits' class='form-control' value='" + lb + "' /> </div>\n");
  server.sendContent("                <div class='form-group'>\n");
  server.sendContent("                  <label for='lpr'>Protocol</label>\n");
  server.sendContent("                  <input type='text' id='lpr' name='protocol' class='form-control' value='" + lpr + "' /> </div>\n");
  server.sendContent("                <div class='form-group'>\n");
  server.sendContent("                  <label for='lpu'>Pulse</label>\n");
  server.sendContent("                  <input type='text' id='lpu' name='pulse' class='form-control' value='" + lpu + "' /> </div>\n");
  server.sendContent("                <div class='form-group'>\n");
  server.sendContent("                  <button type='submit' class='btn btn-default'>Send</button></div>\n");
  server.sendContent("              </form>\n");
  server.sendContent("            </div>\n");
  server.sendContent("          </div>\n");
  server.sendContent("        </div>\n");
  server.sendContent("        <div class='col-md-4 col-sm-12 col-xs-12'>\n");
  server.sendContent("          <div class='panel panel-default'>\n");
  server.sendContent("            <div class='panel-heading'><h3 class='panel-title'>Universal Parameters</h3></div>\n");
  server.sendContent("            <div class='panel-body'>\n");
  server.sendContent("              <form class='form' method='post' action='/config'>\n");
  server.sendContent("                <div class='form-group'>\n");
  server.sendContent("                  <label for='bits'>Bits</label>\n");
  server.sendContent("                  <input type='text' id='bits' name='bits' class='form-control' value='" + String(bits) + "' /> </div>\n");
  server.sendContent("                <div class='form-group'>\n");
  server.sendContent("                  <label for='protocol'>Protocol</label>\n");
  server.sendContent("                  <input type='text' id='protocol' name='protocol' class='form-control' value='" + String(protocol) + "' /> </div>\n");
  server.sendContent("                <div class='form-group'>\n");
  server.sendContent("                  <label for='pulse'>Pulse</label>\n");
  server.sendContent("                  <input type='text' id='pulse' name='pulse' class='form-control' value='" + String(pulse) + "' /> </div>\n");
  server.sendContent("                <div class='form-group'>\n");
  server.sendContent("                  <button type='submit' class='btn btn-default'>Save</button></div>\n");
  server.sendContent("              </form>\n");
  server.sendContent("            </div>\n");
  server.sendContent("          </div>\n");
  server.sendContent("        </div>\n");
  server.sendContent("        <div class='col-md-4 col-sm-12 col-xs-12'>\n");
  server.sendContent("          <div class='panel panel-default'>\n");
  server.sendContent("            <div class='panel-heading'><h3 class='panel-title'>Switch Override</h3></div>\n");
  server.sendContent("            <div class='panel-body'>\n");
  server.sendContent("              <form class='form' method='post' action='/config/switch'>\n");
  server.sendContent("                <div class='form-group'>\n");
  server.sendContent("                  <label for='switches'>Switches</label>\n");
  server.sendContent("                  <select id='switches' name='switch' class='form-control'>\n");
  server.sendContent("                    <option value='1'>" + String(s1name) + "</option>\n");
  server.sendContent("                    <option value='2'>" + String(s2name) + "</option>\n");
  server.sendContent("                    <option value='3'>" + String(s3name) + "</option>\n");
  server.sendContent("                    <option value='4'>" + String(s4name) + "</option>\n");
  server.sendContent("                    <option value='5'>" + String(s5name) + "</option>\n");
  server.sendContent("                  </select></div>\n");
  server.sendContent("                <div class='form-group'>\n");
  server.sendContent("                  <label for='oncode'>On Code</label>\n");
  server.sendContent("                  <input type='text' id='oncode' name='on' class='form-control'/> </div>\n");
  server.sendContent("                <div class='form-group'>\n");
  server.sendContent("                  <label for='offcode'>Off Code</label>\n");
  server.sendContent("                  <input type='text' id='offcode' name='off' class='form-control'/> </div>\n");
  server.sendContent("                <div class='form-group'>\n");
  server.sendContent("                  <button type='submit' class='btn btn-default'>Save</button></div>\n");
  server.sendContent("              </form>\n");
  server.sendContent("            </div>\n");
  server.sendContent("          </div>\n");
  server.sendContent("        </div>\n");
  server.sendContent("      </div>\n");
  server.sendContent("      <hr />\n");
  server.sendContent("      <div class='row'>\n");
  server.sendContent("        <div class='col-md-12'>\n");
  server.sendContent("          <ul class='list-unstyled'>\n");
  server.sendContent("            <li><span class='badge'>GPIO " + String(pinR) + "</span> Receiving </li>\n");
  server.sendContent("            <li><span class='badge'>GPIO " + String(pinT) + "</span> Transmitting </li>\n");
  server.sendContent("          </ul>\n");
  server.sendContent("        </div>\n");
  server.sendContent("      </div>\n");
  sendFooter();
}

void setupWemo() {
  wemoManager.begin();
  wemoSwitch1 = new WemoSwitch(String(s1name), 81, switchOneOn, switchOneOff);
  wemoSwitch2 = new WemoSwitch(String(s2name), 82, switchTwoOn, switchTwoOff);
  wemoSwitch3 = new WemoSwitch(String(s3name), 83, switchThreeOn, switchThreeOff);
  wemoSwitch4 = new WemoSwitch(String(s4name), 84, switchFourOn, switchFourOff);
  wemoSwitch5 = new WemoSwitch(String(s5name), 85, switchFiveOn, switchFiveOff);
  if (String(s1name) != "") wemoManager.addDevice(*wemoSwitch1);
  if (String(s2name) != "") wemoManager.addDevice(*wemoSwitch2);
  if (String(s3name) != "") wemoManager.addDevice(*wemoSwitch3);
  if (String(s4name) != "") wemoManager.addDevice(*wemoSwitch4);
  if (String(s5name) != "") wemoManager.addDevice(*wemoSwitch5);
}

void setup() {
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
  ticker.attach(2, disableLed);

  // Configure mDNS
  if (MDNS.begin(host_name)) {
    Serial.println("mDNS started. Hostname is set to " + String(host_name) + ".local");
  }
  MDNS.addService("http", "tcp", port);

  // Setup the WeMo emulator
  setupWemo();

  rcSwitch.enableTransmit(pinT);  // Pin with which to transmit on
  rcSwitch2.enableReceive(pinR);  // Pin to receive

  server.on("/config/name", []() {
    String s1n = server.arg("s1name");
    String s2n = server.arg("s2name");
    String s3n = server.arg("s3name");
    String s4n = server.arg("s4name");
    String s5n = server.arg("s5name");

    strncpy(s1name, s1n.c_str(), 20);
    strncpy(s2name, s2n.c_str(), 20);
    strncpy(s3name, s3n.c_str(), 20);
    strncpy(s4name, s4n.c_str(), 20);
    strncpy(s5name, s5n.c_str(), 20);
    saveConfig();

    sendPage("Switch name updated, device must restart, please refresh your browser", "Success", 1);
    ESP.reset(); // Device need to be restarted to reset the WeMo switches
  });

  server.on("/config/switch", []() {
    Serial.println("Connection received");
    if (server.hasArg("switch") && server.hasArg("on") && server.hasArg("off")) {
      String oncode = server.arg("on");
      String offcode = server.arg("off");
      switch (server.arg("switch").toInt()) {
        case 1:
          strncpy(s1code, oncode.c_str(), 20);
          strncpy(s1off, offcode.c_str(), 20);
          break;
        case 2:
          strncpy(s2code, oncode.c_str(), 20);
          strncpy(s2off, offcode.c_str(), 20);
          break;
        case 3:
          strncpy(s3code, oncode.c_str(), 20);
          strncpy(s3off, offcode.c_str(), 20);
          break;
        case 4:
          strncpy(s4code, oncode.c_str(), 20);
          strncpy(s4off, offcode.c_str(), 20);
          break;
        case 5:
          strncpy(s5code, oncode.c_str(), 20);
          strncpy(s5off, offcode.c_str(), 20);
          break;
      }
      sendPage("Code updated", "Success", 1);
    }
    else if (server.hasArg("switch") && server.hasArg("state")) {
      saveCode = server.arg("switch").toInt();
      onoff = server.arg("state").toInt() + 1;
      digitalWrite(BUILTIN_LED, LOW);
      ticker.attach(10, stopListening);
      sendPage("Listening for new code for 10 seconds. Page will automatically refresh after this time", "Alert", 2, 200, true);
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

    sendPage("Universal settings updated", "Success", 1);
  });

  server.on("/", []() {
    Serial.println("Connection received");
    sendPage("", "", 0);
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

    sendPage("Signal sent", "Success", 1, String(code), String(b), String(prot), String(pulse));
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
      Serial.print("Received: ");
      Serial.print(rcSwitch2.getReceivedValue());
      Serial.print(" / ");
      Serial.print(rcSwitch2.getReceivedBitlength());
      Serial.print("bit /");
      Serial.print(" protocol ");
      Serial.print(rcSwitch2.getReceivedProtocol());
      Serial.print(" / pulse ");
      Serial.println(rcSwitch2.getReceivedDelay());
    }
    rcSwitch2.resetAvailable();
  }
  delay(200);
}

void sendSignal(char* protocol, char* pulse, char* code, char* bits) {
  rcSwitch.setProtocol(atoi(protocol));
  rcSwitch.setPulseLength(atoi(pulse));
  rcSwitch.send(atoi(code), atoi(bits));
  digitalWrite(BUILTIN_LED, LOW);
  ticker.attach(0.5, disableLed);
  delay(200);
  rcSwitch2.resetAvailable();
}

void switchOneOn() {
  sendSignal(protocol, pulse, s1code, bits);
}

void switchOneOff() {
  sendSignal(protocol, pulse, s1off, bits);
}

void switchTwoOn() {
  sendSignal(protocol, pulse, s2code, bits);
}

void switchTwoOff() {
  sendSignal(protocol, pulse, s2off, bits);
}

void switchThreeOn() {
  sendSignal(protocol, pulse, s3code, bits);
}

void switchThreeOff() {
  sendSignal(protocol, pulse, s3off, bits);
}

void switchFourOn() {
  sendSignal(protocol, pulse, s4code, bits);
}

void switchFourOff() {
  sendSignal(protocol, pulse, s4off, bits);
}

void switchFiveOn() {
  sendSignal(protocol, pulse, s5code, bits);
}

void switchFiveOff() {
  sendSignal(protocol, pulse, s5off, bits);
}

// 15261954 (all on)
// 15261953 (all off)
