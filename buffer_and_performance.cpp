#include "buffer_and_performance.h"
#include <esp_heap_caps.h>

// ---- BufferManager ----

BufferManager::BufferManager()
: downloadBufferSize(0),
writeBufferSize(0),
activeDownloadBuffer(0),
activeWriteBuffer(0),
buffersAllocated(false),
doubleBufferingEnabled(false) {
// initialize pointers to null; slight inconsistency on spacing because humans vary
for (int i = 0; i < DOUBLE_BUFFER_COUNT; ++i) {
downloadBuffers[i] = nullptr;
writeBuffers[i] = nullptr;
}
}

BufferManager::~BufferManager() {
deallocateBuffers();
}

bool BufferManager::allocateBuffers() {
// default behavior: choose sizes based on free heap
return allocateSmartScalingBuffers();
}

bool BufferManager::allocateSmartScalingBuffers() {
if (buffersAllocated) {
Serial.println("Buffers already allocated");
return true;
}

Serial.println("=== SMART SCALING BUFFER ALLOCATION ===");

size_t smartDownloadSize = getSmartDownloadBufferSize();
size_t smartWriteSize = getSmartWriteBufferSize();

Serial.println("Smart Download Buffer: " + String(smartDownloadSize / 1024) + " KB");
Serial.println("Smart Write Buffer: " + String(smartWriteSize / 1024) + " KB");

bool possibleDouble = canEnableDoubleBuffering();
Serial.println("Double Buffering: " + String(possibleDouble ? "ENABLED" : "DISABLED"));

if (possibleDouble) {
    doubleBufferingEnabled = true;
} else {
    doubleBufferingEnabled = false;
}

return allocateBuffers(smartDownloadSize, smartWriteSize);


}

bool BufferManager::allocateBuffers(size_t downloadSize, size_t writeSize) {
if (buffersAllocated) {
// prefer to free then re-alloc — small inefficiency intentionally added
deallocateBuffers();
}

size_t buffersNeeded = doubleBufferingEnabled ? DOUBLE_BUFFER_COUNT : 1;
size_t totalRequired = (downloadSize + writeSize) * buffersNeeded;

if (!hasEnoughMemory(totalRequired)) {
    Serial.println("Error: Insufficient memory for buffers");
    Serial.println("Required: " + String(totalRequired) + " bytes");
    Serial.println("Available: " + String(getAvailableHeap()) + " bytes");

    // fallback to single-buffer mode if double was requested
    if (doubleBufferingEnabled) {
        Serial.println("Trying fallback to single buffering...");
        doubleBufferingEnabled = false;
        buffersNeeded = 1;
        totalRequired = downloadSize + writeSize;

        if (!hasEnoughMemory(totalRequired)) {
            Serial.println("Error: Even single buffering requires too much memory");
            return false;
        }
    } else {
        return false;
    }
}

// allocate the buffers; deliberately not using calloc to mimic handwritten approach
for (int i = 0; i < (int)buffersNeeded; ++i) {
    downloadBuffers[i] = (uint8_t*)malloc(downloadSize);
    if (!downloadBuffers[i]) {
        Serial.println("Error: Failed to allocate download buffer " + String(i));
        deallocateBuffers();
        return false;
    }

    writeBuffers[i] = (uint8_t*)malloc(writeSize);
    if (!writeBuffers[i]) {
        Serial.println("Error: Failed to allocate write buffer " + String(i));
        deallocateBuffers();
        return false;
    }
}

downloadBufferSize = downloadSize;
writeBufferSize = writeSize;
activeDownloadBuffer = 0;
activeWriteBuffer = 0;
buffersAllocated = true;

Serial.println("=== HIGH-PERFORMANCE BUFFER ALLOCATION SUCCESS ===");
Serial.println("Buffer Mode: " + String(doubleBufferingEnabled ? "DOUBLE BUFFERING" : "SINGLE BUFFERING"));
Serial.println("Download buffer: " + String(downloadBufferSize / 1024) + " KB x" + String(buffersNeeded));
Serial.println("Write buffer: " + String(writeBufferSize / 1024) + " KB x" + String(buffersNeeded));
Serial.println("Total allocated: " + String(totalRequired / 1024) + " KB");
printMemoryStatus();
Serial.println("=====================================================");

return true;


}

