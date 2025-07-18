#pragma once
#include <ArduinoJson.h>
#include <string>

class NotepadManager {
public:
    static NotepadManager& getInstance();
    bool saveNote(const String& experiment, const String& notes);
    bool loadNote(const String& experiment, String& notesOut);
private:
    NotepadManager() = default;
    String getFilename(const String& experiment);
};
