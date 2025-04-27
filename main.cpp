#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <string>
#include <cstdlib>
#include <cctype>
#include <vector> // Needed for managing multiple views

// --- Forward Declarations ---
class EditorWindow;
EditorWindow* new_view(); // Function to create a new editor window

// --- Global Variables ---
int changed = 0;
int loading = 0;
char filename[256] = "";
Fl_Text_Buffer textbuf; // Single text buffer shared by all views
Fl_Text_Buffer stylebuf; // Single style buffer shared by all views
std::vector<EditorWindow*> windows; // Keep track of open windows

// --- Syntax Highlighting Data ---
Fl_Text_Display::Style_Table_Entry styletable[] = {
  { FL_BLACK,      FL_COURIER,        14 }, // A - Plain
  { FL_DARK_GREEN, FL_COURIER_ITALIC, 14 }, // B - Line comments
  { FL_DARK_GREEN, FL_COURIER_ITALIC, 14 }, // C - Block comments
  { FL_BLUE,       FL_COURIER,        14 }, // D - Strings
  { FL_DARK_RED,   FL_COURIER,        14 }, // E - Directives
  { FL_DARK_RED,   FL_COURIER_BOLD,   14 }, // F - Types
  { FL_BLUE,       FL_COURIER_BOLD,   14 }  // G - Keywords
};

const char *code_keywords[] = {
  "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break",
  "case", "catch", "char", "class", "compl", "const", "const_cast",
  "continue", "default", "delete", "do", "double", "dynamic_cast", "else",
  "enum", "explicit", "export", "extern", "false", "float", "for",
  "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace",
  "new", "not", "not_eq", "operator", "or", "or_eq", "private",
  "protected", "public", "register", "reinterpret_cast", "return",
  "short", "signed", "sizeof", "static", "static_cast", "struct",
  "switch", "template", "this", "throw", "true", "try", "typedef",
  "typeid", "typename", "union", "unsigned", "using", "virtual", "void",
  "volatile", "wchar_t", "while", "xor", "xor_eq"
};

const char *code_types[] = {
  "Fl_Widget", "Fl_Window", "Fl_Double_Window", "Fl_Gl_Window",
  "Fl_Button", "Fl_Input", "Fl_Text_Editor", "Fl_Text_Buffer",
  "Fl_Menu_Bar", "Fl_File_Chooser"
};


// --- Callback Prototypes ---
void changed_cb(int, int, int, int, const char*, void*);
void copy_cb(Fl_Widget*, void*);
void cut_cb(Fl_Widget*, void*);
void delete_cb(Fl_Widget*, void*);
void find_cb(Fl_Widget*, void*);
void find2_cb(Fl_Widget*, void*); // Find Again
void insert_cb(Fl_Widget*, void*); // Insert File
void new_cb(Fl_Widget*, void*);
void open_cb(Fl_Widget*, void*);
void paste_cb(Fl_Widget*, void*);
void quit_cb(Fl_Widget*, void*);
void replace_cb(Fl_Widget*, void*);
void replace2_cb(Fl_Widget*, void*); // Replace Again / Replace Next
void replall_cb(Fl_Widget*, void*);
void replcan_cb(Fl_Widget*, void*);
void save_cb(Fl_Widget*, void*);
void saveas_cb(Fl_Widget*, void*);
void undo_cb(Fl_Widget*, void*); // Undo
void view_cb(Fl_Widget*, void*); // New View
void close_cb(Fl_Widget*, void*); // Close View (also window close)

// --- Helper Function Prototypes ---
void set_title(EditorWindow* w);
int check_save();
void load_file(const char *newfile, int ipos = -1);
void save_file(const char *newfile);

// --- Syntax Highlighting Prototypes ---
void style_parse(const char *text, char *style, int length);
void style_update(int pos, int nInserted, int nDeleted, int nRestyled, const char *deletedText, void *cbArg);
int compare_keywords(const void *p1, const void *p2);

