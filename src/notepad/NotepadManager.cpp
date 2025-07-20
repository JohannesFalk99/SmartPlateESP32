#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "notepad/NotepadManager.h"

// Singleton instance getter
NotepadManager &NotepadManager::getInstance()
{
    static NotepadManager instance;
    return instance;
}

NotepadManager::NotepadManager()
{
    // Ensure LittleFS is mounted (should already be done, but safe)
    if (!LittleFS.begin(true))
    {
        Serial.println(F("[NotepadManager] Failed to mount LittleFS"));
    }
}

// List all saved note filenames (without extension) into the JSON array "experiments"
NotepadManager::Status NotepadManager::listNotes(JsonArray& arr) const
{
    Serial.println(F("[NotepadManager] listNotes() called"));

    File root = LittleFS.open("/");
    if (!root || !root.isDirectory())
    {
        Serial.println(F("[NotepadManager] Failed to open or invalid root directory"));
        return Status::FILESYSTEM_ERROR;
    }

    Serial.println(F("[NotepadManager] Opened root directory, scanning files..."));

    int fileCount = 0;
    int noteCount = 0;

    File file = root.openNextFile();
    while (file)
    {
        fileCount++;

        if (file.isDirectory())
        {
            Serial.printf("[NotepadManager] Skipping directory: %s\n", file.name());
        }
        else
        {
            String name = file.name();
            Serial.printf("[NotepadManager] Found file: %s\n", name.c_str());

            if (name.endsWith(".json"))
            {
                // Strip leading '/' and '.json' extension
                String expName = name.substring(0, name.length() - 5);
                arr.add(expName);
                noteCount++;
                Serial.printf("[NotepadManager] Added note: %s\n", expName.c_str());
            }
            else
            {
                Serial.printf("[NotepadManager] Skipped non-json file: %s\n", name.c_str());
            }
        }

        file = root.openNextFile();
    }

    Serial.printf("[NotepadManager] Scan complete. Total files: %d, Notes found: %d\n", fileCount, noteCount);
    return Status::SUCCESS;
}

// Load a note by experiment name into the provided string, returns Status indicating success or failure
NotepadManager::Status NotepadManager::loadNote(const String &experiment, String &outNotes) const
{
    String path = "/" + experiment + ".json";
    Serial.printf("[NotepadManager] Attempting to load note from path: %s\n", path.c_str());

    if (!LittleFS.exists(path))
    {
        Serial.printf("[NotepadManager] Note file does NOT exist: %s\n", path.c_str());
        return Status::FILE_NOT_FOUND;
    }

    File file = LittleFS.open(path, "r");
    if (!file)
    {
        Serial.printf("[NotepadManager] Failed to open note file: %s\n", path.c_str());
        return Status::FILE_OPEN_ERROR;
    }

    Serial.printf("[NotepadManager] Successfully opened note file: %s\n", path.c_str());

    outNotes = file.readString();
    file.close();

    Serial.printf("[NotepadManager] Read %u bytes from file %s\n", outNotes.length(), path.c_str());

    if (outNotes.length() == 0)
    {
        Serial.printf("[NotepadManager] Warning: note file %s is empty!\n", path.c_str());
        return Status::FILE_READ_ERROR;
    }

    return Status::SUCCESS;
}

// Save a note for the given experiment name
NotepadManager::Status NotepadManager::saveNote(const String &experiment, const String &notes)
{
    if (experiment.isEmpty() || notes.length() > MAX_NOTE_SIZE) {
        return Status::INVALID_INPUT;
    }
    
    if (!ensureFilesystem()) {
        return Status::FILESYSTEM_ERROR;
    }

    String path = String(BASE_PATH) + sanitizeFilename(experiment) + NOTE_EXTENSION;
    if (path.isEmpty()) {
        return Status::INVALID_INPUT;
    }

    File file = LittleFS.open(path, "w");
    if (!file) {
        Serial.printf("[NotepadManager] Failed to open note file %s for writing\n", path.c_str());
        return Status::FILE_OPEN_ERROR;
    }

    size_t written = file.print(notes);
    file.close();

    if (written != notes.length()) {
        Serial.printf("[NotepadManager] Failed to write complete note to %s (wrote %d/%d bytes)\n", 
                     path.c_str(), written, notes.length());
        return Status::FILE_WRITE_ERROR;
    }

    Serial.printf("[NotepadManager] Saved note %s (%u bytes)\n", path.c_str(), (unsigned)written);
    return Status::SUCCESS;
}
