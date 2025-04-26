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

int changed = 0;
int loading = 0;
char filename[256] = "";
Fl_Text_Buffer textbuf;
Fl_Text_Buffer stylebuf;

Fl_Text_Display::Style_Table_Entry styletable[] = {
  { FL_BLACK,      FL_COURIER,        14 },
  { FL_DARK_GREEN, FL_COURIER_ITALIC, 14 },
  { FL_DARK_GREEN, FL_COURIER_ITALIC, 14 },
  { FL_BLUE,       FL_COURIER,        14 },
  { FL_DARK_RED,   FL_COURIER,        14 },
  { FL_DARK_RED,   FL_COURIER_BOLD,   14 },
  { FL_BLUE,       FL_COURIER_BOLD,   14 }
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

class EditorWindow;

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

void set_title(EditorWindow* w);
int check_save();
void load_file(const char *newfile, int ipos = -1);
void save_file(const char *newfile);

void style_parse(const char *text, char *style, int length);
void style_update(int pos, int nInserted, int nDeleted, int nRestyled, const char *deletedText, void *cbArg);
int compare_keywords(const void *p1, const void *p2);

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
    stylebuf.select(0, stylebuf.length());
    stylebuf.remove_selection();
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

    int r;
    if (!insert) r = textbuf.loadfile(newfile);
    else r = textbuf.insertfile(newfile, ipos);

    if (r) {
        fl_alert("Error reading from file \'%s\':\n%s.", newfile, strerror(errno));
    } else {
        if (!insert) strcpy(filename, newfile);
    }
    changed = insert;
    loading = 0;
    textbuf.call_modify_callbacks();

    stylebuf.select(0, stylebuf.length());
    stylebuf.remove_selection();
    char* text = textbuf.text();
    char* styles = new char[textbuf.length() + 1];
    memset(styles, 'A', textbuf.length());
    styles[textbuf.length()] = '\0';
    style_parse(text, styles, textbuf.length());
    stylebuf.text(styles);
    free(text);
    delete[] styles;
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

  for (current = *style, col = 0, last = 0; length > 0; length --, text ++) {
    if (current == 'A') {
      if (col == 0 && *text == '#') {
        current = 'E';
      } else if (strncmp(text, "//", 2) == 0) {
        current = 'B';
        for (i = 0; i < 2; i++) {
          if (length > 0) {
            *style++ = current; text++; length--; col++;
          }
        }
        if (length == 0) break;
      } else if (strncmp(text, "/*", 2) == 0) {
        current = 'C';
        for (i = 0; i < 2; i++) {
          if (length > 0) {
            *style++ = current; text++; length--; col++;
          }
        }
         if (length == 0) break;
     } else if (strncmp(text, "\\\"", 2) == 0) {
        *style++ = current;
        *style++ = current;
        text ++;
        length --;
        col += 2;
        if (length == 0) break;
        continue;
      } else if (*text == '\"') {
        current = 'D';
      } else if (!last && isalpha(*text)) {
        for (temp = text, bufptr = buf;
             (isalnum(*temp) || *temp == '_') && bufptr < (buf + sizeof(buf) - 1);
             *bufptr++ = *temp++);
        *bufptr = '\0';

        bufptr = buf;

        keyword = (const char **)bsearch(&bufptr, code_types,
                      sizeof(code_types) / sizeof(code_types[0]),
                      sizeof(code_types[0]), compare_keywords);

        if (keyword != NULL) {
          current = 'F';
        } else {
          keyword = (const char **)bsearch(&bufptr, code_keywords,
                           sizeof(code_keywords) / sizeof(code_keywords[0]),
                           sizeof(code_keywords[0]), compare_keywords);
          if (keyword != NULL) {
             current = 'G';
          }
        }

        if (current == 'F' || current == 'G') {
            int keyword_len = (int)strlen(buf);
            for (i = 0; i < keyword_len; i++) {
                 *style++ = current;
            }
            text += keyword_len - 1;
            length -= keyword_len -1;
            col += keyword_len;
            last = 1;
            current = 'A';
            continue;
        }
      }
    } else if (current == 'C' && strncmp(text, "*/", 2) == 0) {
      *style++ = current;
      *style++ = current;
      text ++;
      length --;
      current = 'A';
      col += 2;
      if (length == 0) break;
      continue;
    } else if (current == 'D') {
      if (strncmp(text, "\\\"", 2) == 0) {
        *style++ = current;
        *style++ = current;
        text ++;
        length --;
        col += 2;
        if (length == 0) break;
        continue;
      } else if (*text == '\"') {
        *style++ = current;
        col ++;
        current = 'A';
        continue;
      }
    }

    *style++ = current;
    col ++;

    last = isalnum(*text) || *text == '_';

    if (*text == '\n') {
      col = 0;
      if (current == 'B' || current == 'E') current = 'A';
    }
  }
}

void style_update(int pos, int nInserted, int nDeleted, int, const char*, void *cbArg) {
  int start, end;
  char *style = NULL;
  char *text = NULL;

  if (nInserted == 0 && nDeleted == 0) {
    stylebuf.unselect();
    return;
  }

  if (nInserted > 0) {
    style = new char[nInserted + 1];
    memset(style, 'A', nInserted);
    style[nInserted] = '\0';
    stylebuf.replace(pos, pos + nDeleted, style);
    delete[] style;
    style = NULL;
  } else {
    stylebuf.remove(pos, pos + nDeleted);
  }

  stylebuf.select(pos, pos + nInserted - nDeleted);

  start = textbuf.line_start(pos);
  end = textbuf.line_end(pos + nInserted);

  int text_len = textbuf.length();
  if (end > text_len) end = text_len;

  int current_style_char_pos = stylebuf.length() > start ? start : stylebuf.length() -1;
  if (current_style_char_pos < 0) current_style_char_pos = 0;
  char current_style_char = stylebuf.length() > 0 ? stylebuf.character(current_style_char_pos) : 'A';

  if (current_style_char == 'C') {
      int comment_start = start;
      while (comment_start > 0) {
          if (stylebuf.character(comment_start - 1) == 'C' &&
              textbuf.character(comment_start - 1) == '*' &&
              textbuf.character(comment_start - 2) == '/') {
               break;
           }
           comment_start--;
      }
      start = textbuf.line_start(comment_start);
  }

  text = textbuf.text_range(start, end);
  if (!text) goto cleanup;

  style = stylebuf.text_range(start, end);
  if (!style) goto cleanup;

  char last_style_before_parse = (end > start && end <= stylebuf.length()) ? stylebuf.character(end - 1) : 'A';

  style_parse(text, style, end - start);

  stylebuf.replace(start, end, style);
  ((Fl_Text_Editor *)cbArg)->redisplay_range(start, end);

  char last_style_after_parse = (end > start) ? style[end - start - 1] : 'A';

  if (last_style_after_parse != last_style_before_parse || last_style_after_parse == 'C') {
      int rest_start = end;
      int rest_end = textbuf.length();
      if (rest_start < rest_end) {
          free(text); text = NULL;
          free(style); style = NULL;

          text = textbuf.text_range(rest_start, rest_end);
           if (!text) goto cleanup;

          style = stylebuf.text_range(rest_start, rest_end);
           if (!style) goto cleanup;

          style_parse(text, style, rest_end - rest_start);

          stylebuf.replace(rest_start, rest_end, style);
          ((Fl_Text_Editor *)cbArg)->redisplay_range(rest_start, rest_end);
      }
  }

cleanup:
  free(text);
  free(style);
}

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
    editor->textfont(styletable[0].font);
    editor->textsize(styletable[0].size);

    editor->highlight_data(&stylebuf, styletable,
                           sizeof(styletable) / sizeof(styletable[0]),
                           'A', 0, 0);

    this->resizable(editor);
    this->size_range(300, 200);

    textbuf.add_modify_callback(style_update, editor);
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
    } else {
         textbuf.call_modify_callbacks();
    }

    return Fl::run();
}
