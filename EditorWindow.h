#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>

// --- EditorWindow Class Definition ---
class EditorWindow : public Fl_Double_Window {
public:
    // Constructor and Destructor
    EditorWindow(int w, int h, const char* title);
    ~EditorWindow(); // Important for cleanup, especially with multiple views

    // --- Widgets ---
    Fl_Menu_Bar* menu = nullptr;
    Fl_Text_Editor* editor = nullptr;

    // Replace Dialog Widgets (owned by this window)
    Fl_Window      *replace_dlg = nullptr;
    Fl_Input       *replace_find = nullptr;
    Fl_Input       *replace_with = nullptr;
    Fl_Button      *replace_all = nullptr;
    Fl_Return_Button *replace_next = nullptr;
    Fl_Button      *replace_cancel = nullptr;

    // --- State ---
    char search[256];      // Per-window search term storage
    int window_number;     // Unique identifier for the view
};

#endif // EDITORWINDOW_H

