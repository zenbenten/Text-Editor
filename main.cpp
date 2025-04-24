#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Text_Buffer.H>

Fl_Text_Buffer* textbuf = new Fl_Text_Buffer;

class EditorWindow : public Fl_Double_Window {
public:
    EditorWindow(int w, int h, const char* title);
    
    Fl_Menu_Bar* menu;
    Fl_Text_Editor* editor;
};

EditorWindow::EditorWindow(int w, int h, const char* title) 
    : Fl_Double_Window(w, h, title) {
    
    menu = new Fl_Menu_Bar(0, 0, w, 25);
    
    editor = new Fl_Text_Editor(0, 25, w, h-25);
    editor->buffer(textbuf);
    editor->textfont(FL_COURIER);
    
    Fl_Menu_Item menuitems[] = {
        { "&File", 0, 0, 0, FL_SUBMENU },
            { "&New", 0, 0 },
            { "&Open...", 0, 0 },
            { "&Save", 0, 0 },
            { "Save &As...", 0, 0 },
            { 0 },
        { "&Edit", 0, 0, 0, FL_SUBMENU },
            { "&Undo", 0, 0 },
            { "&Redo", 0, 0 },
            { 0 },
        { 0 }
    };
    
    menu->copy(menuitems);
}

int main(int argc, char** argv) {
    EditorWindow win(800, 600, "FLTK Text Editor");
    
    win.show(argc, argv);
    
    return Fl::run();
}