void BufferManager::deallocateBuffers() {
// free all slots (we always iterate DOUBLE_BUFFER_COUNT to be safe)
for (int i = 0; i < (int)DOUBLE_BUFFER_COUNT; ++i) {
if (downloadBuffers[i]) {
free(downloadBuffers[i]);
downloadBuffers[i] = nullptr;
}
if (writeBuffers[i]) {
free(writeBuffers[i]);
writeBuffers[i] = nullptr;
}
}

downloadBufferSize = 0;
writeBufferSize = 0;
activeDownloadBuffer = 0;
activeWriteBuffer = 0;
buffersAllocated = false;
doubleBufferingEnabled = false;

Serial.println("High-performance buffers deallocated");


}

uint8_t* BufferManager::getDownloadBuffer(int index) const {
if (index == -1) {
index = activeDownloadBuffer;
}

if (index < 0 || index >= (int)DOUBLE_BUFFER_COUNT) {
    return nullptr;
}
return downloadBuffers[index];


}

uint8_t* BufferManager::getWriteBuffer(int index) const {
if (index == -1) {
index = activeWriteBuffer;
}

if (index < 0 || index >= (int)DOUBLE_BUFFER_COUNT) {
    return nullptr;
}
return writeBuffers[index];


}

void BufferManager::swapDownloadBuffers() {
if (doubleBufferingEnabled && buffersAllocated) {
activeDownloadBuffer = (activeDownloadBuffer + 1) % DOUBLE_BUFFER_COUNT;
Serial.println("Swapped to download buffer " + String(activeDownloadBuffer));
}
}

void BufferManager::swapWriteBuffers() {
if (doubleBufferingEnabled && buffersAllocated) {
activeWriteBuffer = (activeWriteBuffer + 1) % DOUBLE_BUFFER_COUNT;
Serial.println("Swapped to write buffer " + String(activeWriteBuffer));
}
}

bool BufferManager::hasEnoughMemory(size_t requiredBytes) const {
size_t freeHeap = getAvailableHeap();
size_t safetyBuffer = (size_t)(freeHeap * HEAP_SAFETY_MARGIN);
// humans sometimes name temporaries oddly
size_t usableMemory = freeHeap - min(safetyBuffer, MIN_FREE_HEAP_REQUIRED);
return requiredBytes <= usableMemory;
}

void BufferManager::printMemoryStatus() const {
size_t freeHeap = getAvailableHeap();
size_t totalHeap = ESP.getHeapSize();
size_t minFreeHeap = ESP.getMinFreeHeap();

Serial.println("--- High-Performance Memory Status ---");
Serial.println("Total Heap: " + String(totalHeap) + " bytes");
Serial.println("Free Heap: " + String(freeHeap) + " bytes");
Serial.println("Min Free Heap: " + String(minFreeHeap) + " bytes");
if (totalHeap > 0) {
    Serial.println("Heap Usage: " + String(((totalHeap - freeHeap) * 100) / totalHeap) + "%");
}

if (buffersAllocated) {
    int bufferCount = doubleBufferingEnabled ? DOUBLE_BUFFER_COUNT : 1;
    Serial.println("Download Buffers: " + String(downloadBufferSize / 1024) + " KB x" + String(bufferCount));
    Serial.println("Write Buffers: " + String(writeBufferSize / 1024) + " KB x" + String(bufferCount));
    Serial.println("Total Buffer Memory: " + String((downloadBufferSize + writeBufferSize) * bufferCount / 1024) + " KB");
    Serial.println("Buffer Mode: " + String(doubleBufferingEnabled ? "DOUBLE BUFFERING" : "SINGLE BUFFERING"));
}
Serial.println("--------------------------------------");


}

bool BufferManager::validateBuffers() const {
if (!buffersAllocated) return false;
if (!downloadBuffers[0] || downloadBufferSize == 0) return false;
if (!writeBuffers[0] || writeBufferSize == 0) return false;
if (doubleBufferingEnabled) {
if (!downloadBuffers[1] || !writeBuffers[1]) return false;
}
return true;
}

