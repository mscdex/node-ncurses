#ifndef CTRL
#  define CTRL(x) ((x) & 0x1f)
#endif

#ifndef NULL
#  define NULL 0
#endif

#ifndef STDIN_FILNO
#  define STDIN_FILENO  0
#  define STDOUT_FILENO 1
#endif

#include <cursesp.h>
#include <node.h>
#include <node_events.h>

#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>

using namespace std;
using namespace v8;
using namespace node;

static int stdin_fd = -1;
static int stdout_fd = -1;

static Persistent<String> inputChar_symbol;
static Persistent<Array> ACS_Chars;

#define ECHO_STATE_SYMBOL String::NewSymbol("echo")
#define SHOWCURSOR_STATE_SYMBOL String::NewSymbol("showCursor")
#define LINES_STATE_SYMBOL String::NewSymbol("lines")
#define COLS_STATE_SYMBOL String::NewSymbol("cols")
#define TABSIZE_STATE_SYMBOL String::NewSymbol("tabsize")
#define HASMOUSE_STATE_SYMBOL String::NewSymbol("hasMouse")
#define HASCOLORS_STATE_SYMBOL String::NewSymbol("hasColors")
#define NUMCOLORPAIRS_STATE_SYMBOL String::NewSymbol("numColorPairs")
#define MAXCOLORPAIRS_STATE_SYMBOL String::NewSymbol("maxColorPairs")
#define ACS_CONSTS_SYMBOL String::NewSymbol("ACS")

#define BKGD_STATE_SYMBOL String::NewSymbol("bkgd")
#define HIDDEN_STATE_SYMBOL String::NewSymbol("hidden")
#define HEIGHT_STATE_SYMBOL String::NewSymbol("height")
#define WIDTH_STATE_SYMBOL String::NewSymbol("width")
#define BEGX_STATE_SYMBOL String::NewSymbol("begx")
#define BEGY_STATE_SYMBOL String::NewSymbol("begy")
#define CURX_STATE_SYMBOL String::NewSymbol("curx")
#define CURY_STATE_SYMBOL String::NewSymbol("cury")
#define MAXX_STATE_SYMBOL String::NewSymbol("maxx")
#define MAXY_STATE_SYMBOL String::NewSymbol("maxy")
#define WINTOUCHED_STATE_SYMBOL String::NewSymbol("touched")

// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
	return *value ? *value : "<string conversion failed>";
}

class MyPanel : public NCursesPanel {
	private:
		void setup() {
			// Initialize color support if it's available
			if (::has_colors())
				::start_color();
				
			// Set non-blocking mode and tell ncurses we want to receive one character at a time instead of one line at a time
			::nodelay(w, true);
			::nocbreak();
			::halfdelay(1);

			// Only setup the default palette once, as to not override any possible palette changes made by the user later on
			if (w == ::stdscr) {
				::init_pair(1, COLOR_RED, COLOR_BLACK);
				::init_pair(2, COLOR_GREEN, COLOR_BLACK);
				::init_pair(3, COLOR_YELLOW, COLOR_BLACK);
				::init_pair(4, COLOR_BLUE, COLOR_BLACK);
				::init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
				::init_pair(6, COLOR_CYAN, COLOR_BLACK);
			}
			
			// Set the window's default color pair (white on black)
			::wcolor_set(w, 0, NULL);
		}
		static bool echoInput_;
		static bool showCursor_;
	public:
		MyPanel(int nlines, int ncols, int begin_y = 0, int begin_x = 0) : NCursesPanel(nlines,ncols,begin_y,begin_x) {
			this->setup();
		}
		MyPanel() : NCursesPanel() {
			this->setup();
		}
		~MyPanel() {
			if (w == ::stdscr)
				endwin();
		}
		static void echo(bool value) {
			MyPanel::echoInput_ = value;
			if (MyPanel::echoInput_)
				::echo();
			else
				::noecho();
		}
		static bool echo() {
			return MyPanel::echoInput_;
		}
		static void showCursor(bool value) {
			MyPanel::showCursor_ = value;
			if (MyPanel::showCursor_)
				::curs_set(1);
			else
				::curs_set(0);
		}
		static bool showCursor() {
			return MyPanel::showCursor_;
		}
		bool isStdscr() {
			return (w == ::stdscr);
		}
		char getch() {
			return ::wgetch(w);
		}
		static bool has_colors() {
			return ::has_colors();
		}
		static int num_pairs() {
			return COLORS;
		}
		static int max_pairs() {
			return COLOR_PAIRS;
		}
		unsigned int pair(int pair, short fore=-1, short back=-1) {
			if (::has_colors()) {
				if (fore == -1 && back == -1)
					return COLOR_PAIR(pair);
				else {
					::init_pair(pair, fore, back);
					return COLOR_PAIR(pair);
				}
			}
		}
		void setFgcolor(int pair, short color) {
			short fore, back;
			if (::pair_content(pair, &fore, &back) != ERR)
				::init_pair(pair, color, back);
		}
		void setBgcolor(int pair, short color) {
			short fore, back;
			if (::pair_content(pair, &fore, &back) != ERR)
				::init_pair(pair, fore, color);
		}
		short getFgcolor(int pair) {
			short fore, back;
			if (::pair_content(pair, &fore, &back) != ERR)
				return fore;
			else
				return -1;
		}
		short getBgcolor(int pair) {
			short fore, back;
			if (::pair_content(pair, &fore, &back) != ERR)
				return back;
			else
				return -1;
		}
		void resetScreen() {
			endwin();
		}
};

