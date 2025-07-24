#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "NotepadManager.h"

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
void NotepadManager::listNotes(JsonArray &arr)
{
    Serial.println(F("[NotepadManager] listNotes() called"));

    // Open the root directory
    File root = LittleFS.open("/");
    if (!root || !root.isDirectory())
    {
        Serial.println(F("[NotepadManager] Failed to open or invalid root directory"));
        return;
    }

    Serial.println(F("[NotepadManager] Opened root directory, scanning files..."));

    int fileCount = 0;
    int noteCount = 0;

    // Iterate through all files in the root directory
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

            // Check if the file is a JSON file
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
}

// Load a note by experiment name into the provided string, returns true on success
bool NotepadManager::loadNote(const String &experiment, String &outNotes)
{
    // Construct the file path for the note
    String path = "/" + experiment + ".json";
    Serial.printf("[NotepadManager] Attempting to load note from path: %s\n", path.c_str());

    // Check if the file exists
    if (!LittleFS.exists(path))
    {
        Serial.printf("[NotepadManager] Note file does NOT exist: %s\n", path.c_str());
        return false;
    }

    // Open the file for reading
    File file = LittleFS.open(path, "r");
    if (!file)
    {
        Serial.printf("[NotepadManager] Failed to open note file: %s\n", path.c_str());
        return false;
    }

    Serial.printf("[NotepadManager] Successfully opened note file: %s\n", path.c_str());

    // Read the file content into the output string
    outNotes = file.readString();
    file.close();

    Serial.printf("[NotepadManager] Read %u bytes from file %s\n", outNotes.length(), path.c_str());

    // Check if the file content is empty
    if (outNotes.length() == 0)
    {
        Serial.printf("[NotepadManager] Note file is empty: %s\n", path.c_str());
        return false;
    }

    return true;
}

// Save a note for the given experiment name, returns true on success
bool NotepadManager::saveNote(const String &experiment, const String &notes)
{
    String path = "/" + experiment + ".json";

    File file = LittleFS.open(path, "w");
    if (!file)
    {
        Serial.printf("[NotepadManager] Failed to open note file %s for writing\n", path.c_str());
        return false;
    }

    size_t written = file.print(notes);
    file.close();

    if (written != notes.length())
    {
        Serial.printf("[NotepadManager] Failed to write full note to %s\n", path.c_str());
        return false;
    }
    Serial.printf("[NotepadManager] Saved note %s (%u bytes)\n", path.c_str(), (unsigned)written);
    return true;
}
