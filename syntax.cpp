#include "syntax.h"
#include "globals.h" // Access to textbuf, stylebuf, windows vector
#include "EditorWindow.h" // Needed to call redisplay_range on editor

#include <FL/Fl_Text_Buffer.H>
#include <cstdlib> // For bsearch, free
#include <cstring> // For strncmp, memset, strlen
#include <cctype>  // For isalpha, isalnum

// --- Syntax Highlighting Data (Definitions) ---
Fl_Text_Display::Style_Table_Entry styletable[] = {
  { FL_BLACK,      FL_COURIER,        14 }, // A - Plain
  { FL_DARK_GREEN, FL_COURIER_ITALIC, 14 }, // B - Line comments (//)
  { FL_DARK_GREEN, FL_COURIER_ITALIC, 14 }, // C - Block comments (/* */)
  { FL_BLUE,       FL_COURIER,        14 }, // D - Strings ("...")
  { FL_DARK_RED,   FL_COURIER,        14 }, // E - Directives (#...)
  { FL_DARK_RED,   FL_COURIER_BOLD,   14 }, // F - Types (Fl_..., etc.)
  { FL_BLUE,       FL_COURIER_BOLD,   14 }  // G - Keywords (if, else, etc.)
};
// Define the table size variable here (removed const)
int styletable_size = sizeof(styletable) / sizeof(styletable[0]);

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
const int num_keywords = sizeof(code_keywords) / sizeof(code_keywords[0]);

const char *code_types[] = {
  "Fl_Widget", "Fl_Window", "Fl_Double_Window", "Fl_Gl_Window",
  "Fl_Button", "Fl_Input", "Fl_Text_Editor", "Fl_Text_Buffer",
  "Fl_Menu_Bar", "Fl_File_Chooser"
  // Add more C++ standard types or FLTK types if desired
  // "int", "char", "double", "float", "void", "bool", "long", "short", etc.
};
const int num_types = sizeof(code_types) / sizeof(code_types[0]);


// --- Syntax Highlighting Function Implementations ---

// Used by bsearch to compare keywords/types
int compare_keywords(const void *p1, const void *p2) {
  return strcmp(*(const char **)p1, *(const char **)p2);
}

// Parses text and generates corresponding style characters
void style_parse(const char *text, char *style, int length) {
  char             current_style_char;
  int              col;
  int              last_char_alnum;
  char             keyword_buf[255], *kbuf_ptr;
  const char       *text_ptr;
  int              i;
  const char       **found_keyword;

  if (!style || !text || length <= 0) return; // Safety check

  char* style_write_ptr = style; // Use a separate pointer for writing to style buffer
  current_style_char = *style_write_ptr; // Initial style context for the segment

  for (col = 0, last_char_alnum = 0; length > 0; length--, text++) {
      switch (current_style_char) {
          case 'A': // Default style
              if (col == 0 && *text == '#') current_style_char = 'E'; // Directive
              else if (strncmp(text, "//", 2) == 0) current_style_char = 'B'; // Line comment
              else if (strncmp(text, "/*", 2) == 0) current_style_char = 'C'; // Block comment start
              else if (strncmp(text, "\\\"", 2) == 0) { // Escaped quote
                  if (length >= 2) { *style_write_ptr++ = current_style_char; *style_write_ptr++ = current_style_char; text++; length--; col += 2; }
                  else { *style_write_ptr++ = current_style_char; col++; } // Handle single char at end
                  if (length <= 1) { length = 0; } continue; // Consume chars, continue loop
              } else if (*text == '\"') current_style_char = 'D'; // String start
              else if (!last_char_alnum && isalpha(*text)) { // Potential keyword/type start
                  for (text_ptr = text, kbuf_ptr = keyword_buf; (isalnum(*text_ptr) || *text_ptr == '_') && kbuf_ptr < (keyword_buf + sizeof(keyword_buf) - 1); *kbuf_ptr++ = *text_ptr++);
                  *kbuf_ptr = '\0'; kbuf_ptr = keyword_buf; char matched_style = 'A';
                  found_keyword = (const char **)bsearch(&kbuf_ptr, code_types, num_types, sizeof(code_types[0]), compare_keywords);
                  if (found_keyword != NULL) matched_style = 'F'; // Type
                  else {
                      found_keyword = (const char **)bsearch(&kbuf_ptr, code_keywords, num_keywords, sizeof(code_keywords[0]), compare_keywords);
                      if (found_keyword != NULL) matched_style = 'G'; // Keyword
                  }
                  if (matched_style != 'A') {
                      int keyword_len = (int)strlen(keyword_buf);
                      for (i = 0; i < keyword_len; i++) *style_write_ptr++ = matched_style;
                      text += keyword_len - 1; length -= keyword_len -1; col += keyword_len;
                      last_char_alnum = 1; current_style_char = 'A'; continue; // Consume keyword, continue loop
                  }
              }
              break; // End case 'A'

          case 'C': // Inside block comment
              if (strncmp(text, "*/", 2) == 0) { // Block comment end
                  if (length >= 2) { *style_write_ptr++ = current_style_char; *style_write_ptr++ = current_style_char; text++; length--; col += 2; current_style_char = 'A'; }
                  else { *style_write_ptr++ = current_style_char; col++; }
                  if (length <= 1) { length = 0; } continue; // Consume chars, continue loop
              }
              break; // End case 'C'

          case 'D': // Inside string literal
              if (strncmp(text, "\\\"", 2) == 0) { // Escaped quote
                  if (length >= 2) { *style_write_ptr++ = current_style_char; *style_write_ptr++ = current_style_char; text++; length--; col += 2; }
                  else { *style_write_ptr++ = current_style_char; col++; }
                  if (length <= 1) { length = 0; } continue; // Consume chars, continue loop
              } else if (*text == '\"') { // End of string
                  *style_write_ptr++ = current_style_char; col++; current_style_char = 'A'; continue;
              }
              break; // End case 'D'
      } // End switch

      // Write the determined style for the current character
      *style_write_ptr++ = current_style_char;
      col++;
      last_char_alnum = isalnum(*text) || *text == '_';

      // Handle end-of-line transitions
      if (*text == '\n') {
          col = 0;
          if (current_style_char == 'B' || current_style_char == 'E') current_style_char = 'A'; // Line comments/directives end
      }
  } // End for loop
}


