var nc = require('../ncurses'), consts = require('../ncconsts'), irc = require('./irc.lib'), sys = require('sys');

function updateTopic(topic) {
	var curx=win.curx, cury=win.cury;
	topic = "" + topic;
	if (topic.length > 0) {
		win.cursor(0, 0);
		win.clrtoeol();
		win.addstr(topic, win.cols);
		win.cursor(cury, curx);
	}
	win.refresh();
}

function getTimestamp() {
	var time = new Date();
	var hours = time.getHours();
	var mins = time.getMinutes();
	return "" + (hours < 10 ? "0" : "") + hours + ":" + (mins < 10 ? "0" : "") + mins;
}

function appendLine(message) {
	var curx=win.curx;
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
	if (arguments.length > 3) {
		win.cursor(win.height-1, 0);
		win.clrtoeol();
	} else
		win.cursor(win.height-1, curx);
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
var inputLine = "";
win.addListener('inputChar', function (chr, intval) {
	if (intval == consts.BACKSPACE || intval == 7) {
		win.delch();
		if (inputLine.length)
			inputLine = inputLine.substr(0, inputLine.length-1);
	} else if (chr == "\n") {
		if (inputLine.substr(0, 4) == "/msg") {
			inputLine = inputLine.substr(5);
			var recipient = inputLine.substr(0, inputLine.indexOf(' '));
			var msg = inputLine.substr(inputLine.indexOf(' ')+1);
			appendLine("to(" + recipient + "): " + msg, null, null, true);
			client.send('PRIVMSG', recipient, ':' + msg);
		} else if (inputLine.substr(0, 3) == "/me") {
			inputLine = inputLine.substr(4);
			appendLine("* " + nick + " " + inputLine, null, null, true);
			client.send('PRIVMSG', channel, ':' + String.fromCharCode(1) + "ACTION " + inputLine + String.fromCharCode(1));
		} else if (inputLine == "/quit") {
			client.send('QUIT');
			client.disconnect();
			cleanup();
		} else if (!isNaN(inputLine.charCodeAt(0))) {
			appendLine("<" + nick + ">: " + inputLine, null, null, true);
			client.send('PRIVMSG', channel, ':' + inputLine);
		}
		inputLine = "";
	} else
		inputLine += chr;
});

// Connection info
var nick = "ncursestest";
var server = "irc.freenode.net";
var port = 6667;
var channel = "#node.js";
var realname = "ncursestest";
var username = "ncursestest";

// Misc IRC client info
var client_name = "Ncurses_IRC_client";
var client_ver = "0.1";
var client_env = "Node.JS";

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
win.setscrreg(1, win.height-3); // Leave one line at the top for the channel name and optional topic
win.cursor(win.height-1, 0);
win.refresh();

appendLine("Connecting ...");

var client = new irc.Client(server, port);
client.connect(nick, username, realname);

client.addListener('001', function() { // server 'welcome message' event
	appendLine("Connected!");
	this.send('JOIN', channel);
});

client.addListener('331', function(prefix, usr, chnl) { // successful join, 'no topic set' event
	appendLine("Joined channel " + chnl);
	updateTopic("[" + chnl + "]");	
});

client.addListener('332', function(prefix, usr, chnl, topic) { // successful join, 'topic set' event
	appendLine("Joined channel " + chnl);
	updateTopic("[" + chnl + "] " + topic );
});

client.addListener('JOIN', function(prefix) {
	appendLine(irc.user(prefix) + " has joined the channel", true, COLOR_JOIN);
});

client.addListener('PART', function(prefix) {
	appendLine(irc.user(prefix) + " has left the channel", true, COLOR_PART);
});

client.addListener('QUIT', function(prefix) {
	appendLine(irc.user(prefix) + " has quit IRC", true, COLOR_QUIT);
});

client.addListener('DISCONNECT', function() {
	appendLine("Disconnected.");
	setTimeout(cleanup(), 3000);
});

client.addListener('PRIVMSG', function(prefix, to, text) {
	if (text.charCodeAt(0) == 1 && text.charCodeAt(text.length-1) == 1) {
		// CTCP message
		var payload = text.substring(1, text.length-1);
		if (payload.substr(0, 6) == "ACTION")
			appendLine("* " + irc.user(prefix) + " " + payload.substr(7));
		else if (payload == "TIME") {
			var curTime = new Date(), dow = curTime.getDay(), day = curTime.getDate(), month = curTime.getMonth(), year = curTime.getFullYear(),
			    hours = curTime.getHours(), mins = curTime.getMinutes(), secs = curTime.getSeconds(), tz = curTime.toString().replace(/^.*\(([^)]+)\)$/, '$1');
			var strTime = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'].slice(dow, (dow == 6 ? dow : dow+1)).toString()
						  + " " + ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'].slice(month, (month == 11 ? month : month+1)).toString()
						  + " " + (day < 10 ? "0" : "") + day + " " + (hours < 10 ? "0" : "") + hours + ":" + (mins < 10 ? "0" : "") + mins + ":" + (secs < 10 ? "0" : "") + secs
						  + " " + year + " " + tz.toUpperCase();
			client.send('NOTICE', irc.user(prefix), ':' + String.fromCharCode(1) + "TIME " + strTime + String.fromCharCode(1));
		} else if (payload == "VERSION")
			client.send('NOTICE', irc.user(prefix), ':' + String.fromCharCode(1) + "VERSION " + client_name + " " + client_ver + " " + client_env + String.fromCharCode(1));
		else
			appendLine("Unknown CTCP payload: " + payload);
	} else if (to[0] == "#")
		appendLine("<" + irc.user(prefix) + ">: " + text);
	else
		appendLine("from(" + irc.user(prefix) + "): " + text);
});