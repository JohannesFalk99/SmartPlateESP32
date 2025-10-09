#ifndef NOTEPADMANAGER_H
#define NOTEPADMANAGER_H

#include <ArduinoJson.h>
#include <Arduino.h>

/**
 * @brief Manages notepad functionality for experiment notes
 * 
 * This singleton class provides functionality to save, load, and list
 * experiment notes stored in the file system.
 */
class NotepadManager
{
public:
    /**
     * @brief Get the singleton instance
     * @return NotepadManager& Reference to the singleton instance
     */
    static NotepadManager &getInstance();

    /**
     * @brief List all available notes
     * @param arr JsonArray to populate with note names
     */
    void listNotes(JsonArray &arr);
    
    /**
     * @brief Load notes for a specific experiment
     * @param experiment Name of the experiment
     * @param outNotes String reference to store the loaded notes
     * @return true if notes were successfully loaded
     * @return false if loading failed
     */
    bool loadNote(const String &experiment, String &outNotes);
    
    /**
     * @brief Save notes for a specific experiment
     * @param experiment Name of the experiment
     * @param notes Notes content to save
     * @return true if notes were successfully saved
     * @return false if saving failed
     */
    bool saveNote(const String &experiment, const String &notes);

private:
    /**
     * @brief Private constructor for singleton pattern
     */
    NotepadManager();
    
    NotepadManager(const NotepadManager &) = delete;
    NotepadManager &operator=(const NotepadManager &) = delete;
};

#endif // NOTEPADMANAGER_H