// --- EditorWindow Class Definition ---
class EditorWindow : public Fl_Double_Window {
public:
    EditorWindow(int w, int h, const char* title);
    ~EditorWindow(); // Make sure to implement destructor for cleanup

    Fl_Menu_Bar* menu;
    Fl_Text_Editor* editor;

    Fl_Window      *replace_dlg;
    Fl_Input       *replace_find;
    Fl_Input       *replace_with;
    Fl_Button      *replace_all;
    Fl_Return_Button *replace_next;
    Fl_Button      *replace_cancel;

    char search[256];
    int window_number; // To distinguish windows if needed
};

// --- EditorWindow Implementation ---

EditorWindow::EditorWindow(int W, int H, const char* t)
    : Fl_Double_Window(W, H, t) {

    static int window_count = 0;
    window_number = ++window_count;

    search[0] = '\0';

    menu = new Fl_Menu_Bar(0, 0, W, 30);

    Fl_Menu_Item menuitems[] = {
        { "&File",              0, 0, 0, FL_SUBMENU },
            { "&New File",      FL_CTRL | 'n', (Fl_Callback *)new_cb, 0 }, // Pass 0 for global callbacks
            { "&Open File...",  FL_CTRL | 'o', (Fl_Callback *)open_cb, 0 }, // Pass 0 for global callbacks
            { "&Insert File...", FL_CTRL | 'i', (Fl_Callback *)insert_cb, this, FL_MENU_DIVIDER }, // Needs window context
            { "&Save File",     FL_CTRL | 's', (Fl_Callback *)save_cb, 0 }, // Pass 0 for global callbacks
            { "Save File &As...", FL_CTRL | FL_SHIFT | 's', (Fl_Callback *)saveas_cb, 0, FL_MENU_DIVIDER }, // Pass 0
            { "New &View",      FL_ALT | 'v', (Fl_Callback *)view_cb, 0 },
            { "&Close View",    FL_CTRL | 'w', (Fl_Callback *)close_cb, this, FL_MENU_DIVIDER }, // Needs window context
            { "E&xit",          FL_CTRL | 'q', (Fl_Callback *)quit_cb, 0 },
            { 0 },
        { "&Edit",              0, 0, 0, FL_SUBMENU },
            { "&Undo",          FL_CTRL | 'z', (Fl_Callback *)undo_cb, 0, FL_MENU_DIVIDER },
            { "Cu&t",           FL_CTRL | 'x', (Fl_Callback *)cut_cb, this }, // Needs window context
            { "&Copy",          FL_CTRL | 'c', (Fl_Callback *)copy_cb, this }, // Needs window context
            { "&Paste",         FL_CTRL | 'v', (Fl_Callback *)paste_cb, this }, // Needs window context
            { "&Delete",        0,             (Fl_Callback *)delete_cb, 0 }, // Operates globally
            { 0 },
        { "&Search",            0, 0, 0, FL_SUBMENU },
            { "&Find...",       FL_CTRL | 'f', (Fl_Callback *)find_cb, this }, // Needs window context
            { "F&ind Again",    FL_CTRL | 'g', (Fl_Callback *)find2_cb, this }, // Needs window context
            { "&Replace...",    FL_CTRL | 'r', (Fl_Callback *)replace_cb, this }, // Needs window context
            { "Re&place Again", FL_CTRL | 't', (Fl_Callback *)replace2_cb, this }, // Needs window context
            { 0 },
        { 0 }
    };

    menu->copy(menuitems);

    editor = new Fl_Text_Editor(0, 30, W, H - 30);
    editor->buffer(&textbuf); // All editors share the same buffer
    editor->textfont(styletable[0].font);
    editor->textsize(styletable[0].size);

    editor->highlight_data(&stylebuf, styletable, // All editors share style buffer
                           sizeof(styletable) / sizeof(styletable[0]),
                           'A', 0, 0);

    this->resizable(editor);
    this->size_range(300, 200);

    // Callbacks are added globally after buffer creation in main()

    replace_dlg = new Fl_Window(300, 105, "Replace");
    {
        replace_find = new Fl_Input(70, 10, 200, 25, "Find:");
        replace_with = new Fl_Input(70, 40, 200, 25, "Replace:");
        replace_all = new Fl_Button(10, 70, 90, 25, "Replace All");
        replace_next = new Fl_Return_Button(105, 70, 120, 25, "Replace Next");
        replace_cancel = new Fl_Button(230, 70, 60, 25, "Cancel");

        // These callbacks need the window context for the dialog widgets
        replace_all->callback(replall_cb, this);
        replace_next->callback(replace2_cb, this);
        replace_cancel->callback(replcan_cb, this);
    }
    replace_dlg->end();
    replace_dlg->set_non_modal();

    // Window close ('X') button triggers close_cb
    this->callback((Fl_Callback *)close_cb, this);
}

