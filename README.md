node-curses is a node.js binding for ncurses.

<b>You will need the ncurses++ C++ binding installed in addition to ncurses.</b>
Currently, if you don't have ncurses++ installed, you'll get build errors, with one of the first ones being:
	../ncurses.cc:9:21: error: cursesp.h: No such file or directory

To build node-curses:
	node-waf configure build
	

Documentation has yet to come, be patient. Until then, check out the source code for the available functions ;-)