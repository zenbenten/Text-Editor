#include "EditorWindow.h" // Includes FLTK headers needed for Window/Editor
#include "globals.h"      // For global variable definitions
#include "callbacks.h"    // For adding callbacks
#include "utils.h"        // For new_view(), load_file()
#include "syntax.h"       // For syntax highlighting setup

#include <FL/Fl.H>
#include <vector>

// --- Global Variable Definitions ---
// These are declared 'extern' in globals.h
int changed = 0;
int loading = 0;
char filename[256] = "";
Fl_Text_Buffer textbuf;  // The single shared text buffer
Fl_Text_Buffer stylebuf; // The single shared style buffer
std::vector<EditorWindow*> windows; // List of open editor windows

// --- Main Function ---
int main(int argc, char **argv) {

    // --- Initialize Shared Buffers ---
    textbuf.canUndo(1); // Enable undo for the text buffer
    // Style buffer undo is complex to sync reliably, rely on re-parse instead
    // stylebuf.canUndo(1);

    // --- Add Global Modify Callbacks ---
    // These callbacks are triggered whenever 'textbuf' changes.
    // Pass nullptr as user data; the callbacks will operate globally or iterate windows.
    textbuf.add_modify_callback(style_update, nullptr);
    textbuf.add_modify_callback(changed_cb, nullptr);

    // --- Create the First Editor View ---
    EditorWindow* first_window = new_view(); // new_view() adds itself to 'windows' vector

    // --- Show the First Window ---
    // Pass command-line arguments to FLTK (optional, but good practice)
    first_window->show(argc, argv);

    // --- Load File from Command Line (if provided) ---
    if (argc > 1) {
        load_file(argv[1]); // load_file handles styling and title updates
    } else {
         // Ensure styling and title are correct for an initial empty buffer
         textbuf.call_modify_callbacks();
    }

    // --- Enter FLTK Event Loop ---
    // Fl::run() returns when the last window is closed
    return Fl::run();
}

