var nc = require('./ncurses'), consts = require('./ncconsts'), irc = require('./irc.lib');

function getTimestamp() {
	var time = new Date();
	var hours = time.getHours();
	var mins = time.getMinutes();
	return "" + (hours < 10 ? "0" : "") + hours + ":" + (mins < 10 ? "0" : "") + mins;
}

function appendLine(message) {
	win.scroll(1);
	win.cursor(win.height-3, 0);
	win.print("[" + getTimestamp() + "] ");
	if (arguments.length > 1 && arguments[1]) {
		if (win.hasColors)
			win.attrset(win.colorPair(1));
		win.print("== ");
	}
	if (arguments.length > 2 && arguments[2] && win.hasColors)
		win.attrset(arguments[2]);
	win.print("" + message);
	if (arguments.length > 2 && arguments[2] && win.hasColors)
		win.attrset(win.colorPair(0));
	if (arguments.length > 3)
		win.cursor(win.height-1, arguments[3]);
	else {
		win.cursor(win.height-1, 0);
		win.deleteln();
	}
	win.refresh();
}

function cleanup() {
	win.clear();
	win.refresh();
	win.close();
	process.exit(0);
}

// ============================================================================

var win = new nc.ncWindow();
win.addListener('inputLine', function (str) {
	if (str == "/quit") {
		client.send('QUIT');
		client.disconnect();
		cleanup();
	} else if (!isNaN(str.charCodeAt(0))) {
		appendLine("<" + nick + ">: " + str)
		client.send('PRIVMSG', channel, ':' + str)
	}
});

// Connection info
var nick = "ncursestest";
var server = "irc.freenode.net";
var port = 6667;
var channel = "#node.js";
var realname = "ncursestest";
var username = "ncursestest";

// Setup colors
var COLOR_JOIN = win.colorPair(2);
var COLOR_PART = win.colorPair(3);
var COLOR_QUIT = win.colorPair(6);
var COLOR_ACTION = win.colorPair(5);

// Setup window
win.clear();
win.echo = true;
win.scrollok(true);
win.hline(win.height-2, 0, win.width);
win.setscrreg(0, win.height-3);
win.cursor(win.height-1, 0);
win.refresh();

appendLine("Connecting ...", null, null, win.curx);

var client = new irc.Client(server, port);
client.connect(nick, username, realname);

client.addListener('001', function() { // server 'welcome message' event
  appendLine("Connected!", null, null, win.curx);
  this.send('JOIN', channel);
});

var onJoinChan = function() { appendLine("Joined channel " + channel, null, null, win.curx); };
client.addListener('331', onJoinChan); // successful join, 'no topic set' event
client.addListener('332', onJoinChan); // successful join, 'topic set' event

client.addListener('JOIN', function(prefix) {
  appendLine(irc.user(prefix) + " has joined the channel", true, COLOR_JOIN, win.curx);
});

client.addListener('PART', function(prefix) {
  appendLine(irc.user(prefix) + " has left the channel", true, COLOR_PART,win.curx);
});

client.addListener('QUIT', function(prefix) {
  appendLine(irc.user(prefix) + " has quit IRC", true, COLOR_QUIT, win.curx);
});

client.addListener('DISCONNECT', function() {
  appendLine("Disconnected.", null, null, win.curx);
  setTimeout(cleanup(), 3000);
});

client.addListener('PRIVMSG', function(prefix, channel, text) {
  appendLine("<" + irc.user(prefix) + ">: " + text, null, null, win.curx);
});