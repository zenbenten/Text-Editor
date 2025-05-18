#include "callbacks.h"
#include "EditorWindow.h" // To access EditorWindow members from user data 'v'
#include "globals.h"      // Access to global buffers, filename, changed flag etc.
#include "utils.h"        // Access to helper functions like check_save, load_file etc.
#include "syntax.h"       // For style_update calling redisplay_range

#include <FL/Fl_Text_Editor.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <string>
#include <vector>
#include <cstdlib> // For exit()

// --- Callback Implementations ---

// Buffer modify callbacks (global)
void changed_cb(int, int nInserted, int nDeleted, int, const char*, void* /*v*/) {
    if ((nInserted || nDeleted) && !loading) changed = 1;
    // Update title for all windows
    for (EditorWindow* w : windows) {
        set_title(w);
    }
}

// style_update is defined in syntax.cpp as it's part of syntax highlighting logic

// Menu item callbacks
void copy_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    if (e && e->editor) { // Safety check
        Fl_Text_Editor::kf_copy(0, e->editor);
    }
}

void cut_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
     if (e && e->editor) { // Safety check
        Fl_Text_Editor::kf_cut(0, e->editor);
    }
}

void delete_cb(Fl_Widget*, void* /*v*/) {
    textbuf.remove_selection(); // Operates on the shared buffer
}

void find_cb(Fl_Widget* w, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    if (!e) return;
    const char *val = fl_input("Search String:", e->search);
    if (val != NULL) {
        strncpy(e->search, val, sizeof(e->search) - 1); // Use strncpy for safety
        e->search[sizeof(e->search) - 1] = '\0'; // Ensure null termination
        find2_cb(w, v); // Pass window context
    }
}

void find2_cb(Fl_Widget* /*w*/, void* v) { // Find Again
    EditorWindow* e = (EditorWindow*)v;
    if (!e || !e->editor) return;
    if (e->search[0] == '\0') {
        find_cb(e, v); // Need to call find_cb with the specific window
        return;
    }

    int pos = e->editor->insert_position();
    int found_pos = pos;

    if (textbuf.search_forward(pos, e->search, &found_pos)) {
        textbuf.select(found_pos, found_pos + strlen(e->search));
        e->editor->insert_position(found_pos + strlen(e->search));
        e->editor->show_insert_position();
    } else {
        fl_alert("No more occurrences of \'%s\' found!", e->search);
    }
}

void insert_cb(Fl_Widget*, void* v) { // Insert File
    EditorWindow* e = (EditorWindow*)v;
    if (!e || !e->editor) return;
    char *newfile = fl_file_chooser("Insert File?", "*", filename); // Use global filename as default suggestion
    if (newfile != NULL) {
        int pos = e->editor->insert_position();
        load_file(newfile, pos); // Call load_file with insert position
    }
}

void new_cb(Fl_Widget*, void* /*v*/) {
    if (!check_save()) return; // Global check
    filename[0] = '\0';
    textbuf.select(0, textbuf.length());
    textbuf.remove_selection();
    stylebuf.select(0, stylebuf.length());
    stylebuf.remove_selection();
    changed = 0;
    textbuf.call_modify_callbacks(); // Update all views
}

void open_cb(Fl_Widget*, void* /*v*/) {
    if (!check_save()) return; // Global check
    char *newfile = fl_file_chooser("Open File?", "*", filename);
    if (newfile != NULL) {
        load_file(newfile); // Load into shared buffer
    }
}

void paste_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    if (e && e->editor) { // Safety check
        Fl_Text_Editor::kf_paste(0, e->editor);
    }
}