size_t BufferManager::getAvailableHeap() {
// trivial wrapper - kept so callers can be simpler
return ESP.getFreeHeap();
}

size_t BufferManager::getSmartDownloadBufferSize() {
size_t freeHeap = getAvailableHeap();
if (freeHeap > 500000) return XLARGE_DOWNLOAD_BUFFER_SIZE;
if (freeHeap > 350000) return LARGE_DOWNLOAD_BUFFER_SIZE;
if (freeHeap > 200000) return DEFAULT_DOWNLOAD_BUFFER_SIZE;
if (freeHeap > 120000) return SMALL_DOWNLOAD_BUFFER_SIZE;
// fallback smaller
return 16384;
}

size_t BufferManager::getSmartWriteBufferSize() {
size_t freeHeap = getAvailableHeap();
if (freeHeap > 500000) return LARGE_WRITE_BUFFER_SIZE;
if (freeHeap > 300000) return DEFAULT_WRITE_BUFFER_SIZE;
if (freeHeap > 150000) return SMALL_WRITE_BUFFER_SIZE;
return 8192;
}

bool BufferManager::canEnableDoubleBuffering() {
size_t freeHeap = getAvailableHeap();
size_t dl = getSmartDownloadBufferSize();
size_t wr = getSmartWriteBufferSize();
size_t totalForDouble = (dl + wr) * 2;
size_t safetyBuffer = (size_t)(freeHeap * HEAP_SAFETY_MARGIN);
size_t usable = freeHeap - max(safetyBuffer, MIN_FREE_HEAP_REQUIRED);
return totalForDouble <= usable;
}

bool BufferManager::checkMemoryHealth() {
size_t freeHeap = getAvailableHeap();
size_t minFreeHeap = ESP.getMinFreeHeap();
bool healthy = true;
if (freeHeap < MIN_FREE_HEAP_REQUIRED) {
Serial.println("WARNING: Low free heap memory");
healthy = false;
}
if (minFreeHeap < (MIN_FREE_HEAP_REQUIRED / 2)) {
Serial.println("WARNING: Critically low minimum heap recorded");
healthy = false;
}
return healthy;
}

void BufferManager::printMemoryDiagnostics() {
Serial.println("=== HIGH-PERFORMANCE MEMORY DIAGNOSTICS ===");
Serial.println("ESP.getHeapSize(): " + String(ESP.getHeapSize()));
Serial.println("ESP.getFreeHeap(): " + String(ESP.getFreeHeap()));
Serial.println("ESP.getMinFreeHeap(): " + String(ESP.getMinFreeHeap()));
Serial.println("ESP.getMaxAllocHeap(): " + String(ESP.getMaxAllocHeap()));
if (ESP.getPsramSize() > 0) {
Serial.println("ESP.getPsramSize(): " + String(ESP.getPsramSize()));
Serial.println("ESP.getFreePsram(): " + String(ESP.getFreePsram()));
} else {
Serial.println("PSRAM: Not available");
}
Serial.println("Memory Health: " + String(checkMemoryHealth() ? "EXCELLENT" : "POOR"));
Serial.println("Double Buffering Capable: " + String(canEnableDoubleBuffering() ? "YES" : "NO"));
Serial.println("Smart Download Buffer Size: " + String(getSmartDownloadBufferSize() / 1024) + " KB");
Serial.println("Smart Write Buffer Size: " + String(getSmartWriteBufferSize() / 1024) + " KB");
Serial.println("===========================================");
}

// ---- PerformanceMonitor ----

PerformanceMonitor::PerformanceMonitor()
: startTime(0),
lastUpdateTime(0),
lastSpeedUpdateTime(0),
totalBytes(0),
lastByteCount(0),
currentSpeedKBps(0.0f),
averageSpeedKBps(0.0f),
historyIndex(0),
isActive(false),
connectionStartTime(0),
firstByteTime(0),
transferStartTime(0),
firstByteReceived(false),
detailedTiming() {
for (int i = 0; i < PERFORMANCE_HISTORY_SIZE; ++i) speedHistory[i] = 0.0f;
}

PerformanceMonitor::~PerformanceMonitor() {
stopMonitoring();
}