EditorWindow::~EditorWindow() {
    // Important for multi-view: Remove window pointer from the global list
    for (size_t i = 0; i < windows.size(); ++i) {
        if (windows[i] == this) {
            windows.erase(windows.begin() + i);
            break;
        }
    }
    // Hide and delete the replace dialog if it exists
    if (replace_dlg) {
        replace_dlg->hide(); // Hide first
        delete replace_dlg; // Then delete
        replace_dlg = nullptr; // Avoid dangling pointer
    }
     // FLTK handles deletion of child widgets (menu, editor)
}

// --- Callback Implementations ---

void changed_cb(int, int nInserted, int nDeleted, int, const char*, void* /*v*/) {
    // Modify callback is global now, doesn't need window pointer 'v'
    if ((nInserted || nDeleted) && !loading) changed = 1;
    // Update title for all windows
    for (EditorWindow* w : windows) {
        set_title(w);
    }
}

void copy_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    Fl_Text_Editor::kf_copy(0, e->editor);
}

void cut_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    Fl_Text_Editor::kf_cut(0, e->editor);
}

void delete_cb(Fl_Widget*, void* /*v*/) {
    // Operates on the shared buffer
    textbuf.remove_selection();
}

void find_cb(Fl_Widget* w, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    const char *val = fl_input("Search String:", e->search);
    if (val != NULL) {
        strcpy(e->search, val);
        find2_cb(w, v); // Pass window context
    }
}

void find2_cb(Fl_Widget* /*w*/, void* v) { // Find Again
    EditorWindow* e = (EditorWindow*)v; // Need window context for search term and editor focus
    if (e->search[0] == '\0') {
        find_cb(e, v); // Need to call find_cb with the specific window
        return;
    }

    int pos = e->editor->insert_position();
    int found_pos = pos;

    // Search in the shared buffer
    if (textbuf.search_forward(pos, e->search, &found_pos)) {
        textbuf.select(found_pos, found_pos + strlen(e->search));
        e->editor->insert_position(found_pos + strlen(e->search));
        e->editor->show_insert_position(); // Show in the current editor view
    }
    else {
        fl_alert("No more occurrences of \'%s\' found!", e->search);
    }
}

void insert_cb(Fl_Widget*, void* v) { // Insert File
    EditorWindow* e = (EditorWindow*)v;
    char *newfile = fl_file_chooser("Insert File?", "*", filename);
    if (newfile != NULL) {
        int pos = e->editor->insert_position(); // Get position from current view
        load_file(newfile, pos); // Call load_file with insert position
    }
}

void new_cb(Fl_Widget*, void* /*v*/) {
    if (!check_save()) return;
    filename[0] = '\0';
    textbuf.select(0, textbuf.length());
    textbuf.remove_selection();
    stylebuf.select(0, stylebuf.length());
    stylebuf.remove_selection();
    changed = 0;
    textbuf.call_modify_callbacks(); // Update all views
}

void open_cb(Fl_Widget*, void* /*v*/) {
    if (!check_save()) return;
    char *newfile = fl_file_chooser("Open File?", "*", filename);
    if (newfile != NULL) {
        load_file(newfile); // Load into shared buffer, updates all views via callbacks
    }
}

