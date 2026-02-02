#ifndef NETWORK_MODULE_H
#define NETWORK_MODULE_H

#include <WiFi.h>

class Network {
public:
  static void startAP(const char* ssid, const char* password) {
    WiFi.mode(WIFI_AP);
    if (WiFi.softAP(ssid, password)) {
        Serial.println("\n[WiFi] Access Point Started!");
        Serial.print("[WiFi] SSID: "); Serial.println(ssid);
        Serial.print("[WiFi] IP Address: "); Serial.println(WiFi.softAPIP()); 
    } else {
        Serial.println("[WiFi] AP Start Failed!");
    }
  }
  static IPAddress getIP() { return WiFi.softAPIP(); }
};
#endif