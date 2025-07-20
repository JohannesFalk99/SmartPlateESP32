/**
 * @file Explorer.cpp
 * @brief Implementation of the FileSystemExplorer class
 */

#include "explorer/Explorer.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <algorithm>

// MIME type mapping
static const std::pair<const char*, const char*> MIME_TYPES[] = {
    {".txt", "text/plain"},
    {".csv", "text/csv"},
    {".json", "application/json"},
    {".gcode", "text/x-gcode"},
    {".bin", "application/octet-stream"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".ico", "image/x-icon"}
};

FileSystemExplorer::FileSystemExplorer(AsyncWebServer &server) 
    : server_(server) {
}

bool FileSystemExplorer::begin() {
    // Mount the file system
    fsMounted_ = LittleFS.begin();
    if (!fsMounted_) {
        Serial.println("[Explorer] Failed to mount LittleFS");
        return false;
    }

    // Register routes
    server_.on("/api/fs/list", HTTP_GET, 
        [this](AsyncWebServerRequest *request) { handleList(request); });
        
    server_.on("/api/fs/delete", HTTP_DELETE, 
        [this](AsyncWebServerRequest *request) { handleDelete(request); });
        
    server_.on("/api/fs/download", HTTP_GET, 
        [this](AsyncWebServerRequest *request) { handleDownload(request); });
    
    // Handle file uploads
    auto uploadHandler = [this](AsyncWebServerRequest *request, 
                              const String& filename, 
                              size_t index, 
                              uint8_t *data, 
                              size_t len, 
                              bool final) {
        onUpload(request, filename, index, data, len, final);
    };
    
    server_.on("/api/fs/upload", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            // Called after upload completes
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        },
        uploadHandler);
    
    Serial.println("[Explorer] File system explorer initialized");
    return true;
}

void FileSystemExplorer::handleList(AsyncWebServerRequest *request) {
    String dir = "/";
    if (request->hasParam("dir")) {
        dir = request->getParam("dir")->value();
    }
    
    // Validate the path
    String error;
    if (!validatePath(dir, error)) {
        request->send(400, "application/json", "{\"error\":\"" + error + "\"}");
        return;
    }

    DynamicJsonDocument doc(4096);
    JsonArray arr = doc.to<JsonArray>();
    
    // List directory contents
    File root = LittleFS.open(dir);
    if (!root || !root.isDirectory()) {
        request->send(404, "application/json", "{\"error\":\"Directory not found\"}");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        JsonObject obj = arr.createNestedObject();
        String name = file.name();
        
        // Get relative path for display
        if (name.startsWith(dir)) {
            name = name.substring(dir.length());
        }
        
        // Skip hidden files
        if (name.length() > 0 && name[0] == '.') {
            file = root.openNextFile();
            continue;
        }
        
        obj["name"] = name;
        obj["path"] = file.name();
        obj["size"] = file.size();
        obj["isDir"] = file.isDirectory();
        obj["mime"] = file.isDirectory() ? "inode/directory" : getMimeType(name);
        
        file = root.openNextFile();
    }
    
    // Send response
    String output;
    serializeJson(arr, output);
    request->send(200, "application/json", output);
}

void FileSystemExplorer::handleDelete(AsyncWebServerRequest *request) {
    if (!request->hasParam("path")) {
        request->send(400, "application/json", "{\"error\":\"Missing path parameter\"}");
        return;
    }

    String path = request->getParam("path")->value();
    String error;
    
    // Validate path
    if (!validatePath(path, error)) {
        request->send(400, "application/json", "{\"error\":\"" + error + "\"}");
        return;
    }

    // Prevent deleting root directory
    if (path == "/") {
        request->send(403, "application/json", "{\"error\":\"Cannot delete root directory\"}");
        return;
    }

    if (!LittleFS.exists(path)) {
        request->send(404, "application/json", "{\"error\":\"File not found\"}");
        return;
    }

    // Check if it's a directory
    File file = LittleFS.open(path);
    if (file.isDirectory()) {
        // TODO: Recursively delete directory contents
        request->send(501, "application/json", "{\"error\":\"Directory deletion not implemented\"}");
        return;
    }
    file.close();

    // Delete the file
    if (LittleFS.remove(path)) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to delete file\"}");
    }
}

void FileSystemExplorer::handleDownload(AsyncWebServerRequest *request) {
    if (!request->hasParam("path")) {
        request->send(400, "application/json", "{\"error\":\"Missing path parameter\"}");
        return;
    }

    String path = request->getParam("path")->value();
    String error;
    
    // Validate path
    if (!validatePath(path, error)) {
        request->send(400, "application/json", "{\"error\":\"" + error + "\"}");
        return;
    }

    if (!LittleFS.exists(path)) {
        request->send(404, "application/json", "{\"error\":\"File not found\"}");
        return;
    }

    File file = LittleFS.open(path, "r");
    if (!file) {
        request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
        return;
    }

    // Get file size for content length
    size_t fileSize = file.size();
    
    // Get filename for content disposition
    String filename = path;
    int lastSlash = path.lastIndexOf('/');
    if (lastSlash != -1) {
        filename = path.substring(lastSlash + 1);
    }
    
    // Create response with file stream
    AsyncWebServerResponse *response = request->beginResponse(
        getMimeType(filename), 
        fileSize,
        [file](uint8_t *buffer, size_t maxLen, size_t index) mutable -> size_t {
            if (!file) return 0;
            
            // Calculate remaining bytes
            size_t remaining = file.available();
            if (remaining == 0) {
                file.close();
                return 0;
            }
            
            // Read up to maxLen bytes
            size_t toRead = min(remaining, maxLen);
            return file.read(buffer, toRead);
        }
    );
    
    // Set content disposition for download
    response->addHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    request->send(response);
}

