#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "buffer_and_performance.h"

// Small wrapper utilities around WiFi and HTTP behavior — humanized names

bool connectToWifi(const char* ssid, const char* pass, unsigned long timeoutMs = 15000);
void disconnectWifiGracefully();

struct HttpRequest {
String url;
String method;
// headers can be tweaked; kept simple for embedded
std::vector<std::pair<String, String>> headers;
HttpRequest() : url(""), method("GET"), headers() {}
};

struct HttpResponse {
int statusCode;
size_t contentLength;
bool ok;
String reason;
// Note: body is intentionally omitted to avoid big copies; stream instead in download engines
HttpResponse() : statusCode(0), contentLength(0), ok(false), reason("") {}
};

// Thin helper to perform a quick HEAD request for probing
HttpResponse httpHead(const String& url);

// helper to set WiFi hostname (small utility)
void setDeviceHostname(const String& name);