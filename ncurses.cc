#include "internal.h"
#include <cursesp.h>

#include <node.h>
#include <node_events.h>

//#include <fstream>
//#include <stdio.h>

#include <string>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>

using namespace std;
using namespace v8;
using namespace node;

static int stdin_fd = -1;
static int stdout_fd = -1;

static Persistent<String> inputChar_symbol;
static Persistent<String> inputLine_symbol;
#define ECHO_STATE_SYMBOL String::New("echoInput")
#define LINES_STATE_SYMBOL String::New("lines")
#define COLS_STATE_SYMBOL String::New("cols")
#define TABSIZE_STATE_SYMBOL String::New("tabsize")
#define NUMCOLORS_STATE_SYMBOL String::New("numcolors")
#define HASMOUSE_STATE_SYMBOL String::New("hasmouse")
#define HIDDEN_STATE_SYMBOL String::New("hidden")
#define HEIGHT_STATE_SYMBOL String::New("height")
#define WIDTH_STATE_SYMBOL String::New("width")
#define BEGX_STATE_SYMBOL String::New("begx")
#define BEGY_STATE_SYMBOL String::New("begy")
#define CURX_STATE_SYMBOL String::New("curx")
#define CURY_STATE_SYMBOL String::New("cury")
#define MAXX_STATE_SYMBOL String::New("maxx")
#define MAXY_STATE_SYMBOL String::New("maxy")
#define BKGD_STATE_SYMBOL String::New("bkgd")
/*#define COLOR_STATE_SYMBOL String::New("color")
#define FGCOLOR_STATE_SYMBOL String::New("fgcolor")
#define BGCOLOR_STATE_SYMBOL String::New("bgcolor")*/

// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
	return *value ? *value : "<string conversion failed>";
}

class MyPanel : public NCursesPanel {
	private:
		void setup() {
			::nodelay(w, true);
			::nocbreak();
			::halfdelay(1);
		}
		static bool echoInput;
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
			echoInput = value;
			if (echoInput)
				echo();
			else
				noecho();
		}
		static bool echo() {
			return echoInput;
		}
		bool isStdscr() {
			return (w == ::stdscr);
		}
};