void quit_cb(Fl_Widget*, void* /*v*/) {
    std::vector<EditorWindow*> windows_copy = windows; // Iterate over a copy
    for (int i = windows_copy.size() - 1; i >= 0; --i) {
        EditorWindow* w = windows_copy[i];
        // Check if window still exists in original vector before trying to close
        bool still_exists = false;
        for(const auto* win_ptr : windows) if (win_ptr == w) { still_exists = true; break; }
        if (!still_exists) continue;

        close_cb(w, w); // Trigger close logic for the window

        // Check again if it was actually closed
        still_exists = false;
        for(const auto* win_ptr : windows) if (win_ptr == w) { still_exists = true; break; }
        if (still_exists) return; // Close was cancelled, abort quit
    }
    exit(0); // Exit only if all windows closed successfully
}

void replace_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    if (e && e->replace_dlg) {
        e->replace_dlg->show();
    }
}

void replace2_cb(Fl_Widget*, void* v) { // Replace Again / Replace Next
    EditorWindow* e = (EditorWindow*)v;
    if (!e || !e->editor || !e->replace_find || !e->replace_with || !e->replace_dlg) return;

    const char *find = e->replace_find->value();
    const char *replace = e->replace_with->value();

    if (find[0] == '\0') {
        e->replace_dlg->show();
        return;
    }

    int pos = e->editor->insert_position();
    int found_pos = pos;

    if (textbuf.search_forward(pos, find, &found_pos)) {
        textbuf.select(found_pos, found_pos + strlen(find));
        textbuf.remove_selection();
        textbuf.insert(found_pos, replace);
        textbuf.select(found_pos, found_pos + strlen(replace));
        e->editor->insert_position(found_pos + strlen(replace));
        e->editor->show_insert_position();
    } else {
        fl_alert("No more occurrences of \'%s\' found!", find);
    }
}

void replall_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
     if (!e || !e->replace_find || !e->replace_with || !e->replace_dlg) return;

    const char *find = e->replace_find->value();
    const char *replace = e->replace_with->value();

    if (find[0] == '\0') {
        e->replace_dlg->show();
        return;
    }
    e->replace_dlg->hide();

    int times = 0;
    int pos = 0;
    int found_pos = 0;

    while(textbuf.search_forward(pos, find, &found_pos)) {
        textbuf.select(found_pos, found_pos + strlen(find));
        textbuf.remove_selection();
        textbuf.insert(found_pos, replace);
        pos = found_pos + strlen(replace);
        times++;
    }

    if (times > 0) {
        fl_message("Replaced %d occurrences.", times);
        changed = 1;
        textbuf.call_modify_callbacks();
    } else {
        fl_alert("No occurrences of \'%s\' found!", find);
    }
}

void replcan_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    if (e && e->replace_dlg) {
        e->replace_dlg->hide();
    }
}

void save_cb(Fl_Widget*, void* /*v*/) {
    if (filename[0] == '\0') {
        saveas_cb(nullptr, nullptr); // Calls global saveas
    } else {
        save_file(filename); // Calls global save_file
    }
}

void saveas_cb(Fl_Widget*, void* /*v*/) {
    char *newfile = fl_file_chooser("Save File As?", "*", filename);
    if (newfile != NULL) {
        save_file(newfile); // Calls global save_file
    }
}

void undo_cb(Fl_Widget*, void* /*v*/) { // Undo
    textbuf.undo();
    textbuf.call_modify_callbacks(); // Trigger restyle and title update
}

void view_cb(Fl_Widget*, void* /*v*/) { // New View
    new_view()->show(); // Calls global utility function
}

void close_cb(Fl_Widget* /*w*/, void* v) { // Close View (also window close)
    EditorWindow* window_to_close = (EditorWindow*)v;
    if (!window_to_close) return;

    // Only prompt to save if this is the *last* window being closed
    if (changed && windows.size() == 1) {
         if (!check_save()) { // Global check_save
             return; // User cancelled save, don't close
         }
    }

    window_to_close->hide();
    // Use Fl::delete_widget to safely delete the window later in the event loop
    // This prevents issues if the callback was triggered during event handling
    Fl::delete_widget(window_to_close);
}

