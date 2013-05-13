Description
===========

node-ncurses is an ncurses binding for [node.js](http://nodejs.org/).

**Note:** Mac OSX users may find themselves encountering bizarre output on their terminals in some situations. This is a known problem and basically requires more frequent Window.refresh()'ing to get everything to display properly.


Requirements
============

* [node.js](http://nodejs.org/) -- v0.8.0+


Terminology
===========

Functions that accept window/screen coordinates use a "row, column" format.

Also, **stdscr** is the name of the first window that is created and fills the terminal/screen by default. It cannot be resized or moved, it is always the bottom-most window, and it is the window you get when you first create a new **Window**.


API Documentation
=================

node-ncurses exposes only one class: **Window**.

#### Special data types

* _Result_ is simply an integer indicating success or failure of the function. 0 for success or -1 for error.

* _Attributes_ is an (unsigned) integer used as a bitmask for holding window attributes. All available attributes are stored in the 'attrs' property of the module.

* _ACS\_Character_ is a special character used when dealing with line graphics. These are automatically determined at runtime by ncurses and thus cannot be defined as constants. Instead, they are accessible statically via the Window class after at least one window has been created (so that ncurses has initialized this special character set). See the **Additional notes** at the bottom for a list of the available characters.
    * An _ACS\_Character_ can currently be retrieved by using the 'ACS' property of the module. Example:

```javascript
      var nc = require('ncurses'),
          win = new Window();
      win.hline(nc.cols, nc.ACS.DIAMOND);
```


Module Functions
---------------- 

* **colorPair**(< _Integer_ >colorPair, < _Integer_ >fgColor, < _Integer_ >bgColor) - _Integer_ - Sets the foreground and background colors for the given color pair number. The color pair number is always returned.

* **colorFg**(< _Integer_ >colorPair) - _Integer_ - Returns the foreground color currently set for the given color pair number.

* **colorBg**(< _Integer_ >colorPair) - _Integer_ - Returns the background color currently set for the given color pair number.

* **setEscDelay**(< _Integer_ >delay) - _(void)_ - Sets the duration (in milliseconds) to wait after ESC is pressed.

* **cleanup**() - _(void)_ - Restores the terminal after using ncurses. This function is automatically called when the last window is closed and thus should never be used except when handling unexpected exceptions (i.e. in node.js's uncaughtException event) so that you can safely restore the terminal back to normal.

* **redraw**() - _(void)_ - Redraws all windows.

* **flash**() - _(void)_ - Flashes the screen.

* **bell**() - _(void)_ - Sounds the terminal bell.

* **leave**() - _(void)_ - Saves the current state of ncurses in memory and exits ncurses.

* **restore**() - _(void)_ - Restores the state saved by **leave** and updates the screen appropriately.


Module Properties
-----------------

* **ACS** - _Object_ [Read-only] - Contains a map of all of the available ACS characters keyed by their character names

* **attrs** - _Object_ [Read-only] - Contains all of the available terminal attributes:
    * _NORMAL_ - normal display
    * _STANDOUT_ - "best" highlighting mode of the terminal
    * _UNDERLINE_ - underlining
    * _REVERSE_ - reverses the foreground and background colors
    * _BLINK_ - blinking
    * _DIM_ - half bright
    * _BOLD_ - extra bright or bold
    * _INVISIBLE_ - invisible or blank mode
    * _PROTECT_ - protected mode (unsure of the real behavior for this one)

* **colors** - _Object_ [Read-only] - Contains a map of names to the basic 8 ANSI colors:
    * _BLACK_
    * _RED_
    * _GREEN_
    * _YELLOW_
    * _BLUE_
    * _MAGENTA_
    * _CYAN_
    * _WHITE_
    * Additional colors (if available) can be accessed by using their numerical value instead (the above list is 0 through 7 and 8 through 15 -- if available -- are generally the bright/bold versions of the above list)

* **keys** - _Object_ [Read-only] - Contains a map of keys on the keyboard (this is a long list -- see "Additional Notes" for all mapped keys)

* **numwins** - _Integer_ [Read-only] - Returns the number of currently open windows.

* **lines** - _Integer_ [Read-only] - The total number of rows of the terminal

* **cols** - _Integer_ [Read-only] - The total number of columns of the terminal

* **tabsize** - _Integer_ [Read-only] - The terminal's tab size

* **hasMouse** - _Boolean_ [Read-only] - Indicates whether the terminal supports mouse (clicks) functionality

* **hasColors** - _Boolean_ [Read-only] - Indicates whether the terminal supports colors

* **numColors** - _Integer_ [Read-only] - Indicates the (maximum) number of colors the terminal supports colors

* **maxColorPairs** - _Integer_ [Read-only] - The maximum number of foreground-background color pairs supported by the terminal

* **echo** - _Boolean_ [Read/Write] - Enable/disable local echoing of keyboard input

* **showCursor** - _Boolean_ [Read/Write] - Show/hide the cursor

* **raw** - _Boolean_ [Read/Write] - Enable/disable terminal's raw mode


Window Events
-------------

* **inputChar**(< _String_ >char, < _Integer_ >charCode, < _Boolean_ >isKey) - Emitted when a key has been pressed. isKey is true when a non-character key has been presssed (e.g. ESC, arrow key, Page Up, etc.).


Window Functions
----------------

* **clearok**(< _Boolean_ >clearOnRefresh) - _Result_ - If redraw is _true_, the next call to **refresh**() will clear the screen completely and redraw the entire screen from scratch.

* **scrollok**(< _Boolean_ >scroll) - _Result_ - Controls what happens when the cursor of a window is moved off the edge of the window or scrolling region, either as a result of a newline action on the bottom line, or typing the last character of the last row. If scroll is _true_, the window is scrolled up one line (**Note:** that in order to get the physical scrolling effect on the terminal, it is also necessary to call **idlok**()), otherwise the cursor is left at the bottom line.

* **idlok**(< _Boolean_ >useInsDelLine) - _Result_ - If useInsDelTerm is _true_, ncurses considers using the hardware insert/delete **line** feature of the terminal (if available). Otherwise if useInsDelTerm is _false_, hardware line insertion and deletion is disabled. This option should be enabled only if the application needs insert/delete line, for example, for a screen editor. It is disabled by default because insert/delete line tends to be visually annoying when used in applications where it isn't really needed. If insert/delete line cannot be used, ncurses redraws the changed portions of all lines.

* **idcok**(< _Boolean_ >useInsDelChar) - _Result_ - If useInsDelChar is _true_, ncurses considers using the hardware insert/delete **character** feature of the terminal (if available). Otherwise if useInsDelChar is _false_, ncurses no longer considers using the hardware insert/delete character feature of the terminal. Use of character insert/delete is enabled by default.

* **leaveok**(< _Boolean_ >moveCursor) - _Result_ - Normally, the hardware cursor is left at the location of the window cursor being refreshed. If moveCursor is _true_, ncurses will allow the cursor to be left wherever an update happens to leave it. **It is useful for applications where the cursor is not used, since it reduces the need for cursor motions.** If possible, the cursor is made invisible when this function is called.

* **immedok**(< _Boolean_ >immedUpdate) - _Result_ - If immedUpdate is _true_, any change in the virtual window, such as the ones caused by **addch**(), **clrtobot**(), **scroll**(), etc., automatically cause a call to **refresh**(). However, it may degrade performance considerably, due to repeated calls to **refresh**(). It is disabled by default.

* **standout**(< _Boolean_ >enable) - _Result_ - If enable is _true_, the standout _Attribute_ is enabled. Otherwise, it is disabled.

* **syncok**(< _Boolean_ >autoSyncUp) - _Result_ - If autoSyncUp is _true_, **syncup**() is automatically called whenever there is a change in this window.

* **syncdown**() - _Result_ - Touches each location in the window that has been touched in any of its ancestor windows. This function is called by **refresh**(), so it should almost never be necessary to call it manually.

* **syncup**() - _Result_ - Touches all locations in ancestors of this window that are changed in this window.

* **cursyncup**() - _Result_ - Updates the current cursor position of all the ancestors of the window to reflect the current cursor position of the window.

* **hide**() - _(void)_ - Hides the window.

* **show**() - _(void)_ - Un-hides the window.

* **top**() - _(void)_ - Bring the window to the front.

* **bottom**() - _(void)_ - Send the window to the back (**stdscr** is always the bottom-most window, so this function will actually make the window the bottom-most window right above **stdscr**).

* **move**(< _Integer_ >row, < _Integer_ >column) - _(void)_ - Moves the window to the given row and column.

* **refresh**() - _Result_ - Update the physical screen to match that of the virtual screen.

* **frame**([< _String_ >header[, < _String_ >footer]]) - _(void)_ - Draws a frame around the window and calls **label**() with the optional arguments.

* **boldframe**([< _String_ >header[, < _String_ >footer]]) - _(void)_ - Same as **frame**(), except the frame is highlighted.

* **label**([< _String_ >header[, < _String_ >footer]]) - _(void)_ - Displays an optional centered header at the top and an optional centered footer at the bottom of the window.

* **centertext**(< _Integer_ >row, < _String_ >text) - _(void)_ - Display a centered string at the given row.

* **cursor**(< _Integer_ >row, < _Integer_ >column) - _Result_ - Moves the cursor to the given row and column.

* **insertln**() - _Result_ - Inserts an empty row above the current row.

* **insdelln**([< _Integer_ >nRows=1]) - _Result_ - If nRows > 0, then nRows rows will be inserted above the current row. If nRows < 0, then nRows rows are deleted, beginning with the current row.

* **insstr**(< _String_ >text[, < _Integer_ >charLimit=-1]) - _Result_ - Insert the string into the window before the current cursor position. Insert stops at the end of the string or when charLimit has been reached. If charLimit < 0, it is ignored.

* **insstr**(< _Integer_ >row, < _Integer_ >column, < _String_ >text[, < _Integer_ >charLimit=-1]) - _Result_ - Moves the cursor to the given row and column, then calls the above version of **insstr**() with the rest of the arguments.

* **attron**(< _Attributes_ >attrs) - _Result_ - Switch on the specified window attributes.

* **attroff**(< _Attributes_ >attrs) - _Result_ - Switch off the specified window attributes.

* **attrset**(< _Attributes_ >attrs) - _Result_ - Sets the window's attributes to be exactly that of the attributes specified.

* **attrget**() - _Attributes_ - Get the window's current set of attributes.

* **chgat**(< _Integer_ >nChars, < _Attributes_ >attrs[, < _Integer_ >colorPair]) - _Result_ - Changes the attributes of the next nChars characters starting at the current cursor position to have the given attributes. colorPair specifies a color to use (defaults to the Window's current color pair).

* **chgat**(< _Integer_ >row, < _Integer_ >column, < _Integer_ >nChars, < _Attributes_ >attrs[, < _Integer_ >colorPair]) - _Result_ - Same as the above, except the given row and column are used as the start position.

* **box**([<_ACS\_Character_>vertChar=0[, <_ACS\_Character_>horizChar=0]]) - _Result_ - Draws a box around the window using the optional vertical and horizontal characters. If a zero is given for any of the arguments, ncurses will use the POSIX default characters instead (See **Additional notes**).

* **border**([<_ACS\_Character_>leftChar=0[, <_ACS\_Character_>rightChar=0[, <_ACS\_Character_>topChar=0[, <_ACS\_Character_>bottomChar=0[, <_ACS\_Character_>topLeftChar=0[, <_ACS\_Character_>topRightChar=0[, <_ACS\_Character_>bottomLeftChar=0[, <_ACS\_Character_>bottomRightChar=0]]]]]]]]) - _Result_ - Draws a border around the window using the optionally specified left, right, top, bottom, top left, top right, bottom left, bottom right characters. If any of the characters are zero, ncurses will use the POSIX default characters instead (See **Additional notes**).

* **hline**(< _Integer_ >length[, <_ACS\_Character_>lineChar=0]) - _Result_ - Draws a horizontal line on the current row with the given length. lineChar specifies the character to be used when drawing the line. If lineChar is zero, ncurses will use the POSIX default characters instead (See **Additional notes**).

* **vline**(< _Integer_ >length[, <_ACS\_Character_>lineChar=0]) - _Result_ - Draws a vertical line on the current column with the given length. lineChar specifies the character to be used when drawing the line. If lineChar is zero, ncurses will use the POSIX default characters instead (See **Additional notes**).

* **erase**() - _Result_ - Copies blanks to every position in the window, clearing the screen.

* **clear**() - _Result_ - Similar to **erase**(), but it also calls **clearok**(true), so that the screen is cleared completely on the next call to **refresh**() for the window and is repainted from scratch.

* **clrtobot**() - _Result_ - Clears to the end of the window.

* **clrtoeol**() - _Result_ - Clears to the end of the current row.

* **delch**() - _Result_ - Deletes the character under the cursor.

* **delch**(< _Integer_ >row, < _Integer_ >column) - _Result_ - Moves the cursor to the given row and column and then deletes the character under the cursor.

* **deleteln**() - _Result_ - Deletes the current row.

* **scroll**([< _Integer_ >nLines=1]) - _Result_ - Scrolls nLines lines. Positive values scroll up and negative values scroll down.

* **setscrreg**(< _Integer_ >startRow, < _Integer_ >endRow) - _Result_ - Sets the scrolling region of a window bounded by startRow and endRow (both inclusive).

* **touchlines**(< _Integer_ >startRow, < _Integer_ >nRows[, < _Boolean_ >markModified=true]) - _Result_ - Marks nRows rows starting at startRow as having been modified if markModified is true or unmodified otherwise.

* **is\_linetouched**(< _Integer_ >row) - _Boolean_ - Indicates whether row has been marked as modified.

* **redrawln**(< _Integer_ >startRow, < _Integer_ >nRows) - _Result_ - Redraws nRows rows starting at startRow.

* **touch**() - _Result_ - Marks the entire window as having been modified.

* **untouch**() - _Result_ - Marks the entire window as having been unmodified.

* **resize**(< _Integer_ >rows, < _Integer_ >columns) - _Result_ - Resizes the window to have the given number of rows and columns.

* **print**(< _String_ >text) - _Result_ - Writes text at the current cursor position.

* **print**(< _Integer_ >row, < _Integer_ >column, < _String_ >text) - _Result_ - Moves the cursor to the given row and column and writes text.

* **addstr**(< _String_ >text[, < _Integer_ >charLimit=-1]) - _Result_ - Writes the specified string at the current cursor position. Writing stops at the end of the string or when charLimit has been reached. If charLimit < 0, it is ignored.

* **addstr**(< _Integer_ >row, < _Integer_ >column, < _String_ >text[, < _Integer_ >charLimit=-1]) - _Result_ - Moves the cursor to the given row and column and calls the version of **addstr**() above with the rest of the arguments.

* **close**() - _Result_ - Destroys the window and any of its children. **Only** call this **once** and when you are completely finished with the window.


Window Properties
-------------------

* **hidden** - _Boolean_ [Read-only] - Is this window hidden?

* **height** - _Integer_ [Read-only] - Current window height

* **width** - _Integer_ [Read-only] - Current window width

* **begx** - _Integer_ [Read-only] - Column of the top-left corner of the window, relative to stdscr

* **begy** - _Integer_ [Read-only] - Row of the top-left corner of the window, relative to stdscr

* **curx** - _Integer_ [Read-only] - Column of the current cursor position

* **cury** - _Integer_ [Read-only] - Row of the current cursor position

* **maxx** - _Integer_ [Read-only] - Largest column number for this window

* **maxy** - _Integer_ [Read-only] - Largest row number for this window

* **touched** - _Integer_ [Read-only] - Indicates whether the window has been marked as modified

* **bkgd** - _Attributes_ [Read/Write] - Get/set the window's background attributes


Additional notes
================

Using ncurses with X
--------------------

A resize operation in X sends SIGWINCH to the running application. The ncurses library does not catch this signal, because it cannot in general know how you want the screen re-painted. You will have to write the SIGWINCH handler yourself.

At minimum, your SIGWINCH handler should do a **clearok**(), followed by a **refresh**() on each of your windows.


ACS_Character descriptions
--------------------------

<pre>
Character Name   POSIX Default  Description
--------------   -------------  -----------
ULCORNER              +         upper left-hand corner
LLCORNER              +         lower left-hand corner
URCORNER              +         upper right-hand corner
LRCORNER              +         lower right-hand corner
RTEE                  +         right tee
LTEE                  +         left tee
BTEE                  +         bottom tee
TTEE                  +         top tee
HLINE                 -         horizontal line
VLINE                 |         vertical line
PLUS                  +         plus
S1                    -         scan line 1
S9                    _         scan line 9
DIAMOND               +         diamond
CKBOARD               :         checker board (stipple)
DEGREE                '         degree symbol
PLMINUS               #         plus/minus
BULLET                o         bullet
LARROW                <         arrow pointing left
RARROW                >         arrow pointing right
DARROW                v         arrow pointing down
UARROW                ^         arrow pointing up
BOARD                 #         board of squares
LANTERN               #         lantern symbol
BLOCK                 #         solid square block
</pre>


Keyboard key names
------------------

Keys prefixed with 'S_' denote SHIFT + the key.

* SPACE
* NEWLINE (the real Enter key)
* ESC
* UP
* DOWN
* LEFT
* RIGHT
* HOME
* BACKSPACE
* BREAK
* F0
* F1
* F2
* F3
* F4
* F5
* F6
* F7
* F8
* F9
* F10
* F11
* F12
* DEL
* INS
* EIC
* CLEAR
* EOS
* EOL
* SF
* SR
* NPAGE (page down)
* PPAGE (page up)
* STAB
* CTAB
* CATAB
* ENTER
* SRESET
* RESET
* PRINT
* LL
* UPLEFT
* UPRIGHT
* CENTER
* DOWNLEFT
* DOWNRIGHT
* BTAB
* BEG
* CANCEL
* CLOSE
* COMMAND
* COPY
* CREATE
* END
* EXIT
* FIND
* FIND
* MARK
* MESSAGE
* MOVE
* NEXT
* OPEN
* OPTIONS
* PREVIOUS
* REDO
* REFERENCE
* REFRESH
* REPLACE
* RESTART
* RESUME
* SAVE
* SELECT
* SEND
* SUSPEND
* S_BEG
* S_CANCEL
* S_COMMAND
* S_COPY
* S_CREATE
* S_DC
* S_DL
* S_EOL
* S_EXIT
* S_FIND
* S_HELP
* S_HOME
* S_IC
* S_LEFT
* S_MESSAGE
* S_MOVE
* S_NEXT
* S_OPTIONS
* S_PREVIOUS
* S_PRINT
* S_REDO
* S_REPLACE
* S_RIGHT
* S_RESUME
* S_SAVE
* S_SUSPEND
* S_UNDO
* UNDO
