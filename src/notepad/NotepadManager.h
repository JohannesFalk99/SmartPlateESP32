#pragma once

#include <ArduinoJson.h>
#include <Arduino.h>
#include <memory>

/**
 * @class NotepadManager
 * @brief Manages storage and retrieval of text notes in the filesystem
 * 
 * This class provides a thread-safe singleton interface for managing text notes
 * stored in the LittleFS filesystem. Notes are stored as JSON files with the
 * naming convention: "/{experiment}.json"
 */
class NotepadManager {
public:
    /**
     * @brief Status codes for note operations
     */
    enum class Status {
        SUCCESS,                ///< Operation completed successfully
        FILE_OPEN_ERROR,        ///< Failed to open file
        FILE_READ_ERROR,        ///< Failed to read file
        FILE_WRITE_ERROR,       ///< Failed to write to file
        FILE_NOT_FOUND,         ///< Requested file does not exist
        INVALID_INPUT,          ///< Invalid input parameters
        FILESYSTEM_ERROR,       ///< Filesystem operation failed
        UNKNOWN_ERROR           ///< An unexpected error occurred
    };

    /**
     * @brief Get the singleton instance of NotepadManager
     * @return Reference to the NotepadManager instance
     */
    static NotepadManager& getInstance();

    /**
     * @brief List all available notes
     * @param[out] arr JsonArray to populate with note names
     * @return Status indicating success or failure
     */
    Status listNotes(JsonArray& arr) const;

    /**
     * @brief Load the contents of a note
     * @param[in] experiment Name of the experiment/note to load
     * @param[out] outNotes String to store the note contents
     * @return Status indicating success or failure
     */
    Status loadNote(const String& experiment, String& outNotes) const;

    /**
     * @brief Save a note
     * @param[in] experiment Name of the experiment/note to save
     * @param[in] notes The note contents to save
     * @return Status indicating success or failure
     */
    Status saveNote(const String& experiment, const String& notes);

    // Delete copy and move constructors and assignment operators
    NotepadManager(const NotepadManager&) = delete;
    NotepadManager& operator=(const NotepadManager&) = delete;
    NotepadManager(NotepadManager&&) = delete;
    NotepadManager& operator=(NotepadManager&&) = delete;

private:
    /**
     * @brief Private constructor for singleton pattern
     */
    NotepadManager();

    /**
     * @brief Ensure the filesystem is properly initialized
     * @return true if filesystem is ready, false otherwise
     */
    bool ensureFilesystem() const;

    /**
     * @brief Sanitize a filename to prevent directory traversal
     * @param filename Input filename to sanitize
     * @return Sanitized filename or empty string if invalid
     */
    String sanitizeFilename(const String& filename) const;

    bool filesystemInitialized_{false};  ///< Track filesystem initialization state
    static constexpr size_t MAX_NOTE_SIZE = 1024 * 50;  ///< 50KB max note size
    static constexpr const char* NOTE_EXTENSION = ".json";
    static constexpr const char* BASE_PATH = "/notes/";  ///< Base directory for notes
};
