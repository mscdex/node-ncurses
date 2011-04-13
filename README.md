Description
===========

node-ncurses is an ncurses binding for [node.js](http://nodejs.org/).


Requirements
============

* [node.js](http://nodejs.org/) -- v0.1.94+

To build node-curses:

    node-waf configure build


Troubleshooting
===============

If you encounter "Error opening terminal" while executing one of the examples or
any other script that uses node-ncurses, do this:

* Ensure you have built node-ncurses properly
* Change to `deps/ncurses` in the source package
* Execute `sudo make install.data`

That should solve this particular problem.


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
    * An _ACS\_Character_ can currently be retrieved by using the 'ACS' property of the module (example: "var nc = require('./ncurses'), win = new Window(); win.hline(nc.cols, nc.ACS.DIAMOND);")


Module Functions
---------------- 

* **colorPair**(Integer[, Integer, Integer]) - _Integer_ - The first value specifies a color pair number in the color palette. If the second and third values are provided, that color pair's foreground and background colors are set respectively. The color pair number is always returned.

* **colorFg**(Integer) - _Integer_ - Returns the foreground color currently set for the supplied color pair.

* **colorBg**(Integer) - _Integer_ - Returns the background color currently set for the supplied color pair.

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

* **echo** - _Boolean_ [Read/Write] - Enable/disable local echoing of keyboard input

* **showCursor** - _Boolean_ [Read/Write] - Show/hide the cursor

* **raw** - _Boolean_ [Read/Write] - Enable/disable terminal's raw mode

* **lines** - _Integer_ [Read-only] - The total number of rows of the terminal

* **cols** - _Integer_ [Read-only] - The total number of columns of the terminal

* **tabsize** - _Integer_ [Read-only] - The terminal's tab size

* **hasMouse** - _Boolean_ [Read-only] - Indicates whether the terminal supports mouse (clicks) functionality

* **hasColors** - _Boolean_ [Read-only] - Indicates whether the terminal supports colors

* **numColors** - _Integer_ [Read-only] - Indicates the (maximum) number of colors the terminal supports colors

* **maxColorPairs** - _Integer_ [Read-only] - The maximum number of foreground-background color pairs supported by the terminal


Window Events
-------------

* **inputChar**(String, Integer) - Called when the user has pressed a key on the keyboard. The first parameter is a string containing the character and the second parameter is the integer value of the character.


Window Functions
----------------

* **clearok**(Boolean) - _Result_ - If _true_ is passed in, the next call to **refresh**() will clear the screen completely and redraw the entire screen from scratch.

* **scrollok**(Boolean) - _Result_ - Controls what happens when the cursor of a window is moved off the edge of the window or scrolling region, either as a result of a newline action on the bottom line, or typing the last character of the last row. If _true_ is passed in, the window is scrolled up one line (**Note:** that in order to get the physical scrolling effect on the terminal, it is also necessary to call **idlok**()), otherwise the cursor is left at the bottom line.

* **idlok**(Boolean) - _Result_ - If _true_ is passed in, ncurses considers using the hardware insert/delete **line** feature of the terminal (if available). Otherwise if _false_ is passed in, hardware line insertion and deletion is disabled. This option should be enabled only if the application needs insert/delete line, for example, for a screen editor. It is disabled by default because insert/delete line tends to be visually annoying when used in applications where it isn't really needed. If insert/delete line cannot be used, ncurses redraws the changed portions of all lines.

* **idcok**(Boolean) - _Result_ - If _true_ is passed in, ncurses considers using the hardware insert/delete **character** feature of the terminal (if available). Otherwise if _false_ is passed in, ncurses no longer considers using the hardware insert/delete character feature of the terminal. Use of character insert/delete is enabled by default.

* **leaveok**(Boolean) - _Result_ - Normally, the hardware cursor is left at the location of the window cursor being refreshed. Passing in _true_ to this function allows the cursor to be left wherever the update happens to leave it. **It is useful for applications where the cursor is not used, since it reduces the need for cursor motions.** If possible, the cursor is made invisible when this function is called.

* **immedok**(Boolean) - _Result_ - If _true_ is passed in, any change in the virtual window, such as the ones caused by **addch**(), **clrtobot**(), **scroll**(), etc., automatically cause a call to **refresh**(). However, it may degrade performance considerably, due to repeated calls to **refresh**(). It is disabled by default.

* **standout**(Boolean) - _Result_ - If _true_ is passed in, the standout _Attribute_ is enabled. Otherwise, it is disabled.

* **syncok**(Boolean) - _Result_ - If _true_ is passed in, **syncup**() is automatically caled whenever there is a change in this window.

* **syncdown**() - _Result_ - Touches each location in the window that has been touched in any of its ancestor windows. This function is called by **refresh**(), so it should almost never be necessary to call it manually.

* **syncup**() - _Result_ - Touches all locations in ancestors of this window that are changed in this window.

* **cursyncup**() - _Result_ - Updates the current cursor position of all the ancestors of the window to reflect the current cursor position of the window.

* **hide**() - _(void)_ - Hides the window.

* **show**() - _(void)_ - Un-hides the window.

* **top**() - _(void)_ - Bring the window to the front.

* **bottom**() - _(void)_ - Send the window to the back (**stdscr** is always the bottom-most window, so this function will actually make the window the bottom-most window right above **stdscr**).

* **move**(Integer, Integer) - _(void)_ - Moves the window to the specified row and column respectively.

* **refresh**() - _Result_ - Update the physical screen to match that of the virtual screen.

* **frame**([String[, String]]) - _(void)_ - Draws a frame around the window and calls **label**() with the optional parameters.

* **boldframe**([String[, String]]) - _(void)_ - Same as **frame**(), except the frame is highlighted.

* **label**([String[, String]]) - _(void)_ - Displays a title at the top and bottom of the window with the first optional parameter being the title text to display centered at the top row of the window and the second optional parameter being the title text to display centered at the bottom row of the window.

* **centertext**(Integer, String) - _(void)_ - Display a centered string at the row specified by the first parameter.

* **cursor**(Integer, Integer) - _Result_ - Moves the cursor to the specified row and column respectively.

* **insertln**() - _Result_ - Inserts an empty row above the current row.

* **insdelln**([Integer=1]) - _Result_ - If the passed in value is greater than zero, that many rows will be inserted above the current row. If the passed in value is less than zero, that many rows are deleted, beginning with the current row.

* **insstr**(String[, Integer=-1]) - _Result_ - Insert the string into the window before the current cursor position. Insert stops at the end of the string or when the limit indicated by the second parameter is reached. If the second parameter is negative, the limit is ignored.

* **insstr**(Integer, Integer, String[, Integer=-1]) - _Result_ - Moves the cursor to the row and column specified by the first two parameters respectively, then calls the version of **insstr**() above with the rest of the parameters.

* **attron**(Attributes) - _Result_ - Switch on the specified window attributes.

* **attroff**(Attributes) - _Result_ - Switch off the specified window attributes.

* **attrset**(Attributes) - _Result_ - Sets the window's attributes to be exactly that of the attributes specified.

* **attrget**() - _Attributes_ - Get the window's current set of attributes.

* **chgat**(Integer, Attributes[, Integer]) - _Result_ - Changes the attributes of the next n characters specified by the first parameter (starting at the current cursor position) to have the attributes given by the second parameter. The optional third parameter is used for specifying a color to use (defaults to the Window's current color pair).

* **chgat**(Integer, Integer, Integer, Attributes[, Integer]) - _Result_ - Same as the above, except the first two parameters specify window coordinates to start at.

* **box**([ACS_Character=0[, ACS_Character=0]]) - _Result_ - Draws a box around the window using the optionally specified parameters as the vertical and horizontal characters respectively. If a zero is given for any of the parameters, ncurses will use the POSIX default characters instead (See **Additional notes**).

* **border**([ACS\_Character=0[, ACS\_Character=0[, ACS\_Character=0[, ACS\_Character=0[, ACS\_Character=0[, ACS\_Character=0[, ACS\_Character=0[, ACS\_Character=0]]]]]]]]) - _Result_ - Draws a border around the window using the optionally specified parameters as the left, right, top, bottom, top left, top right, bottom left, bottom right characters respectively. If a zero is given for any of the parameters, ncurses will use the POSIX default characters instead (See **Additional notes**).

* **hline**(Integer[, ACS\_Character=0]) - _Result_ - Draws a horizontal line on the current row whose length is determined by the first parameter. The second parameter specifies the character to be used when drawing the line. If a zero is given for the second parameter, ncurses will use the POSIX default characters instead (See **Additional notes**).

* **vline**(Integer[, ACS\_Character=0]) - _Result_ - Draws a vertical line on the current column whose length is determined by the first parameter. The second parameter specifies the character to be used when drawing the line. If a zero is given for the second parameter, ncurses will use the POSIX default characters instead (See **Additional notes**).

* **erase**() - _Result_ - Copies blanks to every position in the window, clearing the screen.

* **clear**() - _Result_ - Similar to **erase**(), but it also calls **clearok**(true), so that the screen is cleared completely on the next call to **refresh**() for the window and is repainted from scratch.

* **clrtobot**() - _Result_ - Clears to the end of the window.

* **clrtoeol**() - _Result_ - Clears to the end of the current row.

* **delch**() - _Result_ - Deletes the character under the cursor.

* **delch**(Integer, Integer) - _Result_ - Moves the cursor to the row and column specified by the first two parameters respectively, then calls the version of **delch**() above with the rest of the parameters.

* **deleteln**() - _Result_ - Deletes the current row.

* **scroll**([Integer=1]) - _Result_ - Scrolls the amount of lines specified. Positive values scroll up and negative values scroll down.

* **setscrreg**(Integer, Integer) - _Result_ - Sets the scrolling region of a window using the first parameter as the starting row and the second parameter as the ending row (both inclusive).

* **touchlines**(Integer, Integer[, Boolean=true]) - _Result_ - Marks rows as having been modified or unmodified. The first parameter is the starting row. The second parameter is the number of rows after the starting row. If the third parameter is true, then the lines are marked as modified, otherwise they are marked as unmodified.

* **is\_linetouched**(Integer) - _Boolean_ - Indicates whether the specified row has been marked as modified.

* **redrawln**(Integer, Integer) - _Result_ - Redraws a number of rows specified by the second parameter, starting with the row specified in the first parameter.

* **touch**() - _Result_ - Marks the entire window as having been modified.

* **untouch**() - _Result_ - Marks the entire window as having been unmodified.

* **resize**(Integer, Integer) - _Result_ - Resizes the window to the specified number of rows and columns respectively.

* **print**(String) - _Result_ - Writes the specified string at the current cursor position.

* **print**(Integer, Integer, String) - _Result_ - Moves the cursor to the row and column specified by the first two parameters respectively, then calls the version of **print**() above with the rest of the parameters.

* **addstr**(String[, Integer=-1]) - _Result_ - Writes the specified string at the current cursor position. Writing stops at the end of the string or when the limit indicated by the second parameter is reached. If the second parameter is negative, the limit is ignored.

* **addstr**(Integer, Integer, String[, Integer=-1]) - _Result_ - Moves the cursor to the row and column specified by the first two parameters respectively, then calls the version of **addstr**() above with the rest of the parameters.

* **close**() - _Result_ - Destroys the window and any of its children. **Only** call this **once** and when you are completely finished with the window.


Window Properties
-------------------

* **bkgd** - _Attributes_ [Read/Write] - Get/set the window's background attributes

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
