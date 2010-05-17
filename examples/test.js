var nc = require('../ncurses'), consts = require('../ncconsts');

var intScroll, inputLine = "";
var win = new nc.ncWindow();
win.centertext(win.lines/2, "hello world");
win.refresh();
win.addListener('inputChar', function (chr, intval) {
	if (intval == consts.BACKSPACE || intval == 7) {
		if (inputLine.length)
			inputLine = inputLine.substr(0, inputLine.length-1);
	} else if (chr == "\n") {
		if (intScroll)
			clearInterval(intScroll);
		win.print(2, 1, "You typed: " + inputLine);
		win.refresh();
		setTimeout(function() { win.close(); process.exit(0); }, 2500);
	} else
		inputLine += chr;
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
		clearInterval(intScroll);
	}
}, 1000);