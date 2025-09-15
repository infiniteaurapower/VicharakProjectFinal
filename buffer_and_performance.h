#pragma once
#include <Arduino.h>


// Constants for buffer sizing (kept same numeric values)
const size_t SMALL_DOWNLOAD_BUFFER_SIZE = 32768;
const size_t DEFAULT_DOWNLOAD_BUFFER_SIZE = 65536;
const size_t LARGE_DOWNLOAD_BUFFER_SIZE = 131072;
const size_t XLARGE_DOWNLOAD_BUFFER_SIZE = 262144;

const size_t SMALL_WRITE_BUFFER_SIZE = 16384;
const size_t DEFAULT_WRITE_BUFFER_SIZE = 32768;
const size_t LARGE_WRITE_BUFFER_SIZE = 65536;

const size_t DOUBLE_BUFFER_COUNT = 2;

// Memory bookkeeping thresholds
const size_t MIN_FREE_HEAP_REQUIRED = 80000;
const float HEAP_SAFETY_MARGIN = 0.15f;

// Performance monitor tuning
const float TARGET_SPEED_KBPS = 400.0f;
const int SPEED_UPDATE_INTERVAL_MS = 500;
const int PROGRESS_UPDATE_INTERVAL_MS = 1000;
const int PERFORMANCE_HISTORY_SIZE = 20;

// A small structure to summarize heap status
struct MemoryStatus {
size_t totalHeap;
size_t freeHeap;
size_t minFreeHeap;
size_t maxAllocatable;
bool memoryHealthy;
String statusMessage;
MemoryStatus() : totalHeap(0), freeHeap(0), minFreeHeap(0), maxAllocatable(0), memoryHealthy(false), statusMessage("") {}
};

// Buffer manager: two download/write buffers to support double buffering
class BufferManager {
private:
uint8_t* downloadBuffers[DOUBLE_BUFFER_COUNT];
uint8_t* writeBuffers[DOUBLE_BUFFER_COUNT];
size_t downloadBufferSize;
size_t writeBufferSize;
int activeDownloadBuffer;
int activeWriteBuffer;
bool buffersAllocated;
bool doubleBufferingEnabled;

public:
BufferManager();
~BufferManager();

// High-level allocation helpers
bool allocateBuffers();                  // default smart allocation
bool allocateSmartScalingBuffers();      // uses heap probing
bool allocateBuffers(size_t downloadSize, size_t writeSize); // explicit

void deallocateBuffers();

// Accessors
uint8_t* getActiveDownloadBuffer() const { return downloadBuffers[activeDownloadBuffer]; }
uint8_t* getActiveWriteBuffer() const { return writeBuffers[activeWriteBuffer]; }

// Optionally request a specific index, default = active
uint8_t* getDownloadBuffer(int index = -1) const;
uint8_t* getWriteBuffer(int index = -1) const;

size_t getDownloadBufferSize() const { return downloadBufferSize; }
size_t getWriteBufferSize() const { return writeBufferSize; }

void swapDownloadBuffers();
void swapWriteBuffers();
bool isDoubleBufferingEnabled() const { return doubleBufferingEnabled; }

bool hasEnoughMemory(size_t requiredBytes) const;
void printMemoryStatus() const;
bool validateBuffers() const;

// Static helpers that probe the heap
static size_t getAvailableHeap();
static size_t getSmartDownloadBufferSize();
static size_t getSmartWriteBufferSize();
static bool checkMemoryHealth();
static bool canEnableDoubleBuffering();
static void printMemoryDiagnostics();


};

// Timing details for connection + transfer phases
struct DetailedTiming {
unsigned long connectionSetupMs;
unsigned long firstByteMs;
unsigned long transferOnlyMs;
unsigned long totalTimeMs;
DetailedTiming() : connectionSetupMs(0), firstByteMs(0), transferOnlyMs(0), totalTimeMs(0) {}

float getPureTransferSpeedKBps(size_t bytes) const {
    if (transferOnlyMs > 0) {
        return (bytes / 1024.0f) * 1000.0f / transferOnlyMs;
    }
    return 0.0f;
}

float getOverallSpeedKBps(size_t bytes) const {
    if (totalTimeMs > 0) {
        return (bytes / 1024.0f) * 1000.0f / totalTimeMs;
    }
    return 0.0f;
}

float getEfficiencyPercent() const {
    if (totalTimeMs > 0) {
        return (float(transferOnlyMs) / float(totalTimeMs)) * 100.0f;
    }
    return 0.0f;
}


};

// Simple performance monitor: measures speeds, history and timings
class PerformanceMonitor {
private:
unsigned long startTime;
unsigned long lastUpdateTime;
unsigned long lastSpeedUpdateTime;
size_t totalBytes;
size_t lastByteCount;
float currentSpeedKBps;
float averageSpeedKBps;
float speedHistory[PERFORMANCE_HISTORY_SIZE];
int historyIndex;
bool isActive;

// For enhanced timing details
unsigned long connectionStartTime;
unsigned long firstByteTime;
unsigned long transferStartTime;
bool firstByteReceived;
DetailedTiming detailedTiming;

// internal helpers
void calculateCurrentSpeed(size_t newBytes);
void updateSpeedHistory();
void resetMonitoring();
String getPerformanceRating(float speedKBps) const;


public:
PerformanceMonitor();
~PerformanceMonitor();

void startMonitoring();
void stopMonitoring();
bool isMonitoring() const { return isActive; }

void startConnectionTimer();
void markFirstByte();
void stopEnhancedMonitoring();

// progress updates: either absolute transferred bytes or current+total
void updateProgress(size_t bytesTransferred);
void updateProgress(size_t current, size_t total);
void printProgress() const;

float getCurrentSpeed() const { return currentSpeedKBps; }
float getAverageSpeed() const { return averageSpeedKBps; }
float getPeakSpeed() const;
bool hasAchievedTarget() const;
unsigned long getElapsedTime() const;
DetailedTiming getDetailedTiming() const { return detailedTiming; }

void printEnhancedResults(size_t totalBytesTransferred) const;

// static formatting utilities
static float calculateSpeedKBps(size_t bytes, unsigned long timeMs);
static float convertBytesToKB(size_t bytes);
static String formatSpeed(float speedKBps);
static String formatTime(unsigned long timeMs);
static String formatBytes(size_t bytes);


};

// Some small helper / C-style API used in network code
MemoryStatus getMemoryStatus();
bool initializeMemoryManager();

struct DownloadResult {
bool success = false;
size_t fileSize = 0;
size_t totalBytes = 0;
unsigned long downloadTimeMs = 0;
float averageSpeedKBps = 0.0f;
float peakSpeedKBps = 0.0f;
String errorMessage = "";
int httpStatusCode = 0;
bool targetAchieved = false;

// Enhanced metrics
float pureTransferSpeedKBps = 0.0f;
float transferEfficiencyPercent = 0.0f;
unsigned long connectionSetupMs = 0;
unsigned long transferOnlyMs = 0;
unsigned long connectionTimeMs = 0;


};

struct PerformanceResults {
float averageSpeedKBps;
float peakSpeedKBps;
unsigned long totalTimeMs;
bool targetAchieved;
DetailedTiming timing;
};