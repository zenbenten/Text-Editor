#ifndef UTILS_H
#define UTILS_H

// Forward declare EditorWindow to avoid circular includes if possible
// If function implementations need full definition, include EditorWindow.h in utils.cpp
class EditorWindow;

// --- Utility Function Declarations ---
void set_title(EditorWindow* w);
int check_save(); // Checks global 'changed' flag
void load_file(const char *newfile, int ipos = -1); // Operates on global buffers
void save_file(const char *newfile); // Operates on global buffers
EditorWindow* new_view(); // Creates a new EditorWindow instance

#endif // UTILS_H

