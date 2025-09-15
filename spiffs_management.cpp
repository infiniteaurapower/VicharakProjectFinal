#include "spiffs_management.h"
#include <SPIFFS.h>

// Start/mount SPIFFS
bool startSPIFFS() {
Serial.println("Initializing SPIFFS.");

// pass true to format if mount fails
if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return false;
}

size_t totalBytes = 0, usedBytes = 0;
if (getSPIFFSInfo(totalBytes, usedBytes)) {
    Serial.println("SPIFFS Initialized Successfully");
    Serial.printf("Total: %d bytes, Used: %d bytes, Free: %d bytes\n", totalBytes, usedBytes, (int)(totalBytes - usedBytes));
}
return true;


}

bool mountSPIFFS() {
// small wrapper — we keep it lean
return startSPIFFS();
}

bool saveToSPIFFS(const String& path, const String& data) {
if (data.length() == 0) {
Serial.println("Warning: Attempting to save empty data");
return false;
}

if (!checkSPIFFSSpace(data.length())) {
    Serial.println("Error: Not enough SPIFFS space");
    return false;
}

File f = SPIFFS.open(path, FILE_WRITE);
if (!f) {
    Serial.println("Error: Failed to open file for writing: " + path);
    return false;
}

size_t bytesWritten = f.print(data);
f.close();

if (bytesWritten == data.length()) {
    Serial.println("File saved: " + path + " (" + String(bytesWritten) + " bytes)");
    return true;
} else {
    Serial.println("Write error: " + path + " (wrote " + String(bytesWritten) + "/" + String(data.length()) + " bytes)");
    return false;
}


}

void printFile(const String& path) {
readAndPrintFile(path);
}

void readAndPrintFile(const String& path) {
if (!SPIFFS.exists(path)) {
Serial.println("Error: File does not exist: " + path);
return;
}

File f = SPIFFS.open(path, FILE_READ);
if (!f) {
    Serial.println("Error: Failed to open file for reading: " + path);
    return;
}

Serial.println("\n=== File Content: " + path + " ===");
Serial.println("Size: " + String(f.size()) + " bytes");
Serial.println("Content:");
Serial.println("---");

// Using small chunk reads in case files are large — humans tend to be explicit
while (f.available()) {
    Serial.write(f.read());
}

Serial.println("\n=== End of File ===\n");
f.close();


}

void listSPIFFSFiles() {
Serial.println("\n=== SPIFFS File List ===");

File root = SPIFFS.open("/");
if (!root) {
    Serial.println("Error: Failed to open root directory");
    return;
}

if (!root.isDirectory()) {
    Serial.println("Error: Root is not a directory");
    root.close();
    return;
}

File file = root.openNextFile();
int fileCount = 0;
size_t totalSize = 0;

while (file) {
    if (file.isDirectory()) {
        Serial.printf("[DIR]  %s\n", file.name());
    } else {
        size_t fileSize = file.size();
        totalSize += fileSize;
        fileCount++;
        Serial.printf("[FILE] %s (%d bytes)\n", file.name(), (int)fileSize);
    }
    file = root.openNextFile();
}

root.close();

Serial.printf("\nTotal: %d files, %d bytes\n", fileCount, (int)totalSize);

size_t total, used;
if (getSPIFFSInfo(total, used)) {
    Serial.printf("SPIFFS: %d/%d bytes used (%.1f%%)\n", (int)used, (int)total, (used * 100.0f) / total);
}

Serial.println("========================\n");


}

bool getSPIFFSInfo(size_t& totalBytes, size_t& usedBytes) {
totalBytes = SPIFFS.totalBytes();
usedBytes = SPIFFS.usedBytes();

if (totalBytes == 0) {
    Serial.println("Warning: SPIFFS appears to be uninitialized");
    return false;
}
return true;


}

bool checkSPIFFSSpace(size_t requiredBytes) {
size_t totalBytes = 0, usedBytes = 0;
if (!getSPIFFSInfo(totalBytes, usedBytes)) {
return false;
}

size_t availableBytes = totalBytes - usedBytes;
// safety margin: at least 1KB or 10% of total, whichever is larger
size_t safetyMargin = max(totalBytes / 10, (size_t)1024);

if (requiredBytes + safetyMargin > availableBytes) {
    Serial.printf("Insufficient space: need %d bytes, available %d bytes (with %d bytes safety margin)\n",
                  (int)requiredBytes, (int)availableBytes, (int)safetyMargin);
    return false;
}
return true;


}

bool deleteSPIFFSFile(const String& path) {
if (!SPIFFS.exists(path)) {
Serial.println("File does not exist: " + path);
return false;
}
if (SPIFFS.remove(path)) {
Serial.println("File deleted: " + path);
return true;
} else {
Serial.println("Failed to delete file: " + path);
return false;
}
}

void formatSPIFFS() {
Serial.println("WARNING: Formatting SPIFFS will erase all data!");
Serial.println("This operation cannot be undone.");

// We do not attempt interactive confirmation in embedded runs by default.
// If someone wants interactive formatting later, we could implement it.
if (SPIFFS.format()) {
    Serial.println("SPIFFS formatted successfully");
} else {
    Serial.println("SPIFFS format failed");
}


}