#include "utils.h"
#include "globals.h"      // Access global vars (filename, changed, textbuf, stylebuf, windows)
#include "EditorWindow.h" // Need full definition for set_title, new_view
#include "callbacks.h"    // For check_save calling save_cb
#include "syntax.h"       // For load_file calling style_parse

#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H> // For fl_file_chooser used by load_file
#include <string>
#include <vector>
#include <cstdio>  // For strerror
#include <cstring> // For strcpy, strrchr, strlen, memset
#include <cstdlib> // For free

// --- Utility Function Implementations ---

// Sets the window title based on filename and changed status
void set_title(EditorWindow* w) {
    if (!w) return;
    std::string title_str;
    if (filename[0] == '\0') {
        title_str = "Untitled";
    } else {
        const char* slash = strrchr(filename, '/');
        #ifdef _WIN32 // Handle Windows backslashes too
        const char* backslash = strrchr(filename, '\\');
        if (backslash > slash) slash = backslash;
        #endif
        title_str = (slash ? slash + 1 : filename); // Use part after last slash, or whole string
    }
    if (changed) {
        title_str += " *"; // Use asterisk for modified indicator
    }
    // Add view number if more than one view exists
    if (windows.size() > 1 && w->window_number > 0) { // Check window_number validity
         title_str += " - View " + std::to_string(w->window_number);
    }
    w->copy_label(title_str.c_str()); // Set the window's label
}

// Checks if the buffer is changed and asks the user to save if it is.
// Returns: 1 if it's safe to proceed (not changed, saved, or discarded), 0 if user cancelled.
int check_save() {
    if (!changed) return 1; // Not changed, safe to proceed

    int r = fl_choice("Save changes to document?", // Prompt
                      "Cancel",                    // Button 0
                      "Save",                      // Button 1
                      "Don't Save");               // Button 2

    if (r == 1) { // Save chosen
        save_cb(nullptr, nullptr); // Call the global save callback
        return !changed; // Return 1 if save succeeded (changed==0), 0 otherwise
    }

    return (r != 0); // Return 1 if Don't Save (r=2), 0 if Cancel (r=0)
}

// Loads/inserts a file into the global text buffer and updates styles
void load_file(const char *newfile, int ipos) {
    loading = 1; // Prevent changed_cb from setting 'changed' flag during load
    int insert = (ipos != -1); // Check if inserting or replacing buffer content
    if (!insert) {
        strncpy(filename, newfile, sizeof(filename) - 1); // Update global filename only when replacing
        filename[sizeof(filename) - 1] = '\0';
    }

    int r; // Result of file operation
    if (!insert) {
        r = textbuf.loadfile(newfile); // Replace buffer content
    } else {
        r = textbuf.insertfile(newfile, ipos); // Insert file content at position
    }

    if (r) { // Error occurred
        fl_alert("Error reading from file \'%s\':\n%s.", newfile, strerror(errno));
        loading = 0; // Reset loading flag
        return;      // Exit function on error
    }
    // File operation successful
    changed = insert; // Mark as changed only if inserting, otherwise it's newly loaded/unchanged
    loading = 0; // Clear loading flag

    // --- Fully restyle the buffer after load/insert ---
    int text_len_val = textbuf.length();
    stylebuf.select(0, stylebuf.length());
    stylebuf.remove_selection(); // Clear old styles
    char* text = textbuf.text(); // Get the full text
    if (text) {
        char* styles = new char[text_len_val + 1]; // Allocate buffer for styles
        memset(styles, 'A', text_len_val);         // Initialize with default style 'A'
        styles[text_len_val] = '\0';
        style_parse(text, styles, text_len_val);   // Parse text to generate styles
        stylebuf.text(styles);                     // Set the new styles in the style buffer
        free(text);                                // Free text buffer allocated by textbuf.text()
        delete[] styles;                           // Free our allocated style buffer
    } else {
        stylebuf.text(""); // Ensure style buffer is empty if text buffer is empty
    }

    textbuf.call_modify_callbacks(); // Update titles and trigger style_update if needed
}

// Saves the global text buffer to the specified file
void save_file(const char *newfile) {
    if (textbuf.savefile(newfile)) { // Attempt to save
        fl_alert("Error writing to file \'%s\':\n%s.", newfile, strerror(errno));
    } else { // Success
        strncpy(filename, newfile, sizeof(filename) - 1); // Update global filename
        filename[sizeof(filename) - 1] = '\0';
        changed = 0; // Mark as unchanged
    }
    textbuf.call_modify_callbacks(); // Update titles in all windows
}

// Creates and configures a new EditorWindow instance
EditorWindow* new_view() {
    EditorWindow* w = new EditorWindow(800, 600, "Untitled"); // Create window
    windows.push_back(w); // Add to global list of windows
    set_title(w); // Set initial title (will reflect global filename/changed state)
    if (w->editor) { // Check editor exists
        w->editor->redraw(); // Use redraw() instead of redisplay()
    }
    return w;
}

