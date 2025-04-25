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

// Global variables
int changed = 0;
char filename[256] = "";
Fl_Text_Buffer *textbuf = new Fl_Text_Buffer;

// Forward declarations
class EditorWindow;
void new_cb(Fl_Widget*, void*);
void open_cb(Fl_Widget*, void*);
void save_cb(Fl_Widget*, void*);
void saveas_cb(Fl_Widget*, void*);
void quit_cb(Fl_Widget*, void*);
void find_cb(Fl_Widget*, void*);
void replace_cb(Fl_Widget*, void*);
void replace2_cb(Fl_Widget*, void*);
void replall_cb(Fl_Widget*, void*);
void replcan_cb(Fl_Widget*, void*);

class EditorWindow : public Fl_Double_Window {
public:
    EditorWindow(int w, int h, const char* title);
    
    Fl_Menu_Bar* menu;
    Fl_Text_Editor* editor;
    
    // Replace dialog components
    Fl_Window      *replace_dlg;
    Fl_Input       *replace_find;
    Fl_Input       *replace_with;
    Fl_Button      *replace_all;
    Fl_Return_Button *replace_next;
    Fl_Button      *replace_cancel;
};

EditorWindow::EditorWindow(int w, int h, const char* title) 
    : Fl_Double_Window(w, h, title) {
    
    // Create menu bar
    menu = new Fl_Menu_Bar(0, 0, w, 30);
    
    // Create text editor
    editor = new Fl_Text_Editor(0, 30, w, h-30);
    editor->buffer(textbuf);
    editor->textfont(FL_COURIER);
    this->resizable(editor);

    // Menu structure
    Fl_Menu_Item menuitems[] = {
        { "&File", 0, 0, 0, FL_SUBMENU },
            { "&New File",        0, (Fl_Callback *)new_cb },
            { "&Open File...",    FL_CTRL+'o', (Fl_Callback *)open_cb },
            { "&Save File",       FL_CTRL+'s', (Fl_Callback *)save_cb },
            { "Save File &As...", FL_CTRL+FL_SHIFT+'s', (Fl_Callback *)saveas_cb, 0, FL_MENU_DIVIDER },
            { "E&xit", FL_CTRL+'q', (Fl_Callback *)quit_cb, 0 },
            { 0 },
        { "&Edit", 0, 0, 0, FL_SUBMENU },
            { "Cu&t",        FL_CTRL+'x', (Fl_Callback *)Fl_Text_Editor::kf_cut },
            { "&Copy",       FL_CTRL+'c', (Fl_Callback *)Fl_Text_Editor::kf_copy },
            { "&Paste",      FL_CTRL+'v', (Fl_Callback *)Fl_Text_Editor::kf_paste },
            { 0 },
        { "&Search", 0, 0, 0, FL_SUBMENU },
            { "&Find...",       FL_CTRL+'f', (Fl_Callback *)find_cb },
            { "&Replace...",    FL_CTRL+'r', (Fl_Callback *)replace_cb },
            { 0 },
        { 0 }
    };
    
    menu->copy(menuitems);

    // Create replace dialog
    replace_dlg = new Fl_Window(300, 105, "Replace");
    replace_find = new Fl_Input(70, 10, 200, 25, "Find:");
    replace_with = new Fl_Input(70, 40, 200, 25, "Replace:");
    replace_all = new Fl_Button(10, 70, 90, 25, "Replace All");
    replace_next = new Fl_Return_Button(105, 70, 120, 25, "Replace Next");
    replace_cancel = new Fl_Button(230, 70, 60, 25, "Cancel");
    replace_dlg->end();
    replace_dlg->hide();
    
    // Connect dialog callbacks
    replace_all->callback(replall_cb, this);
    replace_next->callback(replace2_cb, this);
    replace_cancel->callback(replcan_cb, this);
}

// Callback implementations
void new_cb(Fl_Widget*, void* v) {
    if (!changed) return;
    // Add save check logic here
    textbuf->select(0, textbuf->length());
    textbuf->remove_selection();
    filename[0] = '\0';
    changed = 0;
}

void open_cb(Fl_Widget*, void* v) {
    char *file = fl_file_chooser("Open File", "*", filename);
    if (file) {
        if (textbuf->loadfile(file)) fl_alert("Error loading %s", file);
        else strcpy(filename, file);
    }
}

void save_cb(Fl_Widget*, void* v) {
    if (filename[0]) textbuf->savefile(filename);
    else saveas_cb(0, v);
}

void saveas_cb(Fl_Widget*, void* v) {
    char *file = fl_file_chooser("Save File As", "*", filename);
    if (file) textbuf->savefile(file);
}

void quit_cb(Fl_Widget*, void* v) {
    exit(0);
}

void find_cb(Fl_Widget*, void* v) {
    // Implement find functionality
}

void replace_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    e->replace_dlg->show();
}

void replace2_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    const char *find = e->replace_find->value();
    const char *replace = e->replace_with->value();
    
    if (find[0] == '\0') {
        e->replace_dlg->show();
        return;
    }
    
    // Basic replace implementation
    int pos = e->editor->insert_position();
    if (textbuf->search_forward(pos, find, &pos)) {
        textbuf->replace(pos, pos+strlen(find), replace);
    }
}

void replall_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    const char *find = e->replace_find->value();
    const char *replace = e->replace_with->value();
    
    if (find[0] == '\0') return;
    
    textbuf->replace(0, textbuf->length(), 
        str_replace(textbuf->text(), find, replace).c_str());
}

void replcan_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    e->replace_dlg->hide();
}

int main(int argc, char** argv) {
    EditorWindow win(800, 600, "FLTK Text Editor");
    win.show(argc, argv);
    return Fl::run();
}
