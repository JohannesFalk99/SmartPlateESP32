// FileSystemExplorer.cpp
// This file implements file system operations for the ESP32.

#include "utilities/FileSystemExplorer.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config/Config.h"
#include "managers/HeaterModeManager.h" // For LogLevel
#include "utilities/SerialRemote.h"

// Constructor for FileSystemExplorer class
FileSystemExplorer::FileSystemExplorer(AsyncWebServer &srv) : server(srv) {}

// Initialize the file system and set up server routes
void FileSystemExplorer::begin()
{
    if (!LittleFS.begin())
    {
        logMessagef(LogLevel::ERROR, "LittleFS Mount Failed");
        return;
    }

    // Define server routes for file system operations
    server.on("/fs/list", HTTP_GET, std::bind(&FileSystemExplorer::handleList, this, std::placeholders::_1));
    server.on("/fs/delete", HTTP_GET, std::bind(&FileSystemExplorer::handleDelete, this, std::placeholders::_1));
    server.on("/fs/download", HTTP_GET, std::bind(&FileSystemExplorer::handleDownload, this, std::placeholders::_1));

    server.on("/fs/upload", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                  // Send response when upload is done
                  request->send(200, "text/plain", "Upload complete"); }, std::bind(&FileSystemExplorer::onUpload, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
}

// Handle listing files in a directory
void FileSystemExplorer::handleList(AsyncWebServerRequest *request)
{
    String dir = "/";
    if (request->hasParam("dir"))
    {
        dir = request->getParam("dir")->value();
        if (!dir.startsWith("/"))
            dir = "/" + dir;
    }

    DynamicJsonDocument doc(4096);
    JsonArray arr = doc.to<JsonArray>();

    File root = LittleFS.open(dir);
    if (!root || !root.isDirectory())
    {
        request->send(400, "text/plain", "Invalid directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        JsonObject obj = arr.createNestedObject();

        String name = file.name();
        if (name.startsWith(dir))
            name = name.substring(dir.length()); // relative name for display

        obj["name"] = name;        // short name shown in UI
        obj["path"] = file.name(); // full path sent for operations (download, delete)
        obj["size"] = file.size();
        obj["isDir"] = file.isDirectory();

        file = root.openNextFile();
    }

    String out;
    serializeJson(arr, out);
    request->send(200, "application/json", out);
}

// Handle deleting a file
void FileSystemExplorer::handleDelete(AsyncWebServerRequest *request)
{
    if (!request->hasParam("path"))
    {
        request->send(400, "text/plain", "Missing path parameter");
        return;
    }

    String path = request->getParam("path")->value();
    if (path.length() == 0 || path[0] != '/')
        path = "/" + path;

    if (!LittleFS.exists(path))
    {
        request->send(404, "text/plain", "File not found");
        return;
    }

    bool success = LittleFS.remove(path);

    if (success)
        request->send(200, "text/plain", "Deleted");
    else
        request->send(500, "text/plain", "Failed to delete");
}

// Handle downloading a file
void FileSystemExplorer::handleDownload(AsyncWebServerRequest *request)
{
    if (!request->hasParam("path"))
    {
        request->send(400, "text/plain", "Missing path parameter");
        return;
    }

    String path = request->getParam("path")->value();
    if (path.length() == 0 || path[0] != '/')
        path = "/" + path;

    if (!LittleFS.exists(path))
    {
        request->send(404, "text/plain", "File not found");
        return;
    }

    File file = LittleFS.open(path, "r");
    if (!file)
    {
        request->send(500, "text/plain", "Failed to open file");
        return;
    }

    // Use beginResponse with size specified:
    AsyncWebServerResponse *response = request->beginResponse("application/octet-stream", file.size(), [file](uint8_t *buffer, size_t maxLen, size_t index) mutable -> size_t
                                                              {
        if (!file)
            return 0;
        size_t bytesRead = file.read(buffer, maxLen);
        if (bytesRead == 0)
        {
            file.close();
        }
        return bytesRead; });

    response->addHeader("Content-Disposition", "attachment; filename=\"" + path.substring(path.lastIndexOf('/') + 1) + "\"");
    request->send(response);
}

// Handle file upload (not used, actual upload handled in onUpload)
void FileSystemExplorer::handleUpload(AsyncWebServerRequest *request)
{
}

// Handle the upload process
void FileSystemExplorer::onUpload(AsyncWebServerRequest *request, String filename, size_t index,
                                  uint8_t *data, size_t len, bool final)
{
    String path = "/" + filename;
    static File uploadFile;

    if (index == 0)
    {
        if (LittleFS.exists(path))
            LittleFS.remove(path);
        uploadFile = LittleFS.open(path, "w");
    }

    if (uploadFile)
    {
        uploadFile.write(data, len);
    }

    if (final)
    {
        uploadFile.close();
    }
}