void paste_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    Fl_Text_Editor::kf_paste(0, e->editor);
}

void quit_cb(Fl_Widget*, void* /*v*/) {
    // Attempt to close all windows gracefully
    // Iterate backwards because closing removes elements
    // Use a copy of the vector to avoid issues while iterating and deleting
    std::vector<EditorWindow*> windows_copy = windows;
    for (int i = windows_copy.size() - 1; i >= 0; --i) {
        EditorWindow* w = windows_copy[i];
        // Check if the window still exists in the original vector
        bool still_exists = false;
        for(const auto* win_ptr : windows) {
            if (win_ptr == w) {
                still_exists = true;
                break;
            }
        }
        if (!still_exists) continue; // Window was already closed

        // Trigger the window's close callback logic
        close_cb(w, w);

        // Check if the window was actually closed (removed from original vector)
        still_exists = false;
        for(const auto* win_ptr : windows) {
            if (win_ptr == w) {
                still_exists = true;
                break;
            }
        }
        if (still_exists) {
             // The window wasn't removed, meaning close was cancelled
             return; // Abort quitting
        }
    }

    // If loop completes, all windows were closed successfully (or none existed)
    exit(0);
}

void replace_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    e->replace_dlg->show();
}

void replace2_cb(Fl_Widget*, void* v) { // Replace Again / Replace Next
    EditorWindow* e = (EditorWindow*)v;
    const char *find = e->replace_find->value();
    const char *replace = e->replace_with->value();

    if (find[0] == '\0') {
        e->replace_dlg->show();
        return;
    }

    int pos = e->editor->insert_position(); // Position from current view
    int found_pos = pos;

    // Search shared buffer
    if (textbuf.search_forward(pos, find, &found_pos)) {
        textbuf.select(found_pos, found_pos + strlen(find));
        textbuf.remove_selection();
        textbuf.insert(found_pos, replace);
        textbuf.select(found_pos, found_pos + strlen(replace)); // Select replaced text
        e->editor->insert_position(found_pos + strlen(replace)); // Move cursor
        e->editor->show_insert_position(); // Show in current view
    } else {
        fl_alert("No more occurrences of \'%s\' found!", find);
    }
}

void replall_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v; // Context for the dialog values
    const char *find = e->replace_find->value();
    const char *replace = e->replace_with->value();

    if (find[0] == '\0') {
        e->replace_dlg->show(); // Show dialog if find is empty
        return;
    }

    e->replace_dlg->hide();

    int times = 0;
    int pos = 0; // Start search from beginning of shared buffer
    int found_pos = 0;

    while(textbuf.search_forward(pos, find, &found_pos)) {
        textbuf.select(found_pos, found_pos + strlen(find)); // Select the match
        textbuf.remove_selection(); // Remove it
        textbuf.insert(found_pos, replace); // Insert replacement
        pos = found_pos + strlen(replace); // Continue search *after* the replacement
        times++;
    }

    if (times > 0) {
        fl_message("Replaced %d occurrences.", times);
        changed = 1; // Mark as changed
        textbuf.call_modify_callbacks(); // Update display and title in all views
    } else {
        fl_alert("No occurrences of \'%s\' found!", find);
    }
}

void replcan_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    e->replace_dlg->hide();
}

void save_cb(Fl_Widget*, void* /*v*/) {
    if (filename[0] == '\0') {
        saveas_cb(nullptr, nullptr);
    } else {
        save_file(filename);
    }
}

void saveas_cb(Fl_Widget*, void* /*v*/) {
    char *newfile = fl_file_chooser("Save File As?", "*", filename);
    if (newfile != NULL) {
        save_file(newfile);
    }
}

void undo_cb(Fl_Widget*, void* /*v*/) { // Undo
    textbuf.undo();
    // Style buffer undo might not be perfectly synchronized,
    // it's often better to re-parse after undo.
    // stylebuf.undo();
    textbuf.call_modify_callbacks(); // Trigger restyle
}

void view_cb(Fl_Widget*, void* /*v*/) { // New View
    new_view()->show();
}