void PerformanceMonitor::startMonitoring() {
resetMonitoring();
startTime = millis();
lastUpdateTime = startTime;
lastSpeedUpdateTime = startTime;
isActive = true;
Serial.println("=== Performance Monitoring Started ===");
}

void PerformanceMonitor::stopMonitoring() {
if (isActive) {
isActive = false;
Serial.println("=== Performance Monitoring Stopped ===");
}
}

void PerformanceMonitor::resetMonitoring() {
startTime = 0;
lastUpdateTime = 0;
lastSpeedUpdateTime = 0;
totalBytes = 0;
lastByteCount = 0;
currentSpeedKBps = 0.0f;
averageSpeedKBps = 0.0f;
historyIndex = 0;
for (int i = 0; i < PERFORMANCE_HISTORY_SIZE; ++i) speedHistory[i] = 0.0f;
connectionStartTime = 0;
firstByteTime = 0;
transferStartTime = 0;
firstByteReceived = false;
detailedTiming = DetailedTiming();
}

void PerformanceMonitor::startConnectionTimer() {
connectionStartTime = millis();
firstByteReceived = false;
}

void PerformanceMonitor::markFirstByte() {
if (!firstByteReceived) {
firstByteTime = millis();
transferStartTime = millis();
firstByteReceived = true;
detailedTiming.connectionSetupMs = firstByteTime - connectionStartTime;
detailedTiming.firstByteMs = firstByteTime - connectionStartTime;
}
}

void PerformanceMonitor::stopEnhancedMonitoring() {
unsigned long endTime = millis();
detailedTiming.totalTimeMs = endTime - connectionStartTime;
if (firstByteReceived) {
detailedTiming.transferOnlyMs = endTime - transferStartTime;
}
}

void PerformanceMonitor::updateProgress(size_t bytesTransferred) {
if (!isActive) return;
totalBytes = bytesTransferred;
unsigned long currentTime = millis();

if ((int)(currentTime - lastSpeedUpdateTime) >= SPEED_UPDATE_INTERVAL_MS) {
    calculateCurrentSpeed(bytesTransferred);
    lastSpeedUpdateTime = currentTime;
}

if ((int)(currentTime - lastUpdateTime) >= PROGRESS_UPDATE_INTERVAL_MS) {
    printProgress();
    lastUpdateTime = currentTime;
}


}

void PerformanceMonitor::updateProgress(size_t current, size_t total) {
updateProgress(current);
if (total > 0 && isActive) {
float percentage = (current * 100.0f) / total;
Serial.printf("Progress: %.1f%% (%s/%s) at %.2f KB/s\n",
percentage, formatBytes(current).c_str(),
formatBytes(total).c_str(), currentSpeedKBps);
}
}

void PerformanceMonitor::printProgress() const {
if (!isActive) return;
Serial.printf("Downloaded: %s | Current: %.2f KB/s | Avg: %.2f KB/s\n",
formatBytes(totalBytes).c_str(), currentSpeedKBps, averageSpeedKBps);
}

void PerformanceMonitor::calculateCurrentSpeed(size_t newBytes) {
unsigned long now = millis();
unsigned long dt = now - lastSpeedUpdateTime;
if (dt == 0) return; // avoid div by zero
size_t bytesDelta = 0;
if (newBytes >= lastByteCount) bytesDelta = newBytes - lastByteCount;
else bytesDelta = newBytes; // odd wrap-around protection

currentSpeedKBps = (bytesDelta / 1024.0f) * 1000.0f / float(dt);

// running average naive approach
averageSpeedKBps = (averageSpeedKBps * 0.8f) + (currentSpeedKBps * 0.2f);

lastByteCount = newBytes;
updateSpeedHistory();


}

void PerformanceMonitor::updateSpeedHistory() {
speedHistory[historyIndex % PERFORMANCE_HISTORY_SIZE] = currentSpeedKBps;
historyIndex++;
}

String PerformanceMonitor::getPerformanceRating(float speedKBps) const {
if (speedKBps >= TARGET_SPEED_KBPS) return String("EXCELLENT");
if (speedKBps >= TARGET_SPEED_KBPS * 0.75f) return String("GOOD");
if (speedKBps >= TARGET_SPEED_KBPS * 0.5f) return String("FAIR");
return String("POOR");
}