bool MyPanel::echoInput_ = false;
bool MyPanel::showCursor_ = true;

class ncWindow : public EventEmitter {
	public:
		static void	Initialize (Handle<Object> target) {
			HandleScope scope;
			
			Local<FunctionTemplate> t = FunctionTemplate::New(New);
			
			t->Inherit(EventEmitter::constructor_template);
			t->InstanceTemplate()->SetInternalFieldCount(1);
			
			inputChar_symbol = NODE_PSYMBOL("inputChar");

			/* Global/Terminal functions */
			NODE_SET_PROTOTYPE_METHOD(t, "colorPair", Colorpair);
			NODE_SET_PROTOTYPE_METHOD(t, "resetScreen", Resetscreen);

			/* Panel-specific methods */
			// TODO: color_set?, chgat, overlay, overwrite, copywin
			NODE_SET_PROTOTYPE_METHOD(t, "clearok", Clearok);
			NODE_SET_PROTOTYPE_METHOD(t, "scrollok", Scrollok);
			NODE_SET_PROTOTYPE_METHOD(t, "idlok", Idlok);
			NODE_SET_PROTOTYPE_METHOD(t, "idcok", Idcok);
			NODE_SET_PROTOTYPE_METHOD(t, "leaveok", Leaveok);
			NODE_SET_PROTOTYPE_METHOD(t, "syncok", Syncok);
			NODE_SET_PROTOTYPE_METHOD(t, "immedok", Immedok);
			NODE_SET_PROTOTYPE_METHOD(t, "keypad", Keypad);
			NODE_SET_PROTOTYPE_METHOD(t, "meta", Meta);
			NODE_SET_PROTOTYPE_METHOD(t, "standout", Standout);
			NODE_SET_PROTOTYPE_METHOD(t, "hide", Hide);
			NODE_SET_PROTOTYPE_METHOD(t, "show", Show);
			NODE_SET_PROTOTYPE_METHOD(t, "top", Top);
			NODE_SET_PROTOTYPE_METHOD(t, "bottom", Bottom);
			NODE_SET_PROTOTYPE_METHOD(t, "move", Mvwin);
			NODE_SET_PROTOTYPE_METHOD(t, "refresh", Refresh);
			NODE_SET_PROTOTYPE_METHOD(t, "noutrefresh", Noutrefresh);
			NODE_SET_PROTOTYPE_METHOD(t, "redraw", Redraw);
			NODE_SET_PROTOTYPE_METHOD(t, "frame", Frame);
			NODE_SET_PROTOTYPE_METHOD(t, "boldframe", Boldframe);
			NODE_SET_PROTOTYPE_METHOD(t, "label", Label);
			NODE_SET_PROTOTYPE_METHOD(t, "centertext", Centertext);
			NODE_SET_PROTOTYPE_METHOD(t, "cursor", Move);
			NODE_SET_PROTOTYPE_METHOD(t, "insertln", Insertln);
			NODE_SET_PROTOTYPE_METHOD(t, "insdelln", Insdelln);
			NODE_SET_PROTOTYPE_METHOD(t, "insstr", Insstr);
			NODE_SET_PROTOTYPE_METHOD(t, "attron", Attron);
			NODE_SET_PROTOTYPE_METHOD(t, "attroff", Attroff);
			NODE_SET_PROTOTYPE_METHOD(t, "attrset", Attrset);
			NODE_SET_PROTOTYPE_METHOD(t, "attrget", Attrget);
			NODE_SET_PROTOTYPE_METHOD(t, "box", Box);
			NODE_SET_PROTOTYPE_METHOD(t, "border", Border);
			NODE_SET_PROTOTYPE_METHOD(t, "hline", Hline);
			NODE_SET_PROTOTYPE_METHOD(t, "vline", Vline);
			NODE_SET_PROTOTYPE_METHOD(t, "erase", Erase);
			NODE_SET_PROTOTYPE_METHOD(t, "clear", Clear);
			NODE_SET_PROTOTYPE_METHOD(t, "clrtobot", Clrtobot);
			NODE_SET_PROTOTYPE_METHOD(t, "clrtoeol", Clrtoeol);
			NODE_SET_PROTOTYPE_METHOD(t, "delch", Delch);
			NODE_SET_PROTOTYPE_METHOD(t, "deleteln", Deleteln);
			NODE_SET_PROTOTYPE_METHOD(t, "scroll", Scroll);
			NODE_SET_PROTOTYPE_METHOD(t, "setscrreg", Setscrreg);
			NODE_SET_PROTOTYPE_METHOD(t, "touchlines", Touchln);
			NODE_SET_PROTOTYPE_METHOD(t, "is_linetouched", Is_linetouched);
			NODE_SET_PROTOTYPE_METHOD(t, "redrawln", Redrawln);
			NODE_SET_PROTOTYPE_METHOD(t, "touch", Touchwin);
			NODE_SET_PROTOTYPE_METHOD(t, "untouch", Untouchwin);
			NODE_SET_PROTOTYPE_METHOD(t, "resize", Wresize);
			NODE_SET_PROTOTYPE_METHOD(t, "print", Print);
			NODE_SET_PROTOTYPE_METHOD(t, "addstr", Addstr);
			NODE_SET_PROTOTYPE_METHOD(t, "close", Close);
			NODE_SET_PROTOTYPE_METHOD(t, "syncdown", Syncdown);
			NODE_SET_PROTOTYPE_METHOD(t, "syncup", Syncup);
			NODE_SET_PROTOTYPE_METHOD(t, "cursyncup", Cursyncup);

			/* Attribute-related window functions */
			NODE_SET_PROTOTYPE_METHOD(t, "addch", Addch);
			NODE_SET_PROTOTYPE_METHOD(t, "echochar", Echochar);
			NODE_SET_PROTOTYPE_METHOD(t, "inch", Inch);
			NODE_SET_PROTOTYPE_METHOD(t, "insch", Insch);
			//NODE_SET_PROTOTYPE_METHOD(t, "addchstr", Addchstr);
			//NODE_SET_PROTOTYPE_METHOD(t, "inchstr", Inchstr);
			
			/* Global/Terminal properties */
			t->PrototypeTemplate()->SetAccessor(ECHO_STATE_SYMBOL, EchoStateGetter, EchoStateSetter);
			t->PrototypeTemplate()->SetAccessor(SHOWCURSOR_STATE_SYMBOL, ShowcursorStateGetter, ShowcursorStateSetter);
			t->PrototypeTemplate()->SetAccessor(LINES_STATE_SYMBOL, LinesStateGetter);
			t->PrototypeTemplate()->SetAccessor(COLS_STATE_SYMBOL, ColsStateGetter);
			t->PrototypeTemplate()->SetAccessor(TABSIZE_STATE_SYMBOL, TabsizeStateGetter);
			t->PrototypeTemplate()->SetAccessor(HASMOUSE_STATE_SYMBOL, HasmouseStateGetter);
			t->PrototypeTemplate()->SetAccessor(HASCOLORS_STATE_SYMBOL, HascolorsStateGetter);
			t->PrototypeTemplate()->SetAccessor(NUMCOLORPAIRS_STATE_SYMBOL, NumcolorpairsStateGetter);
			t->PrototypeTemplate()->SetAccessor(MAXCOLORPAIRS_STATE_SYMBOL, MaxcolorpairsStateGetter);
			t->PrototypeTemplate()->SetAccessor(ACS_CONSTS_SYMBOL, ACSConstsGetter);

			/* Window properties */
			t->PrototypeTemplate()->SetAccessor(BKGD_STATE_SYMBOL, BkgdStateGetter, BkgdStateSetter);
			t->PrototypeTemplate()->SetAccessor(HIDDEN_STATE_SYMBOL, HiddenStateGetter);
			t->PrototypeTemplate()->SetAccessor(HEIGHT_STATE_SYMBOL, HeightStateGetter);
			t->PrototypeTemplate()->SetAccessor(WIDTH_STATE_SYMBOL, WidthStateGetter);
			t->PrototypeTemplate()->SetAccessor(BEGX_STATE_SYMBOL, BegxStateGetter);
			t->PrototypeTemplate()->SetAccessor(BEGY_STATE_SYMBOL, BegyStateGetter);
			t->PrototypeTemplate()->SetAccessor(CURX_STATE_SYMBOL, CurxStateGetter);
			t->PrototypeTemplate()->SetAccessor(CURY_STATE_SYMBOL, CuryStateGetter);
			t->PrototypeTemplate()->SetAccessor(MAXX_STATE_SYMBOL, MaxxStateGetter);
			t->PrototypeTemplate()->SetAccessor(MAXY_STATE_SYMBOL, MaxyStateGetter);
			t->PrototypeTemplate()->SetAccessor(WINTOUCHED_STATE_SYMBOL, WintouchedStateGetter);

			target->Set(String::NewSymbol("ncWindow"), t->GetFunction());
		}
		