void close_cb(Fl_Widget* w, void* v) { // Close View
    EditorWindow* window_to_close = (EditorWindow*)v;

    if (changed && windows.size() == 1) { // Only check save on last window close
         if (!check_save()) {
             return; // User cancelled save, so don't close
         }
         // If saved or discarded, proceed to close
    } else if (changed && windows.size() > 1) {
        // If not the last window, just close it without saving prompt
        // The change state persists in the buffer for other windows
    }


    window_to_close->hide(); // Hide the window first
    delete window_to_close; // Destructor removes it from 'windows' vector

    if (windows.empty()) {
        exit(0); // Exit if it was the last window
    }
}


// --- Helper Functions ---

void set_title(EditorWindow* w) {
    if (!w) return;
    std::string title;
    if (filename[0] == '\0') {
        title = "Untitled";
    } else {
        const char* slash = strrchr(filename, '/');
#ifdef _WIN32
        const char* backslash = strrchr(filename, '\\');
        if (backslash > slash) slash = backslash;
#endif
        title = (slash ? slash + 1 : filename);
    }
    if (changed) {
        title += " *"; // Use asterisk for modified indicator
    }
    // Add view number if more than one view exists
    if (windows.size() > 1) {
         title += " - View " + std::to_string(w->window_number);
    }
    w->copy_label(title.c_str());
}

int check_save() {
    // Check applies globally to the shared buffer
    if (!changed) return 1;

    int r = fl_choice("Save changes to document?", // Simpler prompt
                      "Cancel",                         // Button 0
                      "Save",                           // Button 1
                      "Don't Save");                    // Button 2

    if (r == 1) { // Save
        save_cb(nullptr, nullptr); // Try to save
        return !changed; // Return 1 if save succeeded (changed == 0), 0 otherwise
    }

    return (r != 0); // Return 1 if Don't Save (r=2), 0 if Cancel (r=0)
}

void load_file(const char *newfile, int ipos) {
    loading = 1;
    int insert = (ipos != -1);
    if (!insert) strcpy(filename, ""); // Clear global filename only if replacing

    int r;
    if (!insert) {
        r = textbuf.loadfile(newfile);
    } else {
        r = textbuf.insertfile(newfile, ipos);
    }

    if (r) {
        fl_alert("Error reading from file \'%s\':\n%s.", newfile, strerror(errno));
        loading = 0; // Reset loading flag on error
        return; // Exit function on error
    } else {
        if (!insert) strcpy(filename, newfile); // Set global filename if replacing
    }

    changed = insert; // Mark changed only if inserting
    loading = 0;

    // Fully restyle after load/insert
    int text_len_val = textbuf.length();
    stylebuf.select(0, stylebuf.length());
    stylebuf.remove_selection(); // Clear old styles
    char* text = textbuf.text();
    if (text) {
        char* styles = new char[text_len_val + 1];
        memset(styles, 'A', text_len_val);
        styles[text_len_val] = '\0';
        style_parse(text, styles, text_len_val);
        stylebuf.text(styles); // Set the new styles
        free(text);
        delete[] styles;
    } else {
        stylebuf.text(""); // Ensure style buffer is empty if text buffer is empty
    }
    textbuf.call_modify_callbacks(); // Update titles and trigger style_update if needed
}

void save_file(const char *newfile) {
    // Saves the shared buffer
    if (textbuf.savefile(newfile)) {
        fl_alert("Error writing to file \'%s\':\n%s.", newfile, strerror(errno));
    } else {
        strcpy(filename, newfile); // Update global filename
        changed = 0; // Mark as unchanged
    }
    // Update titles in all windows
    textbuf.call_modify_callbacks();
}

// --- Syntax Highlighting Functions ---

int compare_keywords(const void *p1, const void *p2) {
  return strcmp(*(const char **)p1, *(const char **)p2);
}

