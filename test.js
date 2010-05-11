var nc = require('./ncurses');

var win = new nc.ncWindow();
win.print("hello world", 0, 0);
win.addListener('inputLine', function (str) {
	win.print("You typed: " + str, 0, 1);
	setTimeout(function() { win.close(); }, 2000);
});
setTimeout(function() { win.close(); }, 5000);