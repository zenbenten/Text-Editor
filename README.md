A FLTK-based text editor implemented in C++. It provides a minimal GUI for editing plain text files with a menu bar for common actions. Supports standard file and edit operations like new, open, save, cut/copy/paste etc. Includes search and replace dialogs. Supports change tracking for unsaved edits. It also adds features like multi-window support for multiple “views” of the same document, an Insert File command, and basic undo functionality.
Technologies Used
C++: The core logic and UI.
FLTK (Fast Light Toolkit): A cross-platform C++ GUI toolkit. All windows, buttons, menus, text editor widgets, and dialogs are from FLTK.
Standard Libraries: Uses standard C libraries for file I/O and string handling (<stdio.h>, <string.h>, etc.), and <FL/fl_ask.H> for alert dialogs.
Known Limitations
Basic functionality: This is a simple editor intended for plain text. It does not support multiple documents in one window or advanced text features (no tabs, no spell-check, etc.).
Undo support: Only basic undo is provided. There is no multi-level redo or history beyond the most recent change.
Error handling: The editor assumes valid text files. Open very large/non-text files at your own risk.