void style_parse(const char *text, char *style, int length) {
  char             current;
  int              col;
  int              last;
  char             buf[255],
                   *bufptr;
  const char       *temp;
  int              i;
  const char       **keyword;

  if (!style || !text) return; // Basic safety check
  char* style_ptr = style;

  // Get the style from the previous character if possible
  // (This part requires the style_update function to handle context correctly)
  // For now, assume starting state is 'A' unless told otherwise externally
  current = *style_ptr;


  for (col = 0, last = 0; length > 0; length --, text ++) {
    if (current == 'A') {
      if (col == 0 && *text == '#') current = 'E';
      else if (strncmp(text, "//", 2) == 0) current = 'B';
      else if (strncmp(text, "/*", 2) == 0) current = 'C';
      else if (strncmp(text, "\\\"", 2) == 0) {
        if (length >= 2) { *style_ptr++ = current; *style_ptr++ = current; text++; length--; col += 2; }
        else if (length == 1) { *style_ptr++ = current; col++; }
        if (length <= 1) break; continue;
      } else if (*text == '\"') current = 'D';
      else if (!last && isalpha(*text)) {
        for (temp = text, bufptr = buf; (isalnum(*temp) || *temp == '_') && bufptr < (buf + sizeof(buf) - 1); *bufptr++ = *temp++);
        *bufptr = '\0'; bufptr = buf; char matched_style = 'A';
        keyword = (const char **)bsearch(&bufptr, code_types, sizeof(code_types) / sizeof(code_types[0]), sizeof(code_types[0]), compare_keywords);
        if (keyword != NULL) matched_style = 'F';
        else {
          keyword = (const char **)bsearch(&bufptr, code_keywords, sizeof(code_keywords) / sizeof(code_keywords[0]), sizeof(code_keywords[0]), compare_keywords);
          if (keyword != NULL) matched_style = 'G';
        }
        if (matched_style != 'A') {
            int keyword_len = (int)strlen(buf);
            for (i = 0; i < keyword_len; i++) *style_ptr++ = matched_style;
            text += keyword_len - 1; length -= keyword_len -1; col += keyword_len;
            last = 1; current = 'A'; continue;
        }
      }
    } else if (current == 'C' && strncmp(text, "*/", 2) == 0) {
        if (length >= 2) { *style_ptr++ = current; *style_ptr++ = current; text++; length--; col += 2; current = 'A'; }
        else if (length == 1) { *style_ptr++ = current; col++; }
       if (length <= 1) break; continue;
    } else if (current == 'D') {
      if (strncmp(text, "\\\"", 2) == 0) {
         if (length >= 2) { *style_ptr++ = current; *style_ptr++ = current; text++; length--; col += 2; }
         else if (length == 1) { *style_ptr++ = current; col++; }
        if (length <= 1) break; continue;
      } else if (*text == '\"') { *style_ptr++ = current; col++; current = 'A'; continue; }
    }

    *style_ptr++ = current; col ++;
    last = isalnum(*text) || *text == '_';
    if (*text == '\n') { col = 0; if (current == 'B' || current == 'E') current = 'A'; }
  }
}