// Updates the style buffer based on changes in the text buffer
void style_update(int pos, int nInserted, int nDeleted, int, const char*, void* /*cbArg*/) {
    int start, end;
    char *style = NULL;
    char *text = NULL;
    char last_style_before_parse = 'A';
    char last_style_after_parse = 'A';

    if (nInserted == 0 && nDeleted == 0) return; // Ignore selection-only changes

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
    start = textbuf.line_start(pos);
    end = textbuf.line_end(pos + nInserted);
    int text_len = textbuf.length();
    if (end > text_len) end = text_len;
    int style_len = stylebuf.length();
    int check_pos = start > 0 ? start - 1 : 0;

    // Determine the style context *before* the change start position
    char initial_style_context = 'A';
    if (check_pos < style_len) {
        initial_style_context = stylebuf.byte_at(check_pos);
        // If previous char suggests a C comment, scan backwards to find the actual start or end
        if (initial_style_context == 'C') {
            int comment_scan_pos = start;
            while (comment_scan_pos > 0) {
                // Check for '*/' ending a *previous* comment block before our change start
                if (comment_scan_pos >= 2 && textbuf.byte_at(comment_scan_pos - 1) == '/' && textbuf.byte_at(comment_scan_pos - 2) == '*') {
                    // We are likely starting in 'A' context after a comment ended
                    initial_style_context = 'A';
                    break;
                }
                // Check for '/*' starting the comment block we are inside
                if (comment_scan_pos >= 2 && textbuf.byte_at(comment_scan_pos - 1) == '*' && textbuf.byte_at(comment_scan_pos - 2) == '/') {
                    // Found the start; adjust parse range start and assume 'A' context before it
                    start = textbuf.line_start(comment_scan_pos - 2);
                    initial_style_context = 'A'; // Context before the /* is A
                    break;
                }
                comment_scan_pos--;
            }
            if (comment_scan_pos == 0) start = 0; // Scanned all the way back
        }
    }

    // --- Parse the primary affected range ---
    text = textbuf.text_range(start, end); if (!text) goto cleanup;
    style = stylebuf.text_range(start, end); if (!style) goto cleanup;

    // Get style at end *before* parsing this segment
    style_len = stylebuf.length(); // Update style_len after potential buffer changes
    if (end > start && end <= style_len) last_style_before_parse = stylebuf.byte_at(end - 1);
    else if (style_len > 0 && start == end && start > 0) last_style_before_parse = stylebuf.byte_at(start - 1);
    else last_style_before_parse = 'A';

    // Parse the segment, providing initial context
    if (end > start) style[0] = initial_style_context;
    style_parse(text, style, end - start);

    // Replace the style buffer segment
    stylebuf.replace(start, end, style);

    // Get style at end *after* parsing
    if (end > start) last_style_after_parse = style[end - start - 1];
    else last_style_after_parse = initial_style_context;

    // --- Check if reparsing the rest of the buffer is needed ---
    if (last_style_after_parse != last_style_before_parse || last_style_after_parse == 'C') {
        int rest_start = end; int rest_end = textbuf.length();
        if (rest_start < rest_end) {
            free(text); text = NULL; free(style); style = NULL;
            text = textbuf.text_range(rest_start, rest_end); if (!text) goto cleanup;
            style = stylebuf.text_range(rest_start, rest_end); if (!style) goto cleanup;

            // Parse the remainder, starting with the context from the previous section
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
        if (w && w->editor) { // Check if window and editor still exist
             w->editor->redisplay_range(start, end);
        }
    }
}

