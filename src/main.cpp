#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

void callback(char* topic, byte* payload, unsigned int length);

#define RELAY_PORT 15


WiFiClient client;

// captive potal set

#define             EEPROM_LENGTH 1024
#define             RESET_PIN 0
ESP8266WebServer    webServer(80);
char                eRead[30];

char                ssid[30];
char                password[30];  
char                mqtt_address[30];  


// MQTT set
const int           mqttPort = 1883;

PubSubClient mqtt_client(client);



String responseHTML = ""
    "<!DOCTYPE html><html><head><title>CaptivePortal</title></head><body><center>"
    "<p>Captive Sample Server App</p>"
    "<form action='/save'>"
    "<p><input type='text' name='ssid' placeholder='SSID' onblur='this.value=removeSpaces(this.value);'></p>"
    "<p><input type='text' name='password' placeholder='WLAN Password'></p>"
    "<p><input type='text' name='MQTT' placeholder='MQTT address'></p>"
    "<p><input type='submit' value='Submit'></p></form>"
    "<p>This is a captive portal example</p></center></body>"
    "<script>function removeSpaces(string) {"
    "   return string.split(' ').join('');"
    "}</script></html>";

// Saves string to EEPROM
void SaveString(int startAt, const char* id) { 
    for (byte i = 0; i <= strlen(id); i++) {
        EEPROM.write(i + startAt, (uint8_t) id[i]);
    }
    EEPROM.commit();
}

// Reads string from EEPROM
void ReadString(byte startAt, byte bufor) {
    for (byte i = 0; i <= bufor; i++) {
        eRead[i] = (char)EEPROM.read(i + startAt);
    }
}

void save(){
    Serial.println("button pressed");
    Serial.println(webServer.arg("ssid"));
    SaveString( 0, (webServer.arg("ssid")).c_str());
    SaveString(30, (webServer.arg("password")).c_str());
    SaveString(60, (webServer.arg("MQTT")).c_str());
    webServer.send(200, "text/plain", "OK");
    ESP.restart();
}

void configWiFi() {
    const byte DNS_PORT = 53;
    IPAddress apIP(192, 168, 1, 1);
    DNSServer dnsServer;
    
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("chyuk_Potal");     // change this to your portal SSID
    
    dnsServer.start(DNS_PORT, "*", apIP);

    webServer.on("/save", save);

    webServer.onNotFound([]() {
        webServer.send(200, "text/html", responseHTML);
    });
    webServer.begin();
    while(true) {
        dnsServer.processNextRequest();
        webServer.handleClient();
        yield();
    }
}

void load_config_wifi() {
    ReadString(0, 30);
    if (!strcmp(eRead, "")) {
        Serial.println("Config Captive Portal started");
        configWiFi();
    } else {
        Serial.println("IOT Device started");
        strcpy(ssid, eRead);
        ReadString(30, 30);
        strcpy(password, eRead);
        ReadString(60, 30);
        strcpy(mqtt_address, eRead);
    }
}



IRAM_ATTR void GPIO0() {
    SaveString(0, ""); // blank out the SSID field in EEPROM
    ESP.restart();
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client1";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqtt_client.connect(clientId.c_str())) 
    {
      Serial.println("connected");
      // ... and resubscribe
      mqtt_client.subscribe("deviceid/chyuk2/cmd/lamp");
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
    Serial.begin(115200);
    EEPROM.begin(EEPROM_LENGTH);
    pinMode(RESET_PIN, INPUT_PULLUP);
    attachInterrupt(RESET_PIN, GPIO0, FALLING);
    while(!Serial);
    Serial.println();

    load_config_wifi(); // load or config wifi if not configured
   
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");

    // Wait for connection
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if(i++ > 15) {
            configWiFi();
        }
    }
    Serial.print("Connected to "); Serial.println(ssid);
    Serial.print("IP address: "); Serial.println(WiFi.localIP());

    Serial.println(mqtt_address);
    mqtt_client.setServer(mqtt_address, mqttPort);
    mqtt_client.setCallback(callback);
 
    while (!mqtt_client.connected()) {
        Serial.println("Connecting to MQTT...");
 
        if (mqtt_client.connect("ESP8266Client")) {
            Serial.println("connected");  
        } else {
            Serial.print("failed with state "); 
            Serial.println(mqtt_client.state());
            delay(2000);
        }
    }
    mqtt_client.subscribe("deviceid/chyuk2/cmd/lamp");


    // relay
    pinMode(RELAY_PORT, OUTPUT);

    Serial.println("Runtime Starting");  
}


void loop() 
{
    if (!mqtt_client.connected()) 
    {
      Serial.println("ERROR");
      reconnect();
    } 
    mqtt_client.loop();

}

void callback(char* topic, byte* payload, unsigned int length) 
{

    Serial.print(topic);
    Serial.print(" : ");
    for (int i = 0; i < (int)length; i++) 
    {
      Serial.print((char)payload[i]);
    }
    Serial.print("\n");

    if ((char)payload[0] == '1')
    {
      digitalWrite(RELAY_PORT, 1);
    }
    else
    {
      digitalWrite(RELAY_PORT, 0);
    }
    
}