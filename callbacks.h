#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <FL/Fl_Widget.H> // Needed for Fl_Widget* parameter type

// --- Callback Function Declarations ---

// Buffer modify callbacks (global)
void changed_cb(int, int, int, int, const char*, void*);
void style_update(int pos, int nInserted, int nDeleted, int nRestyled, const char *deletedText, void *cbArg);

// Menu item callbacks
void copy_cb(Fl_Widget*, void* v);
void cut_cb(Fl_Widget*, void* v);
void delete_cb(Fl_Widget*, void* v);
void find_cb(Fl_Widget* w, void* v);
void find2_cb(Fl_Widget* w, void* v); // Find Again
void insert_cb(Fl_Widget*, void* v); // Insert File
void new_cb(Fl_Widget*, void* v);
void open_cb(Fl_Widget*, void* v);
void paste_cb(Fl_Widget*, void* v);
void quit_cb(Fl_Widget*, void* v);
void replace_cb(Fl_Widget*, void* v);
void replace2_cb(Fl_Widget*, void* v); // Replace Again / Replace Next
void save_cb(Fl_Widget*, void* v);
void saveas_cb(Fl_Widget*, void* v);
void undo_cb(Fl_Widget*, void* v); // Undo
void view_cb(Fl_Widget*, void* v); // New View
void close_cb(Fl_Widget* w, void* v); // Close View (also window close)

// Replace dialog button callbacks
void replall_cb(Fl_Widget*, void* v);
void replcan_cb(Fl_Widget*, void* v);
// replace2_cb is used for replace_next button

#endif // CALLBACKS_H

