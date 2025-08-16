#pragma once

#include <ArduinoJson.h>
#include <Arduino.h>

class NotepadManager
{
public:
    static NotepadManager &getInstance();
    void listNotes(JsonArray &arr);
    bool loadNote(const String &experiment, String &outNotes);
    bool saveNote(const String &experiment, const String &notes);
private:
    NotepadManager();
    NotepadManager(const NotepadManager &) = delete;
    NotepadManager &operator=(const NotepadManager &) = delete;
};