		void init(int nlines=-1, int ncols=-1, int begin_y=-1, int begin_x=-1) {
			bool firstRun = false;
			if (stdin_fd < 0) {
				firstRun = true;
				stdin_fd = STDIN_FILENO;
				int stdin_flags = fcntl(stdin_fd, F_GETFL, 0);
				int r = fcntl(stdin_fd, F_SETFL, stdin_flags | O_NONBLOCK);
				if (r < 0)
					throw "Unable to set stdin to non-block";

				stdout_fd = STDOUT_FILENO;
				int stdout_flags = fcntl(stdout_fd, F_GETFL, 0);
				r = fcntl(stdout_fd, F_SETFL, stdout_flags | O_NONBLOCK);
				if (r < 0)
					throw "Unable to set stdin to non-block";
			}

			// Setup input listener
			ev_init(&read_watcher_, io_event);
			read_watcher_.data = this;
			ev_io_set(&read_watcher_, stdin_fd, EV_READ);
			ev_io_start(EV_DEFAULT_ &read_watcher_);
			if (nlines < 0 || ncols < 0 || begin_y < 0 || begin_x < 0)
				panel_ = new MyPanel();
			else
				panel_ = new MyPanel(nlines, ncols, begin_y, begin_x);
			if (firstRun) {
				// Load runtime-defined ACS_* "constants"
				ACS_Chars = Persistent<Array>::New(Array::New(25));
				ACS_Chars->Set(String::New("ULCORNER"), Uint32::NewFromUnsigned(ACS_ULCORNER), ReadOnly);
				ACS_Chars->Set(String::New("LLCORNER"), Uint32::NewFromUnsigned(ACS_LLCORNER), ReadOnly);
				ACS_Chars->Set(String::New("URCORNER"), Uint32::NewFromUnsigned(ACS_URCORNER), ReadOnly);
				ACS_Chars->Set(String::New("LRCORNER"), Uint32::NewFromUnsigned(ACS_LRCORNER), ReadOnly);
				ACS_Chars->Set(String::New("LTEE"), Uint32::NewFromUnsigned(ACS_LTEE), ReadOnly);
				ACS_Chars->Set(String::New("RTEE"), Uint32::NewFromUnsigned(ACS_RTEE), ReadOnly);
				ACS_Chars->Set(String::New("BTEE"), Uint32::NewFromUnsigned(ACS_BTEE), ReadOnly);
				ACS_Chars->Set(String::New("TTEE"), Uint32::NewFromUnsigned(ACS_TTEE), ReadOnly);
				ACS_Chars->Set(String::New("HLINE"), Uint32::NewFromUnsigned(ACS_HLINE), ReadOnly);
				ACS_Chars->Set(String::New("VLINE"), Uint32::NewFromUnsigned(ACS_VLINE), ReadOnly);
				ACS_Chars->Set(String::New("PLUS"), Uint32::NewFromUnsigned(ACS_PLUS), ReadOnly);
				ACS_Chars->Set(String::New("S1"), Uint32::NewFromUnsigned(ACS_S1), ReadOnly);
				ACS_Chars->Set(String::New("S9"), Uint32::NewFromUnsigned(ACS_S9), ReadOnly);
				ACS_Chars->Set(String::New("DIAMOND"), Uint32::NewFromUnsigned(ACS_DIAMOND), ReadOnly);
				ACS_Chars->Set(String::New("CKBOARD"), Uint32::NewFromUnsigned(ACS_CKBOARD), ReadOnly);
				ACS_Chars->Set(String::New("DEGREE"), Uint32::NewFromUnsigned(ACS_DEGREE), ReadOnly);
				ACS_Chars->Set(String::New("PLMINUS"), Uint32::NewFromUnsigned(ACS_PLMINUS), ReadOnly);
				ACS_Chars->Set(String::New("BULLET"), Uint32::NewFromUnsigned(ACS_BULLET), ReadOnly);
				ACS_Chars->Set(String::New("LARROW"), Uint32::NewFromUnsigned(ACS_LARROW), ReadOnly);
				ACS_Chars->Set(String::New("RARROW"), Uint32::NewFromUnsigned(ACS_RARROW), ReadOnly);
				ACS_Chars->Set(String::New("DARROW"), Uint32::NewFromUnsigned(ACS_DARROW), ReadOnly);
				ACS_Chars->Set(String::New("UARROW"), Uint32::NewFromUnsigned(ACS_UARROW), ReadOnly);
				ACS_Chars->Set(String::New("BOARD"), Uint32::NewFromUnsigned(ACS_BOARD), ReadOnly);
				ACS_Chars->Set(String::New("LANTERN"), Uint32::NewFromUnsigned(ACS_LANTERN), ReadOnly);
				ACS_Chars->Set(String::New("BLOCK"), Uint32::NewFromUnsigned(ACS_BLOCK), ReadOnly);
			}
		}

