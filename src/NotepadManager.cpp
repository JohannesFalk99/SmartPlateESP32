#include "NotepadManager.h"
#include <FS.h>
#include <LittleFS.h>

NotepadManager& NotepadManager::getInstance() {
    static NotepadManager instance;
    return instance;
}

String NotepadManager::getFilename(const String& experiment) {
    String safeExp = experiment;
    safeExp.replace("/", "_");
    safeExp.replace("\\", "_");
    if (safeExp.length() == 0) safeExp = "default";
    return "/notepad_" + safeExp + ".txt";
}

bool NotepadManager::saveNote(const String& experiment, const String& notes) {
    if (!LittleFS.begin()) return false;
    String filename = getFilename(experiment);
    File file = LittleFS.open(filename, "w");
    if (!file) return false;
    file.print(notes);
    file.close();
    return true;
}

bool NotepadManager::loadNote(const String& experiment, String& notesOut) {
    if (!LittleFS.begin()) return false;
    String filename = getFilename(experiment);
    File file = LittleFS.open(filename, "r");
    if (!file) return false;
    notesOut = file.readString();
    file.close();
    return true;
}