// Modified style_update to redisplay all windows
void style_update(int pos, int nInserted, int nDeleted, int, const char*, void* /*cbArg*/) {
  int start, end;
  char *style = NULL;
  char *text = NULL;
  char last_style_before_parse = 'A';
  char last_style_after_parse = 'A';

  if (nInserted == 0 && nDeleted == 0) {
    // Don't unselect here, selection is per-view
    // stylebuf.unselect();
    return;
  }

  // --- Handle buffer modification ---
  if (nInserted > 0) {
    style = new char[nInserted + 1];
    memset(style, 'A', nInserted); style[nInserted] = '\0';
    stylebuf.replace(pos, pos + nDeleted, style);
    delete[] style; style = NULL;
  } else {
    stylebuf.remove(pos, pos + nDeleted);
  }

  // --- Determine range to re-parse ---
  // stylebuf.unselect(); // Selection is per-view, don't unselect globally

  start = textbuf.line_start(pos);
  end = textbuf.line_end(pos + nInserted);
  int text_len = textbuf.length();
  if (end > text_len) end = text_len;
  int style_len = stylebuf.length();
  int check_pos = start > 0 ? start - 1 : 0;

  // Check context before the change start position
  char initial_style_context = 'A';
  if (check_pos < style_len) {
       initial_style_context = stylebuf.byte_at(check_pos);
       // If previous char indicates a C comment, try to find the real start
       if (initial_style_context == 'C') {
            int comment_start = start;
            while(comment_start > 0) {
                if (comment_start >= 2 && stylebuf.byte_at(comment_start -1) == 'C' && textbuf.byte_at(comment_start - 1) == '/' && textbuf.byte_at(comment_start - 2) == '*') break; // End of prior comment
                if (comment_start >= 2 && stylebuf.byte_at(comment_start -1) == 'C' && textbuf.byte_at(comment_start - 1) == '*' && textbuf.byte_at(comment_start - 2) == '/') { start = textbuf.line_start(comment_start - 2); initial_style_context = 'A'; break; } // Found start
                comment_start--;
            }
            if (comment_start == 0) start = 0; // Scanned all the way back
       }
  }


  // --- Parse the primary affected range ---
  text = textbuf.text_range(start, end); if (!text) goto cleanup;
  style = stylebuf.text_range(start, end); if (!style) goto cleanup;

  // Get style at end before parsing
  if (end > start && end <= style_len) last_style_before_parse = stylebuf.byte_at(end - 1);
  else if (style_len > 0 && start == end && start > 0) last_style_before_parse = stylebuf.byte_at(start - 1);
  else last_style_before_parse = 'A';

  // Parse the segment, providing initial context if needed
  if (end > start) style[0] = initial_style_context; // Set context for parser if parsing non-empty range
  style_parse(text, style, end - start);

  // Replace the style buffer segment
  stylebuf.replace(start, end, style);

  // Get style at end after parsing
  if (end > start) last_style_after_parse = style[end - start - 1];
  else last_style_after_parse = initial_style_context; // If range empty, style is unchanged


  // --- Check if reparsing the rest of the buffer is needed ---
  if (last_style_after_parse != last_style_before_parse || last_style_after_parse == 'C') {
      int rest_start = end; int rest_end = textbuf.length();
      if (rest_start < rest_end) {
          free(text); text = NULL; free(style); style = NULL;
          text = textbuf.text_range(rest_start, rest_end); if (!text) goto cleanup;
          style = stylebuf.text_range(rest_start, rest_end); if (!style) goto cleanup;

          // Parse the remainder, starting with the style we ended the previous section with
          if (rest_end > rest_start) style[0] = last_style_after_parse;
          style_parse(text, style, rest_end - rest_start);

          stylebuf.replace(rest_start, rest_end, style);
          end = rest_end; // Update end to cover the re-parsed section for redisplay
      }
  }

cleanup:
  free(text); free(style);
  // --- Redisplay ALL windows ---
  for (EditorWindow* w : windows) {
      w->editor->redisplay_range(start, end);
  }
}


// --- View Management ---
EditorWindow* new_view() {
    EditorWindow* w = new EditorWindow(800, 600, "Untitled");
    windows.push_back(w); // Add to list of windows
    set_title(w); // Set initial title
    return w;
}

// --- Main Function ---
int main(int argc, char **argv) {
    // Enable undo for the shared buffers
    textbuf.canUndo(1);
    // Style buffer undo is complex to sync, disable it and rely on re-parse
    // stylebuf.canUndo(1);

    // Add global modify callbacks (only need to do this once)
    textbuf.add_modify_callback(style_update, nullptr); // Pass nullptr for cbArg
    textbuf.add_modify_callback(changed_cb, nullptr);   // Pass nullptr for cbArg

    // Create the first view
    EditorWindow* first_window = new_view();
    first_window->show(argc, argv); // Show the first window

    if (argc > 1) {
        load_file(argv[1]);
    } else {
         textbuf.call_modify_callbacks(); // Style the initial empty buffer
    }

    return Fl::run();
}

