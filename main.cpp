#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

Fl_Text_Buffer* textbuf = new Fl_Text_Buffer;
int            changed = 0;
char           filename[256] = "";
char           title[256];
int            loading = 0;
char           search[256] = "";

void new_cb(Fl_Widget*, void*);
void open_cb(Fl_Widget*, void*);
void insert_cb(Fl_Widget*, void*);
void save_cb(Fl_Widget*, void*);
void saveas_cb(Fl_Widget*, void*);
void view_cb(Fl_Widget*, void*);
void close_cb(Fl_Widget*, void*);
void quit_cb(Fl_Widget*, void*);
void undo_cb(Fl_Widget*, void*);
void cut_cb(Fl_Widget*, void*);
void copy_cb(Fl_Widget*, void*);
void paste_cb(Fl_Widget*, void*);
void delete_cb(Fl_Widget*, void*);
void find_cb(Fl_Widget*, void*);
void find2_cb(Fl_Widget*, void*);
void replace_cb(Fl_Widget*, void*);
void replace2_cb(Fl_Widget*, void*);
void set_title(Fl_Window* w);
int check_save(void);
void load_file(char *newfile, int ipos);
void save_file(char *newfile);
void changed_cb(int, int, int, int, const char*, void*);

class EditorWindow : public Fl_Double_Window {
public:
    EditorWindow(int w, int h, const char* title);
    ~EditorWindow();

    Fl_Menu_Bar* menu;
    Fl_Text_Editor* editor;
    Fl_Window        *replace_dlg;
    Fl_Input         *replace_find;
    Fl_Input         *replace_with;
    Fl_Button        *replace_all;
    Fl_Return_Button *replace_next;
    Fl_Button        *replace_cancel;
};

EditorWindow::EditorWindow(int w, int h, const char* t)
    : Fl_Double_Window(w, h, t) {

    replace_dlg = 0;
    replace_find = 0;
    replace_with = 0;
    replace_all = 0;
    replace_next = 0;
    replace_cancel = 0;

    menu = new Fl_Menu_Bar(0, 0, w, 25);

    editor = new Fl_Text_Editor(0, 25, w, h - 25);
    editor->buffer(textbuf);
    editor->textfont(FL_COURIER);

    Fl_Menu_Item menuitems[] = {
      { "&File",              0, 0, 0, FL_SUBMENU },
        { "&New File",        0, (Fl_Callback *)new_cb, this },
        { "&Open File...",    FL_CTRL + 'o', (Fl_Callback *)open_cb, this },
        { "&Insert File...",  FL_CTRL + 'i', (Fl_Callback *)insert_cb, this, FL_MENU_DIVIDER },
        { "&Save File",       FL_CTRL + 's', (Fl_Callback *)save_cb, this },
        { "Save File &As...", FL_CTRL + FL_SHIFT + 's', (Fl_Callback *)saveas_cb, this, FL_MENU_DIVIDER },
        { "New &View",        FL_ALT + 'v', (Fl_Callback *)view_cb, this },
        { "&Close View",      FL_CTRL + 'w', (Fl_Callback *)close_cb, this, FL_MENU_DIVIDER },
        { "E&xit",            FL_CTRL + 'q', (Fl_Callback *)quit_cb, this },
        { 0 },

      { "&Edit", 0, 0, 0, FL_SUBMENU },
        { "&Undo",       FL_CTRL + 'z', (Fl_Callback *)undo_cb, this, FL_MENU_DIVIDER },
        { "Cu&t",        FL_CTRL + 'x', (Fl_Callback *)cut_cb, this },
        { "&Copy",       FL_CTRL + 'c', (Fl_Callback *)copy_cb, this },
        { "&Paste",      FL_CTRL + 'v', (Fl_Callback *)paste_cb, this },
        { "&Delete",     0, (Fl_Callback *)delete_cb, this },
        { 0 },

      { "&Search", 0, 0, 0, FL_SUBMENU },
        { "&Find...",       FL_CTRL + 'f', (Fl_Callback *)find_cb, this },
        { "F&ind Again",    FL_CTRL + 'g', (Fl_Callback *)find2_cb, this },
        { "&Replace...",    FL_CTRL + 'r', (Fl_Callback *)replace_cb, this },
        { "Re&place Again", FL_CTRL + 't', (Fl_Callback *)replace2_cb, this },
        { 0 },

      { 0 }
    };

    menu->copy(menuitems);

    textbuf->add_modify_callback(changed_cb, this);
    textbuf->call_modify_callbacks();

    end();
    resizable(editor);
}

EditorWindow::~EditorWindow() {
    delete replace_dlg;
}

