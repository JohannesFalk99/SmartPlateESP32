#ifndef EXPLORER_H
#define EXPLORER_H
#pragma once
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

/**
 * @brief Provides web-based file system exploration and management
 * 
 * This class adds REST endpoints to an AsyncWebServer for listing, uploading,
 * downloading, and deleting files from the LittleFS file system.
 */
class FileSystemExplorer
{
public:
    /**
     * @brief Construct a new File System Explorer
     * @param server Reference to the AsyncWebServer to attach endpoints to
     */
    explicit FileSystemExplorer(AsyncWebServer &server);

    /**
     * @brief Initialize the file explorer and register web server endpoints
     */
    void begin();

private:
    AsyncWebServer &server;

    /**
     * @brief Handle HTTP request to list files in a directory
     * @param request Pointer to the web server request
     */
    void handleList(AsyncWebServerRequest *request);
    
    /**
     * @brief Handle HTTP request to delete a file
     * @param request Pointer to the web server request
     */
    void handleDelete(AsyncWebServerRequest *request);
    
    /**
     * @brief Handle HTTP request to download a file
     * @param request Pointer to the web server request
     */
    void handleDownload(AsyncWebServerRequest *request);

    /**
     * @brief Handle HTTP request to upload a file (final handler)
     * @param request Pointer to the web server request
     */
    void handleUpload(AsyncWebServerRequest *request);

    /**
     * @brief Callback for file upload data chunks
     * @param request Pointer to the web server request
     * @param filename Name of the file being uploaded
     * @param index Current position in the upload stream
     * @param data Pointer to chunk data
     * @param len Length of the chunk
     * @param final True if this is the final chunk
     */
    void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

    /**
     * @brief List files and directories inside a path
     * @param path Path to list
     * @param array JsonArray to populate with file information
     */
    void listDir(const String &path, JsonArray &array);
};

#endif // EXPLORER_H