class ncWindow : public EventEmitter {
	public:
		static void	Initialize (Handle<Object> target) {
			HandleScope scope;
			
			Local<FunctionTemplate> t = FunctionTemplate::New(New);
			
			t->Inherit(EventEmitter::constructor_template);
			t->InstanceTemplate()->SetInternalFieldCount(1);
			
			inputChar_symbol = NODE_PSYMBOL("inputChar");
			inputLine_symbol = NODE_PSYMBOL("inputLine");

			/* Panel-specific methods */
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
			
			/* ACS_* character-related methods */
			NODE_SET_PROTOTYPE_METHOD(t, "addch", Addch);
			NODE_SET_PROTOTYPE_METHOD(t, "echochar", Echochar);
			NODE_SET_PROTOTYPE_METHOD(t, "addstr", Addstr);
			//NODE_SET_PROTOTYPE_METHOD(t, "addchstr", Addchstr);
			NODE_SET_PROTOTYPE_METHOD(t, "inch", Inch);
			//NODE_SET_PROTOTYPE_METHOD(t, "inchstr", Inchstr);
			NODE_SET_PROTOTYPE_METHOD(t, "insch", Insch);
			
			NODE_SET_PROTOTYPE_METHOD(t, "insertln", Insertln);
			NODE_SET_PROTOTYPE_METHOD(t, "insdelln", Insdelln);
			NODE_SET_PROTOTYPE_METHOD(t, "insstr", Insstr);
			
			NODE_SET_PROTOTYPE_METHOD(t, "attron", Attron);
			NODE_SET_PROTOTYPE_METHOD(t, "attroff", Attroff);
			NODE_SET_PROTOTYPE_METHOD(t, "attrset", Attrset);
			NODE_SET_PROTOTYPE_METHOD(t, "attrget", Attrget);

			// TODO: color_set, chgat
			/*NODE_SET_PROTOTYPE_METHOD(t, "color_set", Color_set);
			NODE_SET_PROTOTYPE_METHOD(t, "chgat", Chgat);*/
			
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
			NODE_SET_PROTOTYPE_METHOD(t, "touchline", Touchline); // one line
			NODE_SET_PROTOTYPE_METHOD(t, "touchwin", Touchwin);
			NODE_SET_PROTOTYPE_METHOD(t, "untouchwin", Untouchwin);
			NODE_SET_PROTOTYPE_METHOD(t, "touchln", Touchln); // multiple lines
			NODE_SET_PROTOTYPE_METHOD(t, "is_linetouched", Is_linetouched);
			NODE_SET_PROTOTYPE_METHOD(t, "is_wintouched", Is_wintouched);
			NODE_SET_PROTOTYPE_METHOD(t, "redrawln", Redrawln);
			NODE_SET_PROTOTYPE_METHOD(t, "syncdown", Syncdown);
			NODE_SET_PROTOTYPE_METHOD(t, "syncup", Syncup);
			NODE_SET_PROTOTYPE_METHOD(t, "cursyncup", Cursyncup);

			// TODO: overlay, overwrite, copywin
			NODE_SET_PROTOTYPE_METHOD(t, "resize", Wresize);
			
			NODE_SET_PROTOTYPE_METHOD(t, "print", Print);
			NODE_SET_PROTOTYPE_METHOD(t, "close", Close);

			NODE_SET_PROTOTYPE_METHOD(t, "clearok", Clearok);
			NODE_SET_PROTOTYPE_METHOD(t, "scrollok", Scrollok);
			NODE_SET_PROTOTYPE_METHOD(t, "idlok", Idlok);
			NODE_SET_PROTOTYPE_METHOD(t, "idcok", Idcok);
			NODE_SET_PROTOTYPE_METHOD(t, "leaveok", Leaveok);
			NODE_SET_PROTOTYPE_METHOD(t, "syncok", Syncok);
			//NODE_SET_PROTOTYPE_METHOD(t, "flushok", Flushok);
			NODE_SET_PROTOTYPE_METHOD(t, "immedok", Immedok);
			//NODE_SET_PROTOTYPE_METHOD(t, "initrflush", Initrflush);
			NODE_SET_PROTOTYPE_METHOD(t, "keypad", Keypad);
			NODE_SET_PROTOTYPE_METHOD(t, "meta", Meta);
			NODE_SET_PROTOTYPE_METHOD(t, "standout", Standout);

			// Terminal settings
			/*t->PrototypeTemplate()->SetAccessor(LINES_STATE_SYMBOL, LinesStateGetter);
			t->PrototypeTemplate()->SetAccessor(COLS_STATE_SYMBOL, ColsStateGetter);
			t->PrototypeTemplate()->SetAccessor(TABSIZE_STATE_SYMBOL, TabsizeStateGetter);
			t->PrototypeTemplate()->SetAccessor(NUMCOLORS_STATE_SYMBOL, NumcolorsStateGetter);
			t->PrototypeTemplate()->SetAccessor(HASMOUSE_STATE_SYMBOL, HasmouseStateGetter);

			// Panel/Window settings
			t->PrototypeTemplate()->SetAccessor(HIDDEN_STATE_SYMBOL, HiddenStateGetter);
			t->PrototypeTemplate()->SetAccessor(HEIGHT_STATE_SYMBOL, HeightStateGetter);
			t->PrototypeTemplate()->SetAccessor(WIDTH_STATE_SYMBOL, WidthStateGetter);
			t->PrototypeTemplate()->SetAccessor(BEGX_STATE_SYMBOL, BegxStateGetter);
			t->PrototypeTemplate()->SetAccessor(BEGY_STATE_SYMBOL, BegyStateGetter);
			t->PrototypeTemplate()->SetAccessor(CURX_STATE_SYMBOL, CurxStateGetter);
			t->PrototypeTemplate()->SetAccessor(CURY_STATE_SYMBOL, CuryStateGetter);
			t->PrototypeTemplate()->SetAccessor(MAXX_STATE_SYMBOL, MaxxStateGetter);
			t->PrototypeTemplate()->SetAccessor(MAXY_STATE_SYMBOL, MaxyStateGetter);
			t->PrototypeTemplate()->SetAccessor(BKGD_STATE_SYMBOL, BkgdStateGetter, BkgdStateSetter);*/
			// TODO: color, fgcolor, bgcolor
			/*t->PrototypeTemplate()->SetAccessor(COLOR_STATE_SYMBOL, ColorStateGetter);
			t->PrototypeTemplate()->SetAccessor(FGCOLOR_STATE_SYMBOL, FgcolorStateGetter);
			t->PrototypeTemplate()->SetAccessor(BGCOLOR_STATE_SYMBOL, BgcolorStateGetter);*/
			
			target->Set(String::NewSymbol("ncWindow"), t->GetFunction());
		}
		
