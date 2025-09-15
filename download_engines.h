#pragma once
#include <Arduino.h>
#include "buffer_and_performance.h"

// Abstract downloader base - humanized style
class DownloaderBase {
public:
DownloaderBase() {}
virtual ~DownloaderBase() {}

// Start the download. Returns DownloadResult with metrics & status.
virtual DownloadResult download(const String& url, const String& targetPath) = 0;

// Some implementations may expose this
virtual void cancel() { /* optional */ }

// Basic helpers
virtual String getName() const = 0;


};

// A simple HTTP fetcher - header declarations
class HttpDownloader : public DownloaderBase {
public:
HttpDownloader();
~HttpDownloader() override;

DownloadResult download(const String& url, const String& targetPath) override;
void cancel() override;
String getName() const override { return String("HttpDownloader"); }

// Exposed tuning - users may tweak if they need to
void setBufferManager(BufferManager* mgr) { bufMgr = mgr; }
void setPerformanceMonitor(PerformanceMonitor* m) { perf = m; }


private:
bool cancelled;
BufferManager* bufMgr;
PerformanceMonitor* perf;

// small helper to actually write downloaded bytes to a file
bool writeChunkToFile(const String& path, const uint8_t* data, size_t len, bool append);

// attempted small retries for flaky networks — intentionally simple
int maxRetries;


};

// A minimal resume-capable downloader (humanized, not over-engineered)
class ResumeDownloader : public HttpDownloader {
public:
ResumeDownloader();
~ResumeDownloader() override;
DownloadResult download(const String& url, const String& targetPath) override;
String getName() const override { return String("ResumeDownloader"); }

private:
// internal helper to probe remote file size
long probeRemoteSize(const String& url);
// a tiny helper to append to existing file
bool appendToExistingFile(const String& path, const uint8_t* data, size_t len);
};

// Dual-core FreeRTOS downloader for high-performance parallel processing
class DualCoreDownloader : public DownloaderBase {
public:
DualCoreDownloader();
~DualCoreDownloader() override;

DownloadResult download(const String& url, const String& targetPath) override;
void cancel() override;
String getName() const override { return String("DualCoreDownloader"); }

// Configuration
void setBufferManager(BufferManager* mgr) { bufMgr = mgr; }
void setPerformanceMonitor(PerformanceMonitor* m) { perf = m; }
void setChunkSize(size_t size) { chunkSize = size; }

private:
struct DownloadTask {
    String url;
    String targetPath;
    DualCoreDownloader* downloader;
    DownloadResult* result;
    SemaphoreHandle_t completionSemaphore;
};

BufferManager* bufMgr;
PerformanceMonitor* perf;
size_t chunkSize;
bool cancelled;

// FreeRTOS task functions
static void downloadTaskCore0(void* parameter);
static void downloadTaskCore1(void* parameter);
static void coordinatorTask(void* parameter);

// Helper functions
bool performActualDownload(const String& url, const String& targetPath, DownloadResult* result, PerformanceMonitor* perfMonitor);
};