int main(int argc, char** argv) {
    EditorWindow* win = new EditorWindow(640, 400, "FLTK Simple Editor");

    win->show(argc, argv);

    if (argc > 1) {
        load_file(argv[1], -1);
    } else {
        set_title(win);
    }

    return Fl::run();
}

void new_cb(Fl_Widget* w, void* v) { printf("New File callback triggered.\n"); }
void open_cb(Fl_Widget* w, void* v) { printf("Open File callback triggered.\n"); }
void insert_cb(Fl_Widget* w, void* v) { printf("Insert File callback triggered.\n"); }
void save_cb(Fl_Widget* w, void* v) { printf("Save File callback triggered.\n"); }
void saveas_cb(Fl_Widget* w, void* v) { printf("Save As callback triggered.\n"); }
void view_cb(Fl_Widget* w, void* v) { printf("New View callback triggered.\n"); }
void close_cb(Fl_Widget* w, void* v) {
    EditorWindow* win = (EditorWindow*)v;
    printf("Close View callback triggered.\n");
    if (win->shown()) {
        win->hide();
    }
    quit_cb(w, v);
}
void quit_cb(Fl_Widget* w, void* v) {
    printf("Exit callback triggered.\n");
    if (changed && !check_save()) return;
    exit(0);
}
void undo_cb(Fl_Widget* w, void* v) { printf("Undo callback triggered.\n"); }
void cut_cb(Fl_Widget* w, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    printf("Cut callback triggered.\n");
    Fl_Text_Editor::kf_cut(0, e->editor);
}
void copy_cb(Fl_Widget* w, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    printf("Copy callback triggered.\n");
    Fl_Text_Editor::kf_copy(0, e->editor);
}
void paste_cb(Fl_Widget* w, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    printf("Paste callback triggered.\n");
    Fl_Text_Editor::kf_paste(0, e->editor);
}
void delete_cb(Fl_Widget* w, void* v) {
    printf("Delete callback triggered.\n");
    textbuf->remove_selection();
}
void find_cb(Fl_Widget* w, void* v) { printf("Find callback triggered.\n"); }
void find2_cb(Fl_Widget* w, void* v) { printf("Find Again callback triggered.\n"); }
void replace_cb(Fl_Widget* w, void* v) { printf("Replace callback triggered.\n"); }
void replace2_cb(Fl_Widget* w, void* v) { printf("Replace Again callback triggered.\n"); }

void set_title(Fl_Window* w) {
    if (filename[0] == '\0') strcpy(title, "Untitled");
    else {
        char *slash;
        slash = strrchr(filename, '/');
#ifdef WIN32
        if (slash == NULL) slash = strrchr(filename, '\\');
#endif
        if (slash != NULL) strcpy(title, slash + 1);
        else strcpy(title, filename);
    }
    if (changed) strcat(title, " (modified)");
    w->label(title);
}

int check_save(void) {
    printf("check_save called.\n");
    if (!changed) return 1;
    int r = fl_choice("The current file has not been saved.\n"
                      "Would you like to save it now?",
                      "Cancel", "Save", "Discard");
    if (r == 1) {
        printf("Simulating call to save logic...\n");
        if (filename[0] == '\0') {
            printf("Need to call Save As...\n");
            return 0;
        } else {
            printf("Need to implement save_file()...\n");
            return 0;
        }
    } else if (r == 2) {
        return 1;
    } else {
        return 0;
    }
}

void load_file(char *newfile, int ipos) {
    printf("load_file called for: %s\n", newfile);
    loading = 1;
    int insert = (ipos != -1);
    if (!insert) strcpy(filename, "");
    int r;
    printf("Attempting to load '%s'...\n", newfile);
    if (!insert) r = textbuf->loadfile(newfile);
    else r = textbuf->insertfile(newfile, ipos);
    if (r) {
        fl_alert("Error reading from file '%s':\n%s.", newfile, strerror(errno));
    } else {
        if (!insert) strcpy(filename, newfile);
        changed = insert;
    }
    loading = 0;
    textbuf->call_modify_callbacks();
    printf("Load finished.\n");
}

void save_file(char *newfile) {
    printf("save_file called for: %s\n", newfile);
    if (textbuf->savefile(newfile)) {
        fl_alert("Error writing to file '%s':\n%s.", newfile, strerror(errno));
    } else {
        strcpy(filename, newfile);
        changed = 0;
    }
    Fl_Window* w = Fl::first_window();
    if (w) set_title(w);
    printf("Save finished.\n");
}

void changed_cb(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* v) {
    EditorWindow *w = (EditorWindow *)v;
    if ((nInserted || nDeleted) && !loading) {
        changed = 1;
    }
    set_title(w);
}

