#include <Arduino.h>
#include "CameraModule.h"
#include "NetworkModule.h"
#include "WebStreamModule.h"

// --- AP Settings ---
const char* ap_ssid = "SmartDoorbell_AP"; 
const char* ap_password = "doorbell_secure";

void setup() {
  Serial.begin(115200);
  delay(2000); 
  Serial.println("\n--- Doorbell Camera (Direct AP Mode) ---");

  if (Camera::init() == ESP_OK) {
    Serial.println("Camera: OK");
  } else {
    Serial.println("Camera: FAILED - Check connections");
    return; 
  }

  // Start Access Point
  Network::startAP(ap_ssid, ap_password);
  
  // Start Web Server
  WebStream::startServer();

  Serial.print("Stream Ready at: http://");
  Serial.println(Network::getIP());
  Serial.println("Connect your BeagleY-AI to the WiFi network above.");
}

void loop() {
  WebStream::handleClient();
  delay(2);
}