		void init() {
			if (stdin_fd < 0) {
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
			panel_ = new MyPanel();
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

		/* Debug logging functions */
		/*static void log(const char* message) {
			ofstream outfile("log", ios::out | ios::app);
			outfile << message << endl;
			outfile.close();
		}

		static void log(int val) {
			ofstream outfile("log", ios::out | ios::app);
			outfile << val << endl;
			outfile.close();
		}
		
		static void log(char val) {
			ofstream outfile("log", ios::out | ios::app);
			outfile << val << endl;
			outfile.close();
		}

		static string int2str(int val) {
			char buf[256];
			sprintf(buf, "%i", val);
			return string(buf);
		}*/

	protected:
		static Handle<Value> New (const Arguments& args) {
			HandleScope scope;

			//if (stdin_fd < 0)
				ncWindow *win = new ncWindow();
			//else if (args.Length() == 2
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
			
			if (args.Length() == 0)
				win->panel()->frame(NULL, NULL);
			else if (args.Length() == 1 && args[0]->IsString())
				win->panel()->frame(ToCString(String::Utf8Value(args[0]->ToString())), NULL);
			else if (args.Length() == 2 && args[0]->IsString() && args[1]->IsString())
				win->panel()->frame(ToCString(String::Utf8Value(args[0]->ToString())), ToCString(String::Utf8Value(args[1]->ToString())));
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return Undefined();
		}

		static Handle<Value> Boldframe (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			if (args.Length() == 0)
				win->panel()->boldframe(NULL, NULL);
			else if (args.Length() == 1 && args[0]->IsString())
				win->panel()->boldframe(ToCString(String::Utf8Value(args[0]->ToString())), NULL);
			else if (args.Length() == 2 && args[0]->IsString() && args[1]->IsString())
				win->panel()->boldframe(ToCString(String::Utf8Value(args[0]->ToString())), ToCString(String::Utf8Value(args[1]->ToString())));
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return Undefined();
		}
		
		static Handle<Value> Label (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			if (args.Length() == 1 && args[0]->IsString())
				win->panel()->label(ToCString(String::Utf8Value(args[0]->ToString())), NULL);
			else if (args.Length() == 2 && args[0]->IsString() && args[1]->IsString())
				win->panel()->label(ToCString(String::Utf8Value(args[0]->ToString())), ToCString(String::Utf8Value(args[1]->ToString())));
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return Undefined();
		}
		
		static Handle<Value> Centertext (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsString())
				win->panel()->centertext(args[0]->Int32Value(), ToCString(String::Utf8Value(args[1]->ToString())));
			else {
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
			if (args.Length() == 1 && args[0]->IsString())
				ret = win->panel()->addstr(ToCString(String::Utf8Value(args[0]->ToString())), -1);
			else if (args.Length() == 2 && args[0]->IsString() && args[1]->IsInt32())
				ret = win->panel()->addstr(ToCString(String::Utf8Value(args[0]->ToString())), args[1]->Int32Value());
			else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsString())
				ret = win->panel()->addstr(args[0]->Int32Value(), args[1]->Int32Value(), ToCString(String::Utf8Value(args[2]->ToString())), -1);
			else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsString() && args[3]->IsInt32())
				ret = win->panel()->addstr(args[0]->Int32Value(), args[1]->Int32Value(), ToCString(String::Utf8Value(args[2]->ToString())), args[3]->Int32Value());
			else {
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
			if (args.Length() == 1 && args[0]->IsString())
				ret = win->panel()->insstr(ToCString(String::Utf8Value(args[0]->ToString())), -1);
			else if (args.Length() == 2 && args[0]->IsString() && args[1]->IsInt32())
				ret = win->panel()->insstr(ToCString(String::Utf8Value(args[0]->ToString())), args[1]->Int32Value());
			else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsString())
				ret = win->panel()->insstr(args[0]->Int32Value(), args[1]->Int32Value(), ToCString(String::Utf8Value(args[2]->ToString())), -1);
			else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsString() && args[3]->IsInt32())
				ret = win->panel()->insstr(args[0]->Int32Value(), args[1]->Int32Value(), ToCString(String::Utf8Value(args[2]->ToString())), args[3]->Int32Value());
			else {
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

		static Handle<Value> Touchline (const Arguments& args) {
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
		}

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

		static Handle<Value> Is_wintouched (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			bool ret = win->panel()->is_wintouched();

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
			if (args.Length() == 1 && args[0]->IsString())
				ret = win->panel()->printw("%s", ToCString(String::Utf8Value(args[0]->ToString())));
			else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsString())
				ret = win->panel()->printw(args[0]->Int32Value(), args[1]->Int32Value(), "%s", ToCString(String::Utf8Value(args[2]->ToString())));
			else {
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

		/*static Handle<Value> Flushok (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsBoolean())
				ret = win->panel()->flushok(args[0]->BooleanValue());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}*/

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

		/*static Handle<Value> Initrflush (const Arguments& args) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(args.This());
			HandleScope scope;
			
			int ret;
			if (args.Length() == 1 && args[0]->IsBoolean())
				ret = win->panel()->initrflush(args[0]->BooleanValue());
			else {
				return ThrowException(Exception::Error(
					String::New("Invalid number and/or types of arguments")
				));
			}

			return scope.Close(Integer::New(ret));
		}*/

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

		// Getters/Setters
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
					String::New("echoInput should be of Boolean value")
				));
			}
			
			MyPanel::echo(value->BooleanValue());
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

		static Handle<Value> NumcolorsStateGetter (Local<String> property, const AccessorInfo& info) {
			ncWindow *win = ObjectWrap::Unwrap<ncWindow>(info.This());
			assert(win);
			assert(property == NUMCOLORS_STATE_SYMBOL);
			
			HandleScope scope;
			
			return scope.Close(Integer::New(win->panel()->colors()));
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

		ncWindow() : EventEmitter() {
			panel_ = NULL;
			this->init();
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

				char chr;
				string tmp;
				while ((chr = getch()) != ERR) {
					tmp.clear();
					tmp += chr;
					Local<Value> vChr[1];
					vChr[0] = String::New(tmp.c_str());
					Emit(inputChar_symbol, 1, vChr);
					if (chr == '\n') {
						Local<Value> vLine[1];
						vLine[0] = String::New(curInput_.c_str());
						Emit(inputLine_symbol, 1, vLine);
						curInput_.clear();
					} else
						curInput_ += chr;
				}
			}
		}

		static void
		io_event (EV_P_ ev_io *w, int revents) {
			ncWindow *win = static_cast<ncWindow*>(w->data);
			win->Event(revents);
		}
		
		MyPanel *panel_;

		string curInput_;
		ev_io read_watcher_;
};

extern "C" void
init (Handle<Object> target) {
	HandleScope scope;
	ncWindow::Initialize(target);
}