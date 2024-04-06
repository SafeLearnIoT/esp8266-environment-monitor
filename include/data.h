#ifndef DATA_H
#define DATA_H
#include <Arduino.h>
#include "FS.h"
#include "communication.h"
#include <LittleFS.h>

#define FORMAT_LITTLEFS_IF_FAILED true

void listDir(fs::FS &fs, const char *dirname);

void createDir(fs::FS &fs, const char *path);

void removeDir(fs::FS &fs, const char *path);

String readFile(fs::FS &fs, const char *path);

void writeFile(fs::FS &fs, const char *path, const char *message);

void appendFile(fs::FS &fs, const char *path, const char *message);

void renameFile(fs::FS &fs, const char *path1, const char *path2);

void deleteFile(fs::FS &fs, const char *path);

void checkAndCleanFileSystem(fs::FS &fs);

int getFileSize(fs::FS &fs, const char *path);
#endif