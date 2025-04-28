#include "EditorWindow.h"
#include "callbacks.h" // For setting widget callbacks
#include "syntax.h"    // For styletable access and styletable_size
#include "globals.h"   // For textbuf, stylebuf access

#include <FL/fl_ask.H> // For fl_choice, fl_alert etc. (if needed directly here, though unlikely)

// --- EditorWindow Implementation ---

EditorWindow::EditorWindow(int W, int H, const char* t)
    : Fl_Double_Window(W, H, t) {

    static int window_count = 0;
    window_number = ++window_count;

    search[0] = '\0'; // Initialize search string for this window

    // --- Create Menu Bar ---
    menu = new Fl_Menu_Bar(0, 0, W, 30);

    // Define Menu Items (Callbacks are defined in callbacks.cpp)
    Fl_Menu_Item menuitems[] = {
        { "&File",              0, 0, 0, FL_SUBMENU },
            { "&New File",      FL_CTRL | 'n', (Fl_Callback *)new_cb, 0 },
            { "&Open File...",  FL_CTRL | 'o', (Fl_Callback *)open_cb, 0 },
            { "&Insert File...", FL_CTRL | 'i', (Fl_Callback *)insert_cb, this, FL_MENU_DIVIDER },
            { "&Save File",     FL_CTRL | 's', (Fl_Callback *)save_cb, 0 },
            { "Save File &As...", FL_CTRL | FL_SHIFT | 's', (Fl_Callback *)saveas_cb, 0, FL_MENU_DIVIDER },
            { "New &View",      FL_ALT | 'v', (Fl_Callback *)view_cb, 0 },
            { "&Close View",    FL_CTRL | 'w', (Fl_Callback *)close_cb, this, FL_MENU_DIVIDER },
            { "E&xit",          FL_CTRL | 'q', (Fl_Callback *)quit_cb, 0 },
            { 0 },
        { "&Edit",              0, 0, 0, FL_SUBMENU },
            { "&Undo",          FL_CTRL | 'z', (Fl_Callback *)undo_cb, 0, FL_MENU_DIVIDER },
            { "Cu&t",           FL_CTRL | 'x', (Fl_Callback *)cut_cb, this },
            { "&Copy",          FL_CTRL | 'c', (Fl_Callback *)copy_cb, this },
            { "&Paste",         FL_CTRL | 'v', (Fl_Callback *)paste_cb, this },
            { "&Delete",        0,             (Fl_Callback *)delete_cb, 0 },
            { 0 },
        { "&Search",            0, 0, 0, FL_SUBMENU },
            { "&Find...",       FL_CTRL | 'f', (Fl_Callback *)find_cb, this },
            { "F&ind Again",    FL_CTRL | 'g', (Fl_Callback *)find2_cb, this },
            { "&Replace...",    FL_CTRL | 'r', (Fl_Callback *)replace_cb, this },
            { "Re&place Again", FL_CTRL | 't', (Fl_Callback *)replace2_cb, this },
            { 0 },
        { 0 }
    };
    menu->copy(menuitems); // Assign menu items to the menu bar

    // --- Create Text Editor ---
    editor = new Fl_Text_Editor(0, 30, W, H - 30);
    editor->buffer(&textbuf); // Use the global text buffer
    editor->textfont(styletable[0].font); // Set default font from style table
    editor->textsize(styletable[0].size); // Set default size from style table

    // Setup syntax highlighting using global style buffer and table size variable
    editor->highlight_data(&stylebuf, styletable,
                           styletable_size, // Use the variable defined in syntax.cpp
                           'A', 0, 0); // 'A' is the default style character

    // --- Window Properties ---
    this->resizable(editor); // Make the editor widget resizable
    this->size_range(300, 200); // Minimum window size

    // --- Create Replace Dialog ---
    // This dialog is associated with this specific window instance
    replace_dlg = new Fl_Window(300, 105, "Replace");
    { // Use braces for scope
        replace_find = new Fl_Input(70, 10, 200, 25, "Find:");
        replace_with = new Fl_Input(70, 40, 200, 25, "Replace:");
        replace_all = new Fl_Button(10, 70, 90, 25, "Replace All");
        replace_next = new Fl_Return_Button(105, 70, 120, 25, "Replace Next");
        replace_cancel = new Fl_Button(230, 70, 60, 25, "Cancel");

        // Assign callbacks for the replace dialog buttons (pass 'this' window as user data)
        replace_all->callback(replall_cb, this);
        replace_next->callback(replace2_cb, this);
        replace_cancel->callback(replcan_cb, this);
    }
    replace_dlg->end(); // End of replace_dlg group
    replace_dlg->set_non_modal(); // Allow interaction with main window

    // Set the callback for the window's close button ('X')
    this->callback((Fl_Callback *)close_cb, this);

    // Note: Global buffer callbacks (changed_cb, style_update) are added in main.cpp
}

EditorWindow::~EditorWindow() {
    // Remove this window's pointer from the global list
    for (size_t i = 0; i < windows.size(); ++i) {
        if (windows[i] == this) {
            windows.erase(windows.begin() + i);
            break;
        }
    }

    // Explicitly delete the replace dialog if it exists and hasn't been deleted
    if (replace_dlg) {
        replace_dlg->hide(); // Hide first for safety
        delete replace_dlg;
        replace_dlg = nullptr; // Avoid dangling pointer
    }
    // Child widgets (menu, editor) are automatically deleted by FLTK
    // when the parent window (this) is deleted.
}