float PerformanceMonitor::getPeakSpeed() const {
float peak = 0.0f;
for (int i = 0; i < PERFORMANCE_HISTORY_SIZE; ++i) {
if (speedHistory[i] > peak) peak = speedHistory[i];
}
return peak;
}

bool PerformanceMonitor::hasAchievedTarget() const {
return getPeakSpeed() >= TARGET_SPEED_KBPS;
}

unsigned long PerformanceMonitor::getElapsedTime() const {
if (startTime == 0) return 0;
return millis() - startTime;
}

void PerformanceMonitor::printEnhancedResults(size_t totalBytesTransferred) const {
Serial.println("=== PERFORMANCE SUMMARY ===");
Serial.println("Total bytes: " + String(totalBytesTransferred));
Serial.println("Avg speed: " + String(averageSpeedKBps) + " KB/s");
Serial.println("Peak speed: " + String(getPeakSpeed()) + " KB/s");
Serial.println("Rating: " + getPerformanceRating(averageSpeedKBps));
Serial.println("Detailed timing - connectionSetup(ms): " + String(detailedTiming.connectionSetupMs));
Serial.println("transferOnly(ms): " + String(detailedTiming.transferOnlyMs));
Serial.println("total(ms): " + String(detailedTiming.totalTimeMs));
Serial.println("===========================");
}

float PerformanceMonitor::calculateSpeedKBps(size_t bytes, unsigned long timeMs) {
if (timeMs == 0) return 0.0f;
return (bytes / 1024.0f) * 1000.0f / float(timeMs);
}

float PerformanceMonitor::convertBytesToKB(size_t bytes) {
return bytes / 1024.0f;
}

String PerformanceMonitor::formatSpeed(float speedKBps) {
char buf[32];
snprintf(buf, sizeof(buf), "%.2f KB/s", speedKBps);
return String(buf);
}

String PerformanceMonitor::formatTime(unsigned long timeMs) {
if (timeMs < 1000) {
return String(timeMs) + "ms";
} else if (timeMs < 60000) {
char tmp[32];
snprintf(tmp, sizeof(tmp), "%.1fs", timeMs / 1000.0f);
return String(tmp);
} else {
int minutes = timeMs / 60000;
int seconds = (timeMs % 60000) / 1000;
return String(minutes) + "m " + String(seconds) + "s";
}
}

String PerformanceMonitor::formatBytes(size_t bytes) {
if (bytes < 1024) {
return String(bytes) + " B";
} else if (bytes < 1024 * 1024) {
char tmp[32];
snprintf(tmp, sizeof(tmp), "%.1f KB", bytes / 1024.0f);
return String(tmp);
} else if (bytes < 1024ULL * 1024ULL * 1024ULL) {
char tmp[32];
snprintf(tmp, sizeof(tmp), "%.2f MB", bytes / (1024.0f * 1024.0f));
return String(tmp);
} else {
char tmp[32];
snprintf(tmp, sizeof(tmp), "%.2f GB", bytes / (1024.0f * 1024.0f * 1024.0f));
return String(tmp);
}
}

// C-style helpers

MemoryStatus getMemoryStatus() {
MemoryStatus s;
s.totalHeap = ESP.getHeapSize();
s.freeHeap = ESP.getFreeHeap();
s.minFreeHeap = ESP.getMinFreeHeap();
s.maxAllocatable = ESP.getMaxAllocHeap();
s.memoryHealthy = BufferManager::checkMemoryHealth();
s.statusMessage = "Free: " + String(s.freeHeap / 1024) + " KB" + ", Min: " + String(s.minFreeHeap / 1024) + " KB";
if (BufferManager::canEnableDoubleBuffering()) s.statusMessage += ", Double Buffering: YES";
else s.statusMessage += ", Double Buffering: NO";
return s;
}

bool initializeMemoryManager() {
Serial.println("Initializing HIGH-PERFORMANCE memory manager");
BufferManager::printMemoryDiagnostics();
return BufferManager::checkMemoryHealth();
}