		MyPanel* panel() {
			return panel_;
		}
		
		void close() {
			if (panel_) {
				if (panel_->isStdscr())
					ev_io_stop(EV_DEFAULT_ &read_watcher_);
				delete panel_;
				panel_ = NULL;
			}
		}

	protected:
		static Handle<Value> New (const Arguments& args) {
			HandleScope scope;

			ncWindow *win;
			if (stdin_fd < 0)
				win = new ncWindow();
			else if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
				win = new ncWindow(args[0]->Int32Value(), args[1]->Int32Value(), 0, 0);
			else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsInt32())
				win = new ncWindow(args[0]->Int32Value(), args[1]->Int32Value(), args[2]->Int32Value(), 0);
			else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsInt32() && args[3]->IsInt32())
				win = new ncWindow(args[0]->Int32Value(), args[1]->Int32Value(), args[2]->Int32Value(), args[3]->Int32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			win->Wrap(args.This());
			return args.This();
		}

		static Handle<Value> Close (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;

			if (win->panel() != NULL) {
				win->close();
			}
			return Undefined();
		}
		
		static Handle<Value> Hide (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;

			win->panel()->hide();

			return Undefined();
		}
		
		static Handle<Value> Show (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;

			win->panel()->show();

			return Undefined();
		}
		
		static Handle<Value> Top (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;

			win->panel()->top();

			return Undefined();
		}
		
		static Handle<Value> Bottom (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;

			win->panel()->bottom();

			return Undefined();
		}
		
		static Handle<Value> Mvwin (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;

			int ret;
			if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
				win->panel()->mvwin(args[0]->Int32Value(), args[1]->Int32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Refresh (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;

			int ret = win->panel()->refresh();

			return scope.Close(Integer::New(ret));
		}
		
		static Handle<Value> Noutrefresh (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;

			int ret = win->panel()->noutrefresh();

			return scope.Close(Integer::New(ret));
		}
		
		static Handle<Value> Redraw (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;

			win->panel()->redraw();

			return Undefined();
		}
		
		static Handle<Value> Frame (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			if (args.Length() == 0) {
				win->panel()->frame(NULL, NULL);
			} else if (args.Length() == 1 && args[0]->IsString()) {
				String::Utf8Value str(args[0]->ToString());
			  win->panel()->frame(ToCString(str), NULL);
			} else if (args.Length() == 2 && args[0]->IsString() && args[1]->IsString()) {
			  String::Utf8Value str0(args[0]->ToString());
			  String::Utf8Value str1(args[0]->ToString());
				win->panel()->frame(ToCString(str0), ToCString(str1));
			} else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return Undefined();
		}

		static Handle<Value> Boldframe (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			if (args.Length() == 0) {
				win->panel()->boldframe(NULL, NULL);
			} else if (args.Length() == 1 && args[0]->IsString()) {
			  String::Utf8Value str(args[0]->ToString());
				win->panel()->boldframe(ToCString(str), NULL);
			} else if (args.Length() == 2 && args[0]->IsString() && args[1]->IsString()) {
			  String::Utf8Value str0(args[0]->ToString());
			  String::Utf8Value str1(args[0]->ToString());
				win->panel()->boldframe(ToCString(str0), ToCString(str1));
			} else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return Undefined();
		}
		
		static Handle<Value> Label (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			if (args.Length() == 1 && args[0]->IsString()) {
			  String::Utf8Value str(args[0]->ToString());
				win->panel()->label(ToCString(str), NULL);
			} else if (args.Length() == 2 && args[0]->IsString() && args[1]->IsString()) {
			  String::Utf8Value str0(args[0]->ToString());
			  String::Utf8Value str1(args[1]->ToString());
				win->panel()->label(ToCString(str0), ToCString(str1));
			} else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return Undefined();
		}
		
		static Handle<Value> Centertext (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsString()) {
			  String::Utf8Value str(args[1]->ToString());
				win->panel()->centertext(args[0]->Int32Value(), ToCString(str));
			} else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return Undefined();
		}
		
		static Handle<Value> Move (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
				ret = win->panel()->move(args[0]->Int32Value(), args[1]->Int32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}
		
		static Handle<Value> Addch (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsUint32())
				ret = win->panel()->addch(args[0]->Uint32Value());
			else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsUint32())
				ret = win->panel()->addch(args[0]->Int32Value(), args[1]->Int32Value(), args[2]->Uint32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}
		
		static Handle<Value> Echochar (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsUint32())
				ret = win->panel()->echochar(args[0]->Uint32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Addstr (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsString()) {
			  String::Utf8Value str(args[0]->ToString());
				ret = win->panel()->addstr(ToCString(str), -1);
			} else if (args.Length() == 2 && args[0]->IsString() && args[1]->IsInt32()) {
			  String::Utf8Value str(args[0]->ToString());
				ret = win->panel()->addstr(ToCString(str), args[1]->Int32Value());
			} else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsString()) {
			  String::Utf8Value str(args[2]->ToString());
				ret = win->panel()->addstr(args[0]->Int32Value(), args[1]->Int32Value(), ToCString(str), -1);
			} else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsString() && args[3]->IsInt32()) {
			  String::Utf8Value str(args[2]->ToString());
				ret = win->panel()->addstr(args[0]->Int32Value(), args[1]->Int32Value(), ToCString(str), args[3]->Int32Value());
			} else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}
		
		// FIXME: addchstr requires a pointer to a chtype not an actual value, unlike the other ACS_*-using methods
		/*static Handle<Value> Addchstr (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsUint32())
				ret = win->panel()->addchstr(args[0]->Uint32Value(), -1);
			else if (args.Length() == 2 && args[0]->IsUint32() && args[1]->IsInt32())
				ret = win->panel()->addchstr(args[0]->Uint32Value(), args[1]->Int32Value());
			else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsUint32())
				ret = win->panel()->addchstr(args[0]->Int32Value(), args[1]->Int32Value(), args[2]->Uint32Value(), -1);
			else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsUint32() && args[3]->IsInt32())
				ret = win->panel()->addchstr(args[0]->Int32Value(), args[1]->Int32Value(), args[2]->Uint32Value(), args[3]->Int32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}*/
			
		static Handle<Value> Inch (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			unsigned int ret;
			if (args.Length() == 0)
				ret = win->panel()->inch();
			else if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
				ret = win->panel()->inch(args[0]->Int32Value(), args[1]->Int32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::NewFromUnsigned(ret));
		}
		
		// FIXME: Need pointer to chtype instead of actual value
		/*static Handle<Value> Inchstr (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsUint32())
				ret = win->panel()->inchstr(args[0]->Uint32Value(), -1);
			else if (args.Length() == 2 && args[0]->IsUint32() && args[1]->IsInt32())
				ret = win->panel()->inchstr(args[0]->Uint32Value(), args[1]->Int32Value());
			else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsUint32())
				ret = win->panel()->inchstr(args[0]->Int32Value(), args[1]->Int32Value(), args[2]->Uint32Value(), -1);
			else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsUint32() && args[3]->IsInt32())
				ret = win->panel()->inchstr(args[0]->Int32Value(), args[1]->Int32Value(), args[2]->Uint32Value(), args[3]->Int32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}*/

		static Handle<Value> Insch (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsUint32())
				ret = win->panel()->insch(args[0]->Uint32Value());
			else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsUint32())
				ret = win->panel()->insch(args[0]->Int32Value(), args[1]->Int32Value(), args[2]->Uint32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Insertln (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret = win->panel()->insertln();

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Insdelln (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 0)
				ret = win->panel()->insdelln(1);
			else if (args.Length() == 1 && args[0]->IsInt32())
				ret = win->panel()->insdelln(args[0]->Int32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Insstr (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsString()) {
        String::Utf8Value str(args[0]->ToString());
				ret = win->panel()->insstr(ToCString(str), -1);
			} else if (args.Length() == 2 && args[0]->IsString() && args[1]->IsInt32()) {
        String::Utf8Value str(args[0]->ToString());
				ret = win->panel()->insstr(ToCString(str), args[1]->Int32Value());
			} else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsString()) {
			  String::Utf8Value str(args[2]->ToString());
				ret = win->panel()->insstr(args[0]->Int32Value(), args[1]->Int32Value(), ToCString(str), -1);
			} else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsString() && args[3]->IsInt32()) {
			  String::Utf8Value str(args[2]->ToString());
				ret = win->panel()->insstr(args[0]->Int32Value(), args[1]->Int32Value(), ToCString(str), args[3]->Int32Value());
			} else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Attron (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsUint32())
				ret = win->panel()->attron(args[0]->Uint32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Attroff (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsUint32())
				ret = win->panel()->attroff(args[0]->Uint32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Attrset (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsUint32())
				ret = win->panel()->attrset(args[0]->Uint32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Attrget (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			unsigned int ret = win->panel()->attrget();

			return scope.Close(Integer::NewFromUnsigned(ret));
		}

		static Handle<Value> Box (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 0)
				ret = win->panel()->box(0, 0);
			else if (args.Length() == 1 && args[0]->IsUint32())
				ret = win->panel()->box(args[0]->Uint32Value(), 0);
			else if (args.Length() == 2 && args[0]->IsUint32() && args[1]->IsUint32())
				ret = win->panel()->box(args[0]->Uint32Value(), args[1]->Uint32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Border (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 0)
				ret = win->panel()->border(0);
			else if (args.Length() == 1 && args[0]->IsUint32())
				ret = win->panel()->border(args[0]->Uint32Value(), 0);
			else if (args.Length() == 2 && args[0]->IsUint32() && args[1]->IsUint32())
				ret = win->panel()->border(args[0]->Uint32Value(), args[1]->Uint32Value(), 0);
			else if (args.Length() == 3 && args[0]->IsUint32() && args[1]->IsUint32() && args[2]->IsUint32())
				ret = win->panel()->border(args[0]->Uint32Value(), args[1]->Uint32Value(), args[2]->Uint32Value(), 0);
			else if (args.Length() == 4 && args[0]->IsUint32() && args[1]->IsUint32() && args[2]->IsUint32() && args[3]->IsUint32())
				ret = win->panel()->border(args[0]->Uint32Value(), args[1]->Uint32Value(), args[2]->Uint32Value(), args[3]->Uint32Value(), 0);
			else if (args.Length() == 5 && args[0]->IsUint32() && args[1]->IsUint32() && args[2]->IsUint32() && args[3]->IsUint32() && args[4]->IsUint32())
				ret = win->panel()->border(args[0]->Uint32Value(), args[1]->Uint32Value(), args[2]->Uint32Value(), args[3]->Uint32Value(), args[4]->Uint32Value(), 0);
			else if (args.Length() == 6 && args[0]->IsUint32() && args[1]->IsUint32() && args[2]->IsUint32() && args[3]->IsUint32() && args[4]->IsUint32() && args[5]->IsUint32())
				ret = win->panel()->border(args[0]->Uint32Value(), args[1]->Uint32Value(), args[2]->Uint32Value(), args[3]->Uint32Value(), args[4]->Uint32Value(), args[5]->Uint32Value(), 0);
			else if (args.Length() == 7 && args[0]->IsUint32() && args[1]->IsUint32() && args[2]->IsUint32() && args[3]->IsUint32() && args[4]->IsUint32() && args[5]->IsUint32() && args[6]->IsUint32())
				ret = win->panel()->border(args[0]->Uint32Value(), args[1]->Uint32Value(), args[2]->Uint32Value(), args[3]->Uint32Value(), args[4]->Uint32Value(), args[5]->Uint32Value(), args[6]->Uint32Value(), 0);
			else if (args.Length() == 8 && args[0]->IsUint32() && args[1]->IsUint32() && args[2]->IsUint32() && args[3]->IsUint32() && args[4]->IsUint32() && args[5]->IsUint32() && args[6]->IsUint32() && args[7]->IsUint32())
				ret = win->panel()->border(args[0]->Uint32Value(), args[1]->Uint32Value(), args[2]->Uint32Value(), args[3]->Uint32Value(), args[4]->Uint32Value(), args[5]->Uint32Value(), args[6]->Uint32Value(), args[7]->Uint32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Hline (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsInt32())
				ret = win->panel()->hline(args[0]->Int32Value(), 0);
			else if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsUint32())
				ret = win->panel()->hline(args[0]->Int32Value(), args[1]->Uint32Value());
			else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsInt32())
				ret = win->panel()->hline(args[0]->Int32Value(), args[1]->Int32Value(), args[2]->Int32Value(), 0);
			else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsInt32() && args[3]->IsUint32())
				ret = win->panel()->hline(args[0]->Int32Value(), args[1]->Int32Value(), args[2]->Int32Value(), args[3]->Uint32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Vline (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsInt32())
				ret = win->panel()->vline(args[0]->Int32Value(), 0);
			else if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsUint32())
				ret = win->panel()->vline(args[0]->Int32Value(), args[1]->Uint32Value());
			else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsInt32())
				ret = win->panel()->vline(args[0]->Int32Value(), args[1]->Int32Value(), args[2]->Int32Value(), 0);
			else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsInt32() && args[3]->IsUint32())
				ret = win->panel()->vline(args[0]->Int32Value(), args[1]->Int32Value(), args[2]->Int32Value(), args[3]->Uint32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Erase (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret = win->panel()->erase();

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Clear (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret = win->panel()->clear();

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Clrtobot (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret = win->panel()->clrtobot();

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Clrtoeol (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret = win->panel()->clrtoeol();

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Delch (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 0)
				ret = win->panel()->delch();
			else if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
				ret = win->panel()->delch(args[0]->Int32Value(), args[1]->Int32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Deleteln (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret = win->panel()->deleteln();

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Scroll (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 0)
				ret = win->panel()->scroll(1);
			else if (args.Length() == 1 && args[0]->IsInt32())
				ret = win->panel()->scroll(args[0]->Int32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Setscrreg (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
				ret = win->panel()->setscrreg(args[0]->Int32Value(), args[1]->Int32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		/*static Handle<Value> Touchline (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
				ret = win->panel()->touchline(args[0]->Int32Value(), args[1]->Int32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}*/

		static Handle<Value> Touchwin (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret = win->panel()->touchwin();

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Untouchwin (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret = win->panel()->untouchwin();

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Touchln (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
				ret = win->panel()->touchln(args[0]->Int32Value(), args[1]->Int32Value(), true);
			else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsBoolean())
				ret = win->panel()->touchln(args[0]->Int32Value(), args[1]->Int32Value(), args[2]->BooleanValue());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Is_linetouched (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			bool ret;
			if (args.Length() == 1 && args[0]->IsInt32())
				ret = win->panel()->is_linetouched(args[0]->Int32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Boolean::New(ret));
		}

		static Handle<Value> Redrawln (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
				ret = win->panel()->redrawln(args[0]->Int32Value(), args[1]->Int32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Syncdown (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			win->panel()->syncdown();

			return Undefined();
		}

		static Handle<Value> Syncup (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			win->panel()->syncup();

			return Undefined();
		}

		static Handle<Value> Cursyncup (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			win->panel()->cursyncup();

			return Undefined();
		}

		static Handle<Value> Wresize (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
				ret = win->panel()->wresize(args[0]->Int32Value(), args[1]->Int32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Print (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsString()) {
        String::Utf8Value str(args[0]->ToString());
				ret = win->panel()->printw("%s", ToCString(str));
			} else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsString()) { 
        String::Utf8Value str(args[0]->ToString());
				ret = win->panel()->printw(args[0]->Int32Value(), args[1]->Int32Value(), "%s", ToCString(str));
			} else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Clearok (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsBoolean())
				ret = win->panel()->clearok(args[0]->BooleanValue());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Scrollok (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsBoolean())
				ret = win->panel()->scrollok(args[0]->BooleanValue());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Idlok (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsBoolean())
				ret = win->panel()->idlok(args[0]->BooleanValue());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Idcok (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			if (args.Length() == 1 && args[0]->IsBoolean())
				win->panel()->idcok(args[0]->BooleanValue());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return Undefined();
		}

		static Handle<Value> Leaveok (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsBoolean())
				ret = win->panel()->leaveok(args[0]->BooleanValue());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Syncok (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsBoolean())
				ret = win->panel()->syncok(args[0]->BooleanValue());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Immedok (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			if (args.Length() == 1 && args[0]->IsBoolean())
				win->panel()->immedok(args[0]->BooleanValue());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return Undefined();
		}

		static Handle<Value> Keypad (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsBoolean())
				ret = win->panel()->keypad(args[0]->BooleanValue());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Meta (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsBoolean())
				ret = win->panel()->meta(args[0]->BooleanValue());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Standout (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsBoolean()) {
				if (args[0]->BooleanValue())
					ret = win->panel()->standout();
				else
					ret = win->panel()->standend();
			} else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Colorpair (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsInt32())
				ret = win->panel()->pair(args[0]->Int32Value());
			else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsInt32())
				ret = win->panel()->pair(args[0]->Int32Value(), (short)args[1]->Int32Value(), (short)args[2]->Int32Value());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}

		static Handle<Value> Resetscreen (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			win->panel()->resetScreen();

			return Undefined();
		}

		// -- Getters/Setters -----------------------------------------------------------------------
		static Handle<Value> EchoStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == ECHO_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Boolean::New(MyPanel::echo()));
		}
		
		static void EchoStateSetter (Local<String> property, Local<Value> value, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == ECHO_STATE_SYMBOL);

			if (!value->IsBoolean()) {
				ThrowException(Exception::TypeError(
					String::New("echo should be of Boolean value")
				));
			}

			MyPanel::echo(value->BooleanValue());
		}

		static Handle<Value> ShowcursorStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == SHOWCURSOR_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Boolean::New(MyPanel::showCursor()));
		}
		
		static void ShowcursorStateSetter (Local<String> property, Local<Value> value, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == SHOWCURSOR_STATE_SYMBOL);

			if (!value->IsBoolean()) {
				ThrowException(Exception::TypeError(
					String::New("showCursor should be of Boolean value")
				));
			}

			MyPanel::showCursor(value->BooleanValue());
		}

		static Handle<Value> LinesStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == LINES_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::New(win->panel()->lines()));
		}

		static Handle<Value> ColsStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == COLS_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::New(win->panel()->cols()));
		}

		static Handle<Value> TabsizeStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == TABSIZE_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::New(win->panel()->tabsize()));
		}

		static Handle<Value> HasmouseStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == HASMOUSE_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::New(win->panel()->has_mouse()));
		}

		static Handle<Value> HiddenStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == HIDDEN_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Boolean::New(win->panel()->hidden()));
		}

		static Handle<Value> HeightStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == HEIGHT_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::New(win->panel()->height()));
		}

		static Handle<Value> WidthStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == WIDTH_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::New(win->panel()->width()));
		}

		static Handle<Value> BegxStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == BEGX_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::New(win->panel()->begx()));
		}

		static Handle<Value> BegyStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == BEGY_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::New(win->panel()->begy()));
		}

		static Handle<Value> CurxStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == CURX_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::New(win->panel()->curx()));
		}

		static Handle<Value> CuryStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == CURY_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::New(win->panel()->cury()));
		}

		static Handle<Value> MaxxStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == MAXX_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::New(win->panel()->maxx()));
		}

		static Handle<Value> MaxyStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == MAXY_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::New(win->panel()->maxy()));
		}

		static Handle<Value> BkgdStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == BKGD_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::NewFromUnsigned(win->panel()->getbkgd()));
		}

		static void BkgdStateSetter (Local<String> property, Local<Value> value, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == BKGD_STATE_SYMBOL);

			if (!value->IsUint32()) {
				ThrowException(Exception::TypeError(
					String::New("bkgd should be of unsigned integer value")
				));
			}
			
			win->panel()->bkgd(value->Uint32Value());
		}

		static Handle<Value> HascolorsStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == HASCOLORS_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Boolean::New(MyPanel::has_colors()));
		}

		static Handle<Value> NumcolorpairsStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == NUMCOLORPAIRS_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::New(MyPanel::num_pairs()));
		}

		static Handle<Value> MaxcolorpairsStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == MAXCOLORPAIRS_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::New(MyPanel::max_pairs()));
		}

		static Handle<Value> WintouchedStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == WINTOUCHED_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Boolean::New(win->panel()->is_wintouched()));
		}

		static Handle<Value> ACSConstsGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == ACS_CONSTS_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(ACS_Chars);
		}

		ncWindow() : EventEmitter() {
			panel_ = NULL;
			this->init();
			assert(panel_ != NULL);
		}
		
		ncWindow(int nlines, int ncols, int begin_y, int begin_x) : EventEmitter() {
			panel_ = NULL;
			this->init(nlines, ncols, begin_y, begin_x);
			assert(panel_ != NULL);
		}
		
		~ncWindow() {
			if (panel_) {
				this->close();
			}
			assert(panel_ == NULL);
		}
		
	private:
		void Event (int revents) {
			if (revents & EV_ERROR)
				return;

			if (revents & EV_READ) {
				HandleScope scope;

				int chr;
				char tmp[2];
				tmp[1] = 0;
				while ((chr = this->panel()->getch()) != ERR) {
					tmp[0] = chr;
					Local<Value> vChr[2];
					vChr[0] = String::New(tmp);
					vChr[1] = Integer::New(chr);
					Emit(inputChar_symbol, 2, vChr);
				}
			}
		}

		static void
		io_event (EV_P_ ev_io *w, int revents) {
			ncWindow *win = static_cast<ncWindow*>(w->data);
			win->Event(revents);
		}
		
		MyPanel *panel_;
		ev_io read_watcher_;
};

extern "C" void
init (Handle<Object> target) {
	HandleScope scope;
	ncWindow::Initialize(target);
}