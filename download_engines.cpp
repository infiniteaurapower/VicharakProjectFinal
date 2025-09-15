#include "download_engines.h"
#include <FS.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>

HttpDownloader::HttpDownloader()
: cancelled(false), bufMgr(nullptr), perf(nullptr), maxRetries(2) {
// little human note: default retries are conservative
}

HttpDownloader::~HttpDownloader() {
// nothing special
}

void HttpDownloader::cancel() {
cancelled = true;
}

// tiny helper to write data to SPIFFS
bool HttpDownloader::writeChunkToFile(const String& path, const uint8_t* data, size_t len, bool append) {
// some people prefer to open/close per chunk to be safe on embedded FS
File f;
if (append) f = SPIFFS.open(path, FILE_APPEND);
else f = SPIFFS.open(path, FILE_WRITE);

if (!f) {
    Serial.println("Failed to open file: " + path);
    return false;
}

size_t written = f.write(data, len);
f.close();

if (written != len) {
    Serial.println("Partial write: wrote " + String(written) + " of " + String(len) + " bytes");
    return false;
}
return true;


}

DownloadResult HttpDownloader::download(const String& url, const String& targetPath) {
DownloadResult result;
result.success = false;

if (!SPIFFS.begin(true)) {
    result.errorMessage = "SPIFFS not mounted";
    Serial.println("SPIFFS not mounted, aborting download");
    return result;
}

// prefer to allocate local buffers only if provided manager is available
size_t bufSize = DEFAULT_DOWNLOAD_BUFFER_SIZE;
if (bufMgr && bufMgr->getDownloadBufferSize() > 0) bufSize = bufMgr->getDownloadBufferSize();

// small local buffer if nothing else provided
uint8_t* localBuf = nullptr;
if (!bufMgr) {
    localBuf = (uint8_t*)malloc(bufSize);
    if (!localBuf) {
        result.errorMessage = "Failed to allocate temp buffer";
        Serial.println("Failed to allocate local temp buffer");
        return result;
    }
}

HTTPClient http;
int tries = 0;

while (tries <= maxRetries && !cancelled) {
    tries++;
    http.begin(url);
    // we keep it minimal; user can add headers externally if needed
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.println("HTTP error code: " + String(code) + ", try " + String(tries));
        http.end();
        if (tries > maxRetries) {
            result.httpStatusCode = code;
            result.errorMessage = "HTTP GET failed: " + String(code);
            break;
        } else {
            delay(300); // small backoff
            continue;
        }
    }

    // got a good response; stream it
    WiFiClient* stream = http.getStreamPtr();
    size_t total = http.getSize();
    size_t downloaded = 0;

    // initialize performance monitor if present
    if (perf) {
        perf->startMonitoring();
        perf->startConnectionTimer();
    }

    // create/truncate file first
    File out = SPIFFS.open(targetPath, FILE_WRITE);
    if (!out) {
        Serial.println("Failed to open output file: " + targetPath);
        result.errorMessage = "Failed to open output file";
        http.end();
        break;
    }
    out.close(); // we'll append as we get chunks

    // stream loop
    while (http.connected() && (total > 0 ? downloaded < total : stream->available()) && !cancelled) {
        size_t toRead = bufSize;
        if (total > 0) {
            // don't read past end
            if (toRead > (total - downloaded)) toRead = total - downloaded;
        }

        int available = stream->available();
        if (available == 0) {
            // no data available yet; give a tiny sleep
            delay(5);
            continue;
        }

        int readBytes = stream->readBytes((char*)(localBuf ? localBuf : bufMgr->getActiveDownloadBuffer()), toRead);
        if (readBytes <= 0) {
            // nothing read; break out if connection closed
            break;
        }

        // write to file
        bool ok = writeChunkToFile(targetPath, localBuf ? localBuf : bufMgr->getActiveDownloadBuffer(), readBytes, true);
        if (!ok) {
            result.errorMessage = "Write failed";
            break;
        }

        downloaded += readBytes;

        if (perf) {
            perf->updateProgress(downloaded);
        }
    } // end stream loop

    // wrap up
    if (perf) {
        perf->stopEnhancedMonitoring();
        perf->stopMonitoring();
    }

    result.fileSize = total;
    result.totalBytes = downloaded;
    result.httpStatusCode = code;
    result.success = (downloaded > 0 && !cancelled);
    if (localBuf) {
        free(localBuf);
        localBuf = nullptr;
    }

    http.end();
    break; // we either succeeded or had an error; break out of retry loop
} // end retry loop

if (cancelled) {
    result.errorMessage = "Cancelled by user";
    result.success = false;
}

return result;


}

// ---- ResumeDownloader ----

ResumeDownloader::ResumeDownloader() : HttpDownloader() {
// nothing fancy
}

ResumeDownloader::~ResumeDownloader() {}

long ResumeDownloader::probeRemoteSize(const String& url) {
HTTPClient http;
http.begin(url);
int code = http.sendRequest("HEAD");
long size = -1;
if (code == HTTP_CODE_OK || code == HTTP_CODE_MOVED_PERMANENTLY || code == HTTP_CODE_FOUND) {
size = http.getSize();
}
http.end();
return size;
}

bool ResumeDownloader::appendToExistingFile(const String& path, const uint8_t* data, size_t len) {
// this duplicates HttpDownloader::writeChunkToFile but it's fine — humans duplicate sometimes
File f = SPIFFS.open(path, FILE_APPEND);
if (!f) return false;
size_t w = f.write(data, len);
f.close();
return w == len;
}

DownloadResult ResumeDownloader::download(const String& url, const String& targetPath) {
// simple resume logic:
// 1. probe remote size
// 2. check local file size
// 3. start HTTP with Range header if needed
DownloadResult res;
res.success = false;

long remoteSize = probeRemoteSize(url);
size_t localSize = 0;

if (SPIFFS.exists(targetPath)) {
    File f = SPIFFS.open(targetPath, FILE_READ);
    if (f) {
        localSize = f.size();
        f.close();
    }
}

if (remoteSize > 0 && localSize >= (size_t)remoteSize) {
    res.success = true;
    res.fileSize = remoteSize;
    res.totalBytes = localSize;
    res.errorMessage = "Already complete";
    Serial.println("File already downloaded, skipping");
    return res;
}

// otherwise fall back to parent implementation but try resume via headers
// for simplicity we'll just call the parent download (no range) — real resume would add Range header
// leaving a commented-out idea for later:
// // setRangeHeader(localSize)
res = HttpDownloader::download(url, targetPath);
return res;
}