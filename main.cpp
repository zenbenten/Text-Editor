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

// Global variables
int changed = 0;
int loading = 0;
char filename[256] = "";
Fl_Text_Buffer textbuf;

// Forward declarations
class EditorWindow;

// Callback Prototypes
void changed_cb(int, int, int, int, const char*, void*);
void copy_cb(Fl_Widget*, void*);
void cut_cb(Fl_Widget*, void*);
void delete_cb(Fl_Widget*, void*);
void find_cb(Fl_Widget*, void*);
void find2_cb(Fl_Widget*, void*);
void new_cb(Fl_Widget*, void*);
void open_cb(Fl_Widget*, void*);
void paste_cb(Fl_Widget*, void*);
void quit_cb(Fl_Widget*, void*);
void replace_cb(Fl_Widget*, void*);
void replace2_cb(Fl_Widget*, void*);
void replall_cb(Fl_Widget*, void*);
void replcan_cb(Fl_Widget*, void*);
void save_cb(Fl_Widget*, void*);
void saveas_cb(Fl_Widget*, void*);

// Helper Function Prototypes
void set_title(EditorWindow* w);
int check_save();
void load_file(const char *newfile, int ipos = -1);
void save_file(const char *newfile);


// EditorWindow Class Definition 
class EditorWindow : public Fl_Double_Window {
public:
    EditorWindow(int w, int h, const char* title);
    ~EditorWindow() {}

    Fl_Menu_Bar* menu;
    Fl_Text_Editor* editor;

    Fl_Window      *replace_dlg;
    Fl_Input       *replace_find;
    Fl_Input       *replace_with;
    Fl_Button      *replace_all;
    Fl_Return_Button *replace_next;
    Fl_Button      *replace_cancel;

    char search[256];
};

//Callback Implementations

void changed_cb(int, int nInserted, int nDeleted, int, const char*, void* v) {
    if ((nInserted || nDeleted) && !loading) changed = 1;
    EditorWindow *w = (EditorWindow *)v;
    set_title(w);
    if (loading) w->editor->show_insert_position();
}

void copy_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    Fl_Text_Editor::kf_copy(0, e->editor);
}

void cut_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    Fl_Text_Editor::kf_cut(0, e->editor);
}

void delete_cb(Fl_Widget*, void*) {
    textbuf.remove_selection();
}

void find_cb(Fl_Widget* w, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    const char *val = fl_input("Search String:", e->search);
    if (val != NULL) {
        strcpy(e->search, val);
        find2_cb(w, v);
    }
}

void find2_cb(Fl_Widget* w, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    if (e->search[0] == '\0') {
        find_cb(w, v);
        return;
    }

    int pos = e->editor->insert_position();
    int found_pos = pos;

    if (textbuf.search_forward(pos, e->search, &found_pos)) {
        textbuf.select(found_pos, found_pos + strlen(e->search));
        e->editor->insert_position(found_pos + strlen(e->search));
        e->editor->show_insert_position();
    }
    else {
        fl_alert("No more occurrences of \'%s\' found!", e->search);
    }
}

void new_cb(Fl_Widget*, void* v) {
    if (!check_save()) return;
    filename[0] = '\0';
    textbuf.select(0, textbuf.length());
    textbuf.remove_selection();
    changed = 0;
    textbuf.call_modify_callbacks();
}

void open_cb(Fl_Widget*, void* v) {
    if (!check_save()) return;
    char *newfile = fl_file_chooser("Open File?", "*", filename);
    if (newfile != NULL) {
        load_file(newfile);
    }
}

void paste_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    Fl_Text_Editor::kf_paste(0, e->editor);
}

void quit_cb(Fl_Widget*, void*) {
    if (changed && !check_save()) {
        return;
    }
    exit(0);
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
    const char *find = e->replace_find->value();
    const char *replace = e->replace_with->value();

    if (find[0] == '\0') return;

    e->replace_dlg->hide();

    int times = 0;
    int pos = 0;
    int found_pos = 0;

    while(textbuf.search_forward(pos, find, &found_pos)) {
        textbuf.remove(found_pos, found_pos + strlen(find));
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
    e->replace_dlg->hide();
}

void save_cb(Fl_Widget* w, void* v) {
    if (filename[0] == '\0') {
        saveas_cb(w, v);
    } else {
        save_file(filename);
    }
}

void saveas_cb(Fl_Widget*, void*) {
    char *newfile = fl_file_chooser("Save File As?", "*", filename);
    if (newfile != NULL) {
        save_file(newfile);
    }
}


//Helper Functions

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
        title += " (modified)";
    }
    w->copy_label(title.c_str());
}

