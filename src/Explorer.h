#ifndef EXPLORER_H
#define EXPLORER_H
#pragma once
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

class FileSystemExplorer
{
public:
    explicit FileSystemExplorer(AsyncWebServer &server);

    void begin();

private:
    AsyncWebServer &server;

    void handleList(AsyncWebServerRequest *request);
    void handleDelete(AsyncWebServerRequest *request);
    void handleDownload(AsyncWebServerRequest *request);

    void handleUpload(AsyncWebServerRequest *request);

    void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

    // Helper to list files and directories inside a path
    void listDir(const String &path, JsonArray &array);
};

#endif // EXPLORER_H
