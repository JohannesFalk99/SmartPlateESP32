/**
 * @file Explorer.h
 * @brief File system explorer for handling file operations via web interface
 * 
 * This module provides a RESTful API for managing files on the device's filesystem,
 * including listing, uploading, downloading, and deleting files.
 */

#ifndef EXPLORER_H
#define EXPLORER_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>
#include <string>
#include <memory>

// Maximum allowed file size for uploads (10MB)
constexpr size_t MAX_UPLOAD_SIZE = 10 * 1024 * 1024;

// Allowed file extensions for upload
const std::vector<std::string> ALLOWED_EXTENSIONS = {
    ".txt", ".json", ".csv", ".bin", ".gcode"
};

/**
 * @class FileSystemExplorer
 * @brief Handles file system operations via a web interface
 */
class FileSystemExplorer {
public:
    /**
     * @brief Construct a new File System Explorer
     * @param server Reference to the AsyncWebServer instance
     */
    explicit FileSystemExplorer(AsyncWebServer &server);
    
    // Prevent copying and assignment
    FileSystemExplorer(const FileSystemExplorer&) = delete;
    FileSystemExplorer& operator=(const FileSystemExplorer&) = delete;
    
    /**
     * @brief Initialize the file system explorer and register routes
     * @return true if initialization was successful, false otherwise
     */
    bool begin();

private:
    AsyncWebServer &server_;
    bool fsMounted_{false};
    
    // Route handlers
    void handleList(AsyncWebServerRequest *request);
    void handleDelete(AsyncWebServerRequest *request);
    void handleDownload(AsyncWebServerRequest *request);
    void handleUpload(AsyncWebServerRequest *request);
    void onUpload(AsyncWebServerRequest *request, String filename, size_t index, 
                 uint8_t *data, size_t len, bool final);
    
    // Helper methods
    bool validatePath(const String &path, String &error) const;
    bool isPathSafe(const String &path) const;
    bool isAllowedExtension(const String &filename) const;
    String getMimeType(const String &filename) const;
    void listDir(const String &path, JsonArray &array) const;
    
    // Upload state
    struct UploadState {
        File file;
        size_t totalSize{0};
        String filename;
        bool isValid{false};
    };
    
    // Current upload state (only one upload at a time)
    std::unique_ptr<UploadState> uploadState_;
};

#endif // EXPLORER_H