int check_save() {
    if (!changed) return 1;

    int r = fl_choice("The document has been modified.",
                      "Cancel",
                      "Save",
                      "Discard");

    if (r == 1) {
        save_cb(nullptr, nullptr);
        return !changed;
    }

    return (r != 0);
}

void load_file(const char *newfile, int ipos) {
    loading = 1;
    int insert = (ipos != -1);
    if (!insert) strcpy(filename, "");

    if (textbuf.loadfile(newfile)) {
        fl_alert("Error reading from file \'%s\':\n%s.", newfile, strerror(errno));
    } else {
        if (!insert) strcpy(filename, newfile);
    }
    changed = insert;
    loading = 0;
    textbuf.call_modify_callbacks();
}

void save_file(const char *newfile) {
    if (textbuf.savefile(newfile)) {
        fl_alert("Error writing to file \'%s\':\n%s.", newfile, strerror(errno));
    } else {
        strcpy(filename, newfile);
        changed = 0;
    }
    textbuf.call_modify_callbacks();
}


// EditorWindow Implementation 

EditorWindow::EditorWindow(int W, int H, const char* title)
    : Fl_Double_Window(W, H, title) {

    search[0] = '\0';

    menu = new Fl_Menu_Bar(0, 0, W, 30);

    Fl_Menu_Item menuitems[] = {
        { "&File",              0, 0, 0, FL_SUBMENU },
            { "&New File",      FL_CTRL | 'n', (Fl_Callback *)new_cb, this },
            { "&Open File...",  FL_CTRL | 'o', (Fl_Callback *)open_cb, this },
            { "&Save File",     FL_CTRL | 's', (Fl_Callback *)save_cb, this },
            { "Save File &As...", FL_CTRL | FL_SHIFT | 's', (Fl_Callback *)saveas_cb, this, FL_MENU_DIVIDER },
            { "E&xit",          FL_CTRL | 'q', (Fl_Callback *)quit_cb, this },
            { 0 },
        { "&Edit",              0, 0, 0, FL_SUBMENU },
            { "Cu&t",           FL_CTRL | 'x', (Fl_Callback *)cut_cb, this },
            { "&Copy",          FL_CTRL | 'c', (Fl_Callback *)copy_cb, this },
            { "&Paste",         FL_CTRL | 'v', (Fl_Callback *)paste_cb, this },
            { "&Delete",        0,             (Fl_Callback *)delete_cb, this },
            { 0 },
        { "&Search",            0, 0, 0, FL_SUBMENU },
            { "&Find...",       FL_CTRL | 'f', (Fl_Callback *)find_cb, this },
            { "&Replace...",    FL_CTRL | 'r', (Fl_Callback *)replace_cb, this },
            { 0 },
        { 0 }
    };

    menu->copy(menuitems);

    editor = new Fl_Text_Editor(0, 30, W, H - 30);
    editor->buffer(&textbuf);
    editor->textfont(FL_COURIER);
    editor->textsize(14);

    this->resizable(editor);
    this->size_range(300, 200);

    textbuf.add_modify_callback(changed_cb, this);
    textbuf.call_modify_callbacks();

    replace_dlg = new Fl_Window(300, 105, "Replace");
    {
        replace_find = new Fl_Input(70, 10, 200, 25, "Find:");
        replace_with = new Fl_Input(70, 40, 200, 25, "Replace:");
        replace_all = new Fl_Button(10, 70, 90, 25, "Replace All");
        replace_next = new Fl_Return_Button(105, 70, 120, 25, "Replace Next");
        replace_cancel = new Fl_Button(230, 70, 60, 25, "Cancel");

        replace_all->callback(replall_cb, this);
        replace_next->callback(replace2_cb, this);
        replace_cancel->callback(replcan_cb, this);
    }
    replace_dlg->end();
    replace_dlg->set_non_modal();

    this->callback((Fl_Callback *)quit_cb, this);
}


int main(int argc, char **argv) {
    EditorWindow win(800, 600, "Untitled");

    win.show(argc, argv);

    if (argc > 1) {
        load_file(argv[1]);
    }

    return Fl::run();
}

