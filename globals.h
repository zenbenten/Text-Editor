#ifndef GLOBALS_H
#define GLOBALS_H

#include <FL/Fl_Text_Buffer.H>
#include <vector>
#include "EditorWindow.h" // Include EditorWindow definition for the vector

// --- Global Variables (Declarations) ---
// These are defined in main.cpp

extern int changed;
extern int loading;
extern char filename[256];
extern Fl_Text_Buffer textbuf;  // Shared text buffer
extern Fl_Text_Buffer stylebuf; // Shared style buffer
extern std::vector<EditorWindow*> windows; // List of open editor windows

#endif // GLOBALS_H

