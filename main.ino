#include <Arduino.h>
#include <SPIFFS.h>
#include "network_and_http.h"
#include "download_engines.h"
#include "buffer_and_performance.h"
#include "spiffs_management.h"

// Tweak these to match your network
const char* WIFI_SSID = "Airtel_Vive_7601";
const char* WIFI_PASS = "9910907601@123";

// Example URL and target path — change when integrating
const String DOWNLOAD_URL = "https://httpbin.org/bytes/1024";
const String TARGET_PATH = "/downloaded.bin";

BufferManager globalBufMgr;
PerformanceMonitor globalPerf;
HttpDownloader httpDl;

void setup() {
Serial.begin(9600);
delay(100);

Serial.println("Starting up (humanized sketch)");

// SPIFFS mount
if (!startSPIFFS()) {
    Serial.println("SPIFFS failed to start. Continuing but file operations may fail.");
}

// WiFi connect
if (!connectToWifi(WIFI_SSID, WIFI_PASS, 20000)) {
    Serial.println("Unable to connect to WiFi — continuing with limited functionality.");
}

// Prepare buffer manager (try to enable smart allocation)
if (!globalBufMgr.allocateBuffers()) {
    Serial.println("Buffer allocation failed; continuing with minimal buffers.");
    // optionally try small fixed allocation
    globalBufMgr.allocateBuffers(8192, 4096);
}

// Attach helpers
httpDl.setBufferManager(&globalBufMgr);
httpDl.setPerformanceMonitor(&globalPerf);


}

void loop() {
// One-shot example: perform a single download and then halt (or sleep)
Serial.println("Starting download: " + DOWNLOAD_URL);

DownloadResult res = httpDl.download(DOWNLOAD_URL, TARGET_PATH);

if (res.success) {
    Serial.println("Downloaded successfully: " + String(res.totalBytes) + " bytes");
    globalPerf.printEnhancedResults(res.totalBytes);
} else {
    Serial.println("Download failed: " + res.errorMessage);
}

// done for demo purposes: sleep forever
Serial.println("Main loop finished — halting.");
while (true) {
    delay(1000);
}


}