void FileSystemExplorer::handleUpload(AsyncWebServerRequest *request) {
    // This is a no-op as we handle uploads in onUpload
    request->send(400, "application/json", "{\"error\":\"Invalid upload request\"}");
}

void FileSystemExplorer::onUpload(AsyncWebServerRequest *request, String filename, 
                                 size_t index, uint8_t *data, size_t len, bool final) {
    // Initialize upload state if this is the first chunk
    if (index == 0) {
        uploadState_ = std::make_unique<UploadState>();
        uploadState_->filename = "/" + filename; // Store with leading slash
        uploadState_->isValid = true;
        
        // Validate filename and extension
        String error;
        if (!validatePath(uploadState_->filename, error) || !isAllowedExtension(filename)) {
            uploadState_->isValid = false;
            request->send(400, "application/json", "{\"error\":\"Invalid file type or path\"}");
            return;
        }
        
        // Delete existing file if it exists
        if (LittleFS.exists(uploadState_->filename)) {
            LittleFS.remove(uploadState_->filename);
        }
        
        // Open file for writing
        uploadState_->file = LittleFS.open(uploadState_->filename, "w");
        if (!uploadState_->file) {
            uploadState_->isValid = false;
            request->send(500, "application/json", "{\"error\":\"Failed to create file\"}");
            return;
        }
        
        Serial.printf("[Explorer] Starting upload: %s\n", uploadState_->filename.c_str());
    }
    
    // Skip if upload state is invalid
    if (!uploadState_ || !uploadState_->isValid) {
        return;
    }
    
    // Check file size limit
    uploadState_->totalSize += len;
    if (uploadState_->totalSize > MAX_UPLOAD_SIZE) {
        uploadState_->file.close();
        LittleFS.remove(uploadState_->filename);
        uploadState_->isValid = false;
        request->send(413, "application/json", "{\"error\":\"File too large\"}");
        return;
    }
    
    // Write data to file
    if (len > 0 && uploadState_->file) {
        uploadState_->file.write(data, len);
    }
    
    // Finalize upload
    if (final) {
        if (uploadState_->file) {
            uploadState_->file.close();
            Serial.printf("[Explorer] Upload complete: %s (%d bytes)\n", 
                        uploadState_->filename.c_str(), 
                        uploadState_->totalSize);
        }
        
        // Clear upload state
        uploadState_.reset();
    }
}

bool FileSystemExplorer::validatePath(const String &path, String &error) const {
    // Check for path traversal attempts
    if (path.indexOf("..") != -1 || 
        path.indexOf("\\") != -1 || 
        path.indexOf("//") != -1) {
        error = "Invalid path";
        return false;
    }
    
    // Ensure path starts with /
    if (!path.startsWith("/")) {
        error = "Path must be absolute";
        return false;
    }
    
    // Additional path validation can be added here
    
    return true;
}

bool FileSystemExplorer::isPathSafe(const String &path) const {
    // Check for forbidden patterns
    if (path.indexOf("..") != -1 || 
        path.indexOf("\\") != -1 || 
        path.indexOf("//") != -1) {
        return false;
    }
    
    // Ensure path starts with /
    if (!path.startsWith("/")) {
        return false;
    }
    
    return true;
}

bool FileSystemExplorer::isAllowedExtension(const String &filename) const {
    // Allow directories
    if (filename.endsWith("/")) {
        return true;
    }
    
    // Check against allowed extensions
    for (const auto &ext : ALLOWED_EXTENSIONS) {
        if (filename.endsWith(ext.c_str())) {
            return true;
        }
    }
    
    return false;
}

String FileSystemExplorer::getMimeType(const String &filename) const {
    // Default MIME type
    String mimeType = "application/octet-stream";
    
    // Find matching extension
    for (const auto &mime : MIME_TYPES) {
        if (filename.endsWith(mime.first)) {
            mimeType = mime.second;
            break;
        }
    }
    
    return mimeType;
}

void FileSystemExplorer::listDir(const String &path, JsonArray &array) const {
    File root = LittleFS.open(path);
    if (!root || !root.isDirectory()) {
        return;
    }
    
    File file = root.openNextFile();
    while (file) {
        JsonObject obj = array.createNestedObject();
        String name = file.name();
        
        // Get relative path for display
        if (name.startsWith(path)) {
            name = name.substring(path.length());
        }
        
        // Skip hidden files
        if (name.length() > 0 && name[0] == '.') {
            file = root.openNextFile();
            continue;
        }
        
        obj["name"] = name;
        obj["path"] = file.name();
        obj["size"] = file.size();
        obj["isDir"] = file.isDirectory();
        obj["mime"] = file.isDirectory() ? "inode/directory" : getMimeType(name);
        
        file = root.openNextFile();
    }
}
