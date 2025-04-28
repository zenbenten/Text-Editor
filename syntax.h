#ifndef SYNTAX_H
#define SYNTAX_H

#include <FL/Fl_Text_Display.H> // For Style_Table_Entry

// --- Syntax Highlighting Data (Declarations) ---
// Defined in syntax.cpp
extern Fl_Text_Display::Style_Table_Entry styletable[];
extern int styletable_size; // Declaration for the table size (removed const)
extern const char *code_keywords[];
extern const int num_keywords;
extern const char *code_types[];
extern const int num_types;

// --- Syntax Highlighting Function Declarations ---
void style_parse(const char *text, char *style, int length);
void style_update(int pos, int nInserted, int nDeleted, int nRestyled, const char *deletedText, void *cbArg);
int compare_keywords(const void *p1, const void *p2); // Used by bsearch

#endif // SYNTAX_H

