var nc = require('./ncurses');

var win = new nc.ncWindow();
var intScroll;
win.centertext(win.lines/2, "hello world");
win.refresh();
win.addListener('inputLine', function (str) {
	if (intScroll)
		clearInterval(intScroll);
	win.print(2, 1, "You typed: " + str);
	win.refresh();
	setTimeout(function() { win.close(); process.exit(0); }, 2500);
});
win.scrollok(true);
var scrollsdown = 5;
var scrollsup = 5;
intScroll = setInterval(function() {
	if (scrollsdown-- > 0) {
		win.scroll(-1);
		win.refresh();
	} else if (scrollsup-- > 0) {
		win.scroll(1);
		win.refresh();
	} else {
		win.close();
		clearInterval(int);
	}
}, 1000);