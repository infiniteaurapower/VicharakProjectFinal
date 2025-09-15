#pragma once
#include <Arduino.h>

// SPIFFS helper functions — small handwritten-style docstrings and a FileInfo struct

bool startSPIFFS();
bool mountSPIFFS();
bool saveToSPIFFS(const String& path, const String& data);
void printFile(const String& path);
void readAndPrintFile(const String& path);
void listSPIFFSFiles();
bool getSPIFFSInfo(size_t& totalBytes, size_t& usedBytes);
bool checkSPIFFSSpace(size_t requiredBytes);
bool deleteSPIFFSFile(const String& path);
void formatSPIFFS();

// FileInfo: tiny struct used by list/indexing helpers
struct FileInfo {
String name;
size_t size;
bool isDirectory;
FileInfo() : name(""), size(0), isDirectory(false) {}
FileInfo(String n, size_t s, bool d) : name(n), size(s), isDirectory(d) {}
};