#include "network_and_http.h"
#include <HTTPClient.h>
#include <WiFi.h>

bool connectToWifi(const char* ssid, const char* pass, unsigned long timeoutMs) {
if (!ssid || strlen(ssid) == 0) return false;
if (WiFi.isConnected()) {
// already connected but maybe to another AP; leave as-is
Serial.println("WiFi already connected");
return true;
}

WiFi.mode(WIFI_STA);
WiFi.begin(ssid, pass);

unsigned long start = millis();
Serial.println("Connecting to WiFi: " + String(ssid));

while (millis() - start < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected, IP: " + WiFi.localIP().toString());
        return true;
    }
    delay(250);
}

Serial.println("WiFi connection timed out");
return false;


}

void disconnectWifiGracefully() {
if (WiFi.isConnected()) {
WiFi.disconnect(true, true);
Serial.println("WiFi disconnected");
}
}

HttpResponse httpHead(const String& url) {
HttpResponse r;
HTTPClient http;
http.begin(url);
int code = http.sendRequest("HEAD");
r.statusCode = code;
r.ok = (code >= 200 && code < 300);
r.contentLength = http.getSize();
// getResponseHeader is not always available in embedded libs; left as a human note
r.reason = code == 200 ? String("OK") : String("HTTP") + String(code);
http.end();
return r;
}

void setDeviceHostname(const String& name) {
// small check to avoid empty names
if (name.length() == 0) return;
WiFi.setHostname(name.c_str());
Serial.println("Hostname set to " + name);
}