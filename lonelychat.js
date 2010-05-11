var nc = require('./ncurses');

var win = new nc.ncWindow();
win.echo = true;
win.scrollok(true);
win.hline(win.height-2, 0, win.width);
win.setscrreg(0, win.height-3);
win.cursor(win.height-1, 0);
win.refresh();
win.addListener('inputLine', function (str) {
	if (str == "/quit") {
		win.clear();
		win.refresh();
		win.close();
		process.exit(0);
	} else if (!isNaN(str.charCodeAt(0))) {
		win.scroll(1);
		win.print(win.height-3, 0, "<You>: " + str);
		win.cursor(win.height-1, 0);
		win.deleteln();
		win.refresh();
	}
});

// Keep the program alive
setInterval(function() {}, 30000);