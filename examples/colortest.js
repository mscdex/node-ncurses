var nc = require('../ncurses'), consts = require('../ncconsts');

var win = new nc.ncWindow();
win.print("Max color pairs support == " + win.maxColorPairs + "\n");
win.print("Initialized color pairs == " + win.numColors + "\n\n");
win.print("White on black\n");

// Note 1: Color pair number 0 is the default (white on black) and cannot be changed.
// Note 2: When the first node-ncurses window is first instantiated, it will set up a palette
//         containing the other 6 colors (excluding black) on black for convenience.

win.attrset(win.colorPair(1, consts.colors.RED, consts.colors.WHITE));
win.print("Red on white\n");

win.attrset(win.colorPair(2));
win.print("Green on black\n");

win.attrset(win.colorPair(3));
win.print("Yellow on black\n");

win.refresh();

setTimeout(function() {win.close();}, 3000);