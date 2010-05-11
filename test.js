var nc = require('./ncurses');

var win = new nc.ncWindow();
win.print("hello world");
win.refresh();
win.addListener('inputLine', function (str) {
	win.print(1, 0, "You typed: " + str);
	win.refresh();
	setTimeout(function() { win.close(); process.exit(0); }, 2000);
});
setTimeout(function() { win.close(); }, 5000);