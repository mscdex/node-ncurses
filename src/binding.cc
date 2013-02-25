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

#include <ncurses_cfg.h>
#include <cursesp.h>
#include <node.h>
#include <node_object_wrap.h>

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std;
using namespace v8;
using namespace node;

class MyPanel;
class Window;

static int stdin_fd = -1;

typedef struct {
  void* prev; // panel_node
  void* next; // panel_node
  MyPanel* panel;
  Window* window;
} panel_node;

static panel_node* head_panel = NULL;
static MyPanel* topmost_panel = NULL;

static Persistent<FunctionTemplate> window_constructor;
static Persistent<String> emit_symbol;
static Persistent<String> inputchar_symbol;
static Persistent<String> echo_state_symbol;
static Persistent<String> showcursor_state_symbol;
static Persistent<String> lines_state_symbol;
static Persistent<String> cols_state_symbol;
static Persistent<String> tabsize_state_symbol;
static Persistent<String> hasmouse_state_symbol;
static Persistent<String> hascolors_state_symbol;
static Persistent<String> numcolors_state_symbol;
static Persistent<String> maxcolorpairs_state_symbol;
static Persistent<String> raw_state_symbol;
static Persistent<String> bkgd_state_symbol;
static Persistent<String> hidden_state_symbol;
static Persistent<String> height_state_symbol;
static Persistent<String> width_state_symbol;
static Persistent<String> begx_state_symbol;
static Persistent<String> begy_state_symbol;
static Persistent<String> curx_state_symbol;
static Persistent<String> cury_state_symbol;
static Persistent<String> maxx_state_symbol;
static Persistent<String> maxy_state_symbol;
static Persistent<String> wintouched_state_symbol;
static Persistent<String> numwins_symbol;
static Persistent<String> ACS_symbol;
static Persistent<String> keys_symbol;
static Persistent<String> colors_symbol;
static Persistent<String> attrs_symbol;
static Persistent<Object> ACS_Chars;
static Persistent<Object> Keys;
static Persistent<Object> Attrs;
static Persistent<Object> Colors;

// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

static int wincounter = 0;
uv_poll_t* read_watcher_;

class MyPanel : public NCursesPanel {
  private:
    void setup() {
      if (head_panel) {
        panel_node* cur = head_panel;
        while (cur) {
          if (cur->next)
            cur = (panel_node*)(cur->next);
          else {
            cur->next = new panel_node;
            ((panel_node*)(cur->next))->next = NULL;
            ((panel_node*)(cur->next))->prev = cur;
            ((panel_node*)(cur->next))->panel = this;
            ((panel_node*)(cur->next))->window = this->assocwin;
            break;
          }
        }
      } else {
        head_panel = new panel_node;
        head_panel->prev = NULL;
        head_panel->next = NULL;
        head_panel->panel = this;
        head_panel->window = this->assocwin;
      }
      // Initialize color support if it's available
      if (::has_colors())
        ::start_color();

      // Set non-blocking mode and tell ncurses we want to receive one character
      // at a time instead of one line at a time
      ::nodelay(w, true);
      ::cbreak();
      ::keypad(w, true);

      // Setup a default palette
      if (w == ::stdscr && ::has_colors() && COLORS > 1) {
        short fore, back;
        ::pair_content(0, &fore, &back);
        for (int color=0,i=1; i<COLOR_PAIRS; i++) {
          ::init_pair(i, color++, back);
          if (color == COLORS)
            color = 0;
        }
      }

      // Set the window's default color pair
      ::wcolor_set(w, 0, NULL);

      topmost_panel = this;
      ++wincounter;
    }
    static bool echoInput_;
    static bool showCursor_;
    static bool isRaw_;
    Window* assocwin;
  public:
    MyPanel(Window* win, int nlines, int ncols, int begin_y = 0, int begin_x = 0)
      : NCursesPanel(nlines,ncols,begin_y,begin_x), assocwin(win) {
      this->setup();
    }
    MyPanel(Window* win) : NCursesPanel(), assocwin(win) {
      this->setup();
    }
    ~MyPanel() {
      panel_node* cur = head_panel;
      while (cur) {
        if (cur->panel == this) {
          if (cur->next)
            ((panel_node*)(cur->next))->prev = cur->prev;
          if (cur->prev)
            ((panel_node*)(cur->prev))->next = cur->next;
          if (cur == head_panel)
            head_panel = NULL;
          delete cur;
          --wincounter;
          break;
        } else
          cur = (panel_node*)(cur->next);
      }
    }
    PANEL* getPanel() {
      return p;
    }
    Window* getWindow() {
      return assocwin;
    }
    static void updateTopmost() {
      PANEL *pan, *panLastVis = NULL;
      panel_node *cur = head_panel;

      pan = ::panel_above(NULL);
      while (pan) {
        if (!(::panel_hidden(pan)))
          panLastVis = pan;
        if (::panel_above(pan))
          pan = ::panel_above(pan);
        else
          break;
      }
      if (!panLastVis)
        topmost_panel = NULL;
      else {
        while (cur) {
          if (cur->panel->getPanel() == panLastVis) {
            topmost_panel = cur->panel;
            break;
          } else
            cur = (panel_node*)(cur->next);
        }
      }
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
    static void raw(bool value) {
      MyPanel::isRaw_ = value;
      if (MyPanel::isRaw_)
        ::raw();
      else
        ::noraw();
    }
    static bool raw() {
      return MyPanel::isRaw_;
    }
    bool isStdscr() {
      return (w == ::stdscr);
    }
    static bool has_colors() {
      return ::has_colors();
    }
    static int num_colors() {
      return COLORS;
    }
    static int max_pairs() {
      // at least with xterm-256color, ncurses exhibits unexpected video behavior
      // with color pair numbers greater than 255 :-(
      if (COLOR_PAIRS > 256)
        return 256;
      else
        return COLOR_PAIRS;
    }
    static unsigned int pair(int pair, short fore=-1, short back=-1) {
      if (::has_colors()) {
        if (fore == -1 && back == -1)
          return COLOR_PAIR(pair);
        else {
          ::init_pair(pair, fore, back);
          return COLOR_PAIR(pair);
        }
      }
      return 0;
    }
    static void setFgcolor(int pair, short color) {
      short fore, back;
      if (::pair_content(pair, &fore, &back) != ERR)
        ::init_pair(pair, color, back);
    }
    static void setBgcolor(int pair, short color) {
      short fore, back;
      if (::pair_content(pair, &fore, &back) != ERR)
        ::init_pair(pair, fore, color);
    }
    static short getFgcolor(int pair) {
      short fore, back;
      if (::pair_content(pair, &fore, &back) != ERR)
        return fore;
      else
        return -1;
    }
    static short getBgcolor(int pair) {
      short fore, back;
      if (::pair_content(pair, &fore, &back) != ERR)
        return back;
      else
        return -1;
    }
    void hide() {
      NCursesPanel::hide();
      MyPanel::updateTopmost();
    }
    void show() {
      NCursesPanel::show();
      MyPanel::updateTopmost();
    }
    void top() {
      NCursesPanel::top();
      MyPanel::updateTopmost();
    }
    void bottom() {
      NCursesPanel::bottom();
      MyPanel::updateTopmost();
    }
    static void doUpdate() {
      ::doupdate();
    }
    static bool hasMouse() {
      return (::has_key(KEY_MOUSE) || ::has_mouse() ? TRUE : FALSE);
    }
};

bool MyPanel::echoInput_ = false;
bool MyPanel::showCursor_ = true;
bool MyPanel::isRaw_ = false;

class Window : public ObjectWrap {
  public:
    Persistent<Function> Emit;
    static void  Initialize (Handle<Object> target) {
      HandleScope scope;

      Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
      Local<String> name = String::NewSymbol("Window");

      window_constructor = Persistent<FunctionTemplate>::New(tpl);
      window_constructor->InstanceTemplate()->SetInternalFieldCount(1);
      window_constructor->SetClassName(name);

      emit_symbol = NODE_PSYMBOL("emit");
      inputchar_symbol = NODE_PSYMBOL("inputChar");
      echo_state_symbol = NODE_PSYMBOL("echo");
      showcursor_state_symbol = NODE_PSYMBOL("showCursor");
      lines_state_symbol = NODE_PSYMBOL("lines");
      cols_state_symbol = NODE_PSYMBOL("cols");
      tabsize_state_symbol = NODE_PSYMBOL("tabsize");
      hasmouse_state_symbol = NODE_PSYMBOL("hasMouse");
      hascolors_state_symbol = NODE_PSYMBOL("hasColors");
      numcolors_state_symbol = NODE_PSYMBOL("numColors");
      maxcolorpairs_state_symbol = NODE_PSYMBOL("maxColorPairs");
      raw_state_symbol = NODE_PSYMBOL("raw");
      bkgd_state_symbol = NODE_PSYMBOL("bkgd");
      hidden_state_symbol = NODE_PSYMBOL("hidden");
      height_state_symbol = NODE_PSYMBOL("height");
      width_state_symbol = NODE_PSYMBOL("width");
      begx_state_symbol = NODE_PSYMBOL("begx");
      begy_state_symbol = NODE_PSYMBOL("begy");
      curx_state_symbol = NODE_PSYMBOL("curx");
      cury_state_symbol = NODE_PSYMBOL("cury");
      maxx_state_symbol = NODE_PSYMBOL("maxx");
      maxy_state_symbol = NODE_PSYMBOL("maxy");
      wintouched_state_symbol = NODE_PSYMBOL("touched");
      numwins_symbol = NODE_PSYMBOL("numwins");
      ACS_symbol = NODE_PSYMBOL("ACS");
      keys_symbol = NODE_PSYMBOL("keys");
      colors_symbol = NODE_PSYMBOL("colors");
      attrs_symbol = NODE_PSYMBOL("attrs");

      /* Panel-specific methods */
      // TODO: color_set?, overlay, overwrite
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "clearok", Clearok);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "scrollok", Scrollok);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "idlok", Idlok);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "idcok", Idcok);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "leaveok", Leaveok);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "syncok", Syncok);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "immedok", Immedok);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "keypad", Keypad);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "meta", Meta);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "standout", Standout);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "hide", Hide);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "show", Show);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "top", Top);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "bottom", Bottom);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "move", Mvwin);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "refresh", Refresh);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "noutrefresh", Noutrefresh);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "frame", Frame);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "boldframe", Boldframe);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "label", Label);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "centertext", Centertext);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "cursor", Move);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "insertln", Insertln);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "insdelln", Insdelln);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "insstr", Insstr);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "attron", Attron);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "attroff", Attroff);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "attrset", Attrset);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "attrget", Attrget);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "box", Box);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "border", Border);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "hline", Hline);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "vline", Vline);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "erase", Erase);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "clear", Clear);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "clrtobot", Clrtobot);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "clrtoeol", Clrtoeol);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "delch", Delch);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "deleteln", Deleteln);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "scroll", Scroll);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "setscrreg", Setscrreg);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "touchlines", Touchln);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "is_linetouched", Is_linetouched);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "redrawln", Redrawln);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "touch", Touchwin);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "untouch", Untouchwin);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "resize", Wresize);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "print", Print);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "addstr", Addstr);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "close", Close);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "syncdown", Syncdown);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "syncup", Syncup);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "cursyncup", Cursyncup);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "copywin", Copywin);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "redraw", Redrawwin);

      /* Attribute-related window functions */
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "addch", Addch);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "echochar", Echochar);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "inch", Inch);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "insch", Insch);
      NODE_SET_PROTOTYPE_METHOD(window_constructor, "chgat", Chgat);
      //NODE_SET_PROTOTYPE_METHOD(window_constructor, "addchstr", Addchstr);
      //NODE_SET_PROTOTYPE_METHOD(window_constructor, "inchstr", Inchstr);

      /* Window properties */
      window_constructor->PrototypeTemplate()->SetAccessor(bkgd_state_symbol,
                                                           BkgdStateGetter,
                                                           BkgdStateSetter);
      window_constructor->PrototypeTemplate()->SetAccessor(hidden_state_symbol,
                                                           HiddenStateGetter);
      window_constructor->PrototypeTemplate()->SetAccessor(height_state_symbol,
                                                           HeightStateGetter);
      window_constructor->PrototypeTemplate()->SetAccessor(width_state_symbol,
                                                           WidthStateGetter);
      window_constructor->PrototypeTemplate()->SetAccessor(begx_state_symbol,
                                                           BegxStateGetter);
      window_constructor->PrototypeTemplate()->SetAccessor(begy_state_symbol,
                                                           BegyStateGetter);
      window_constructor->PrototypeTemplate()->SetAccessor(curx_state_symbol,
                                                           CurxStateGetter);
      window_constructor->PrototypeTemplate()->SetAccessor(cury_state_symbol,
                                                           CuryStateGetter);
      window_constructor->PrototypeTemplate()->SetAccessor(maxx_state_symbol,
                                                           MaxxStateGetter);
      window_constructor->PrototypeTemplate()->SetAccessor(maxy_state_symbol,
                                                           MaxyStateGetter);
      window_constructor->PrototypeTemplate()->SetAccessor(wintouched_state_symbol,
                                                           WintouchedStateGetter);

      /* Global/Terminal properties and functions */
      target->SetAccessor(echo_state_symbol, EchoStateGetter, EchoStateSetter);
      target->SetAccessor(showcursor_state_symbol, ShowcursorStateGetter,
                          ShowcursorStateSetter);
      target->SetAccessor(lines_state_symbol, LinesStateGetter);
      target->SetAccessor(cols_state_symbol, ColsStateGetter);
      target->SetAccessor(tabsize_state_symbol, TabsizeStateGetter);
      target->SetAccessor(hasmouse_state_symbol, HasmouseStateGetter);
      target->SetAccessor(hascolors_state_symbol, HascolorsStateGetter);
      target->SetAccessor(numcolors_state_symbol, NumcolorsStateGetter);
      target->SetAccessor(maxcolorpairs_state_symbol, MaxcolorpairsStateGetter);
      target->SetAccessor(raw_state_symbol, RawStateGetter, RawStateSetter);
      target->SetAccessor(numwins_symbol, NumwinsGetter);
      target->SetAccessor(ACS_symbol, ACSConstsGetter);
      target->SetAccessor(keys_symbol, KeyConstsGetter);
      target->SetAccessor(colors_symbol, ColorConstsGetter);
      target->SetAccessor(attrs_symbol, AttrConstsGetter);
      NODE_SET_METHOD(target, "setEscDelay", Setescdelay);
      NODE_SET_METHOD(target, "cleanup", Resetscreen);
      NODE_SET_METHOD(target, "redraw", Redraw);
      NODE_SET_METHOD(target, "leave", LeaveNcurses);
      NODE_SET_METHOD(target, "restore", RestoreNcurses);
      NODE_SET_METHOD(target, "beep", Beep);
      NODE_SET_METHOD(target, "flash", Flash);
      NODE_SET_METHOD(target, "doupdate", DoUpdate);
      NODE_SET_METHOD(target, "colorPair", Colorpair);
      NODE_SET_METHOD(target, "colorFg", Colorfg);
      NODE_SET_METHOD(target, "colorBg", Colorbg);
      NODE_SET_METHOD(target, "dup2", Dup2);
      NODE_SET_METHOD(target, "dup", Dup);

      target->Set(name, window_constructor->GetFunction());
    }

    void init(int nlines=-1, int ncols=-1, int begin_y=-1, int begin_x=-1) {
      static bool firstRun = true;
      if (stdin_fd < 0) {
        stdin_fd = STDIN_FILENO;
        int stdin_flags = fcntl(stdin_fd, F_GETFL, 0);
        int r = fcntl(stdin_fd, F_SETFL, stdin_flags | O_NONBLOCK);
        if (r < 0) {
          ThrowException(Exception::Error(
            String::New("Unable to set stdin to non-block")
          ));
          return;
        }
      }

      if (firstRun) {
        // Setup input listener
        read_watcher_ = new uv_poll_t;
        read_watcher_->data = this;
        uv_poll_init(uv_default_loop(), read_watcher_, stdin_fd);
        uv_poll_start(read_watcher_, UV_READABLE, io_event);
      }

      if (nlines < 0 || ncols < 0 || begin_y < 0 || begin_x < 0)
        panel_ = new MyPanel(this);
      else
        panel_ = new MyPanel(this, nlines, ncols, begin_y, begin_x);

      if (firstRun) {
        firstRun = false;

        ACS_Chars = Persistent<Object>::New(Object::New());
        ACS_Chars->Set(String::New("ULCORNER"), Uint32::NewFromUnsigned(ACS_ULCORNER));
        ACS_Chars->Set(String::New("LLCORNER"), Uint32::NewFromUnsigned(ACS_LLCORNER));
        ACS_Chars->Set(String::New("URCORNER"), Uint32::NewFromUnsigned(ACS_URCORNER));
        ACS_Chars->Set(String::New("LRCORNER"), Uint32::NewFromUnsigned(ACS_LRCORNER));
        ACS_Chars->Set(String::New("LTEE"), Uint32::NewFromUnsigned(ACS_LTEE));
        ACS_Chars->Set(String::New("RTEE"), Uint32::NewFromUnsigned(ACS_RTEE));
        ACS_Chars->Set(String::New("BTEE"), Uint32::NewFromUnsigned(ACS_BTEE));
        ACS_Chars->Set(String::New("TTEE"), Uint32::NewFromUnsigned(ACS_TTEE));
        ACS_Chars->Set(String::New("HLINE"), Uint32::NewFromUnsigned(ACS_HLINE));
        ACS_Chars->Set(String::New("VLINE"), Uint32::NewFromUnsigned(ACS_VLINE));
        ACS_Chars->Set(String::New("PLUS"), Uint32::NewFromUnsigned(ACS_PLUS));
        ACS_Chars->Set(String::New("S1"), Uint32::NewFromUnsigned(ACS_S1));
        ACS_Chars->Set(String::New("S9"), Uint32::NewFromUnsigned(ACS_S9));
        ACS_Chars->Set(String::New("DIAMOND"), Uint32::NewFromUnsigned(ACS_DIAMOND));
        ACS_Chars->Set(String::New("CKBOARD"), Uint32::NewFromUnsigned(ACS_CKBOARD));
        ACS_Chars->Set(String::New("DEGREE"), Uint32::NewFromUnsigned(ACS_DEGREE));
        ACS_Chars->Set(String::New("PLMINUS"), Uint32::NewFromUnsigned(ACS_PLMINUS));
        ACS_Chars->Set(String::New("BULLET"), Uint32::NewFromUnsigned(ACS_BULLET));
        ACS_Chars->Set(String::New("LARROW"), Uint32::NewFromUnsigned(ACS_LARROW));
        ACS_Chars->Set(String::New("RARROW"), Uint32::NewFromUnsigned(ACS_RARROW));
        ACS_Chars->Set(String::New("DARROW"), Uint32::NewFromUnsigned(ACS_DARROW));
        ACS_Chars->Set(String::New("UARROW"), Uint32::NewFromUnsigned(ACS_UARROW));
        ACS_Chars->Set(String::New("BOARD"), Uint32::NewFromUnsigned(ACS_BOARD));
        ACS_Chars->Set(String::New("LANTERN"), Uint32::NewFromUnsigned(ACS_LANTERN));
        ACS_Chars->Set(String::New("BLOCK"), Uint32::NewFromUnsigned(ACS_BLOCK));

        Keys = Persistent<Object>::New(Object::New());
        Keys->Set(String::New("SPACE"), Uint32::NewFromUnsigned(32));
        Keys->Set(String::New("NEWLINE"), Uint32::NewFromUnsigned(10));
        Keys->Set(String::New("ESC"), Uint32::NewFromUnsigned(27));
        Keys->Set(String::New("UP"), Uint32::NewFromUnsigned(KEY_UP));
        Keys->Set(String::New("DOWN"), Uint32::NewFromUnsigned(KEY_DOWN));
        Keys->Set(String::New("LEFT"), Uint32::NewFromUnsigned(KEY_LEFT));
        Keys->Set(String::New("RIGHT"), Uint32::NewFromUnsigned(KEY_RIGHT));
        Keys->Set(String::New("HOME"), Uint32::NewFromUnsigned(KEY_HOME));
        Keys->Set(String::New("BACKSPACE"), Uint32::NewFromUnsigned(KEY_BACKSPACE));
        Keys->Set(String::New("BREAK"), Uint32::NewFromUnsigned(KEY_BREAK));
        Keys->Set(String::New("F0"), Uint32::NewFromUnsigned(KEY_F(0)));
        Keys->Set(String::New("F1"), Uint32::NewFromUnsigned(KEY_F(1)));
        Keys->Set(String::New("F2"), Uint32::NewFromUnsigned(KEY_F(2)));
        Keys->Set(String::New("F3"), Uint32::NewFromUnsigned(KEY_F(3)));
        Keys->Set(String::New("F4"), Uint32::NewFromUnsigned(KEY_F(4)));
        Keys->Set(String::New("F5"), Uint32::NewFromUnsigned(KEY_F(5)));
        Keys->Set(String::New("F6"), Uint32::NewFromUnsigned(KEY_F(6)));
        Keys->Set(String::New("F7"), Uint32::NewFromUnsigned(KEY_F(7)));
        Keys->Set(String::New("F8"), Uint32::NewFromUnsigned(KEY_F(8)));
        Keys->Set(String::New("F9"), Uint32::NewFromUnsigned(KEY_F(9)));
        Keys->Set(String::New("F10"), Uint32::NewFromUnsigned(KEY_F(10)));
        Keys->Set(String::New("F11"), Uint32::NewFromUnsigned(KEY_F(11)));
        Keys->Set(String::New("F12"), Uint32::NewFromUnsigned(KEY_F(12)));
        Keys->Set(String::New("DL"), Uint32::NewFromUnsigned(KEY_DL));
        Keys->Set(String::New("IL"), Uint32::NewFromUnsigned(KEY_IL));
        Keys->Set(String::New("DEL"), Uint32::NewFromUnsigned(KEY_DC));
        Keys->Set(String::New("INS"), Uint32::NewFromUnsigned(KEY_IC));
        Keys->Set(String::New("EIC"), Uint32::NewFromUnsigned(KEY_EIC));
        Keys->Set(String::New("CLEAR"), Uint32::NewFromUnsigned(KEY_CLEAR));
        Keys->Set(String::New("EOS"), Uint32::NewFromUnsigned(KEY_EOS));
        Keys->Set(String::New("EOL"), Uint32::NewFromUnsigned(KEY_EOL));
        Keys->Set(String::New("SF"), Uint32::NewFromUnsigned(KEY_SF));
        Keys->Set(String::New("SR"), Uint32::NewFromUnsigned(KEY_SR));
        Keys->Set(String::New("NPAGE"), Uint32::NewFromUnsigned(KEY_NPAGE));
        Keys->Set(String::New("PPAGE"), Uint32::NewFromUnsigned(KEY_PPAGE));
        Keys->Set(String::New("STAB"), Uint32::NewFromUnsigned(KEY_STAB));
        Keys->Set(String::New("CTAB"), Uint32::NewFromUnsigned(KEY_CTAB));
        Keys->Set(String::New("CATAB"), Uint32::NewFromUnsigned(KEY_CATAB));
        Keys->Set(String::New("ENTER"), Uint32::NewFromUnsigned(KEY_ENTER));
        Keys->Set(String::New("SRESET"), Uint32::NewFromUnsigned(KEY_SRESET));
        Keys->Set(String::New("RESET"), Uint32::NewFromUnsigned(KEY_RESET));
        Keys->Set(String::New("PRINT"), Uint32::NewFromUnsigned(KEY_PRINT));
        Keys->Set(String::New("LL"), Uint32::NewFromUnsigned(KEY_LL));
        Keys->Set(String::New("UPLEFT"), Uint32::NewFromUnsigned(KEY_A1));
        Keys->Set(String::New("UPRIGHT"), Uint32::NewFromUnsigned(KEY_A3));
        Keys->Set(String::New("CENTER"), Uint32::NewFromUnsigned(KEY_B2));
        Keys->Set(String::New("DOWNLEFT"), Uint32::NewFromUnsigned(KEY_C1));
        Keys->Set(String::New("DOWNRIGHT"), Uint32::NewFromUnsigned(KEY_C3));
        Keys->Set(String::New("BTAB"), Uint32::NewFromUnsigned(KEY_BTAB));
        Keys->Set(String::New("BEG"), Uint32::NewFromUnsigned(KEY_BEG));
        Keys->Set(String::New("CANCEL"), Uint32::NewFromUnsigned(KEY_CANCEL));
        Keys->Set(String::New("CLOSE"), Uint32::NewFromUnsigned(KEY_CLOSE));
        Keys->Set(String::New("COMMAND"), Uint32::NewFromUnsigned(KEY_COMMAND));
        Keys->Set(String::New("COPY"), Uint32::NewFromUnsigned(KEY_COPY));
        Keys->Set(String::New("CREATE"), Uint32::NewFromUnsigned(KEY_CREATE));
        Keys->Set(String::New("END"), Uint32::NewFromUnsigned(KEY_END));
        Keys->Set(String::New("EXIT"), Uint32::NewFromUnsigned(KEY_EXIT));
        Keys->Set(String::New("FIND"), Uint32::NewFromUnsigned(KEY_FIND));
        Keys->Set(String::New("FIND"), Uint32::NewFromUnsigned(KEY_HELP));
        Keys->Set(String::New("MARK"), Uint32::NewFromUnsigned(KEY_MARK));
        Keys->Set(String::New("MESSAGE"), Uint32::NewFromUnsigned(KEY_MESSAGE));
        Keys->Set(String::New("MOVE"), Uint32::NewFromUnsigned(KEY_MOVE));
        Keys->Set(String::New("NEXT"), Uint32::NewFromUnsigned(KEY_NEXT));
        Keys->Set(String::New("OPEN"), Uint32::NewFromUnsigned(KEY_OPEN));
        Keys->Set(String::New("OPTIONS"), Uint32::NewFromUnsigned(KEY_OPTIONS));
        Keys->Set(String::New("PREVIOUS"), Uint32::NewFromUnsigned(KEY_PREVIOUS));
        Keys->Set(String::New("REDO"), Uint32::NewFromUnsigned(KEY_REDO));
        Keys->Set(String::New("REFERENCE"), Uint32::NewFromUnsigned(KEY_REFERENCE));
        Keys->Set(String::New("REFRESH"), Uint32::NewFromUnsigned(KEY_REFRESH));
        Keys->Set(String::New("REPLACE"), Uint32::NewFromUnsigned(KEY_REPLACE));
        Keys->Set(String::New("RESTART"), Uint32::NewFromUnsigned(KEY_RESTART));
        Keys->Set(String::New("RESUME"), Uint32::NewFromUnsigned(KEY_RESUME));
        Keys->Set(String::New("SAVE"), Uint32::NewFromUnsigned(KEY_SAVE));
        Keys->Set(String::New("S_BEG"), Uint32::NewFromUnsigned(KEY_SBEG));
        Keys->Set(String::New("S_CANCEL"), Uint32::NewFromUnsigned(KEY_SCANCEL));
        Keys->Set(String::New("S_COMMAND"), Uint32::NewFromUnsigned(KEY_SCOMMAND));
        Keys->Set(String::New("S_COPY"), Uint32::NewFromUnsigned(KEY_SCOPY));
        Keys->Set(String::New("S_CREATE"), Uint32::NewFromUnsigned(KEY_SCREATE));
        Keys->Set(String::New("S_DC"), Uint32::NewFromUnsigned(KEY_SDC));
        Keys->Set(String::New("S_DL"), Uint32::NewFromUnsigned(KEY_SDL));
        Keys->Set(String::New("SELECT"), Uint32::NewFromUnsigned(KEY_SELECT));
        Keys->Set(String::New("SEND"), Uint32::NewFromUnsigned(KEY_SEND));
        Keys->Set(String::New("S_EOL"), Uint32::NewFromUnsigned(KEY_SEOL));
        Keys->Set(String::New("S_EXIT"), Uint32::NewFromUnsigned(KEY_SEXIT));
        Keys->Set(String::New("S_FIND"), Uint32::NewFromUnsigned(KEY_SFIND));
        Keys->Set(String::New("S_HELP"), Uint32::NewFromUnsigned(KEY_SHELP));
        Keys->Set(String::New("S_HOME"), Uint32::NewFromUnsigned(KEY_SHOME));
        Keys->Set(String::New("S_IC"), Uint32::NewFromUnsigned(KEY_SIC));
        Keys->Set(String::New("S_LEFT"), Uint32::NewFromUnsigned(KEY_SLEFT));
        Keys->Set(String::New("S_MESSAGE"), Uint32::NewFromUnsigned(KEY_SMESSAGE));
        Keys->Set(String::New("S_MOVE"), Uint32::NewFromUnsigned(KEY_SMOVE));
        Keys->Set(String::New("S_NEXT"), Uint32::NewFromUnsigned(KEY_SNEXT));
        Keys->Set(String::New("S_OPTIONS"), Uint32::NewFromUnsigned(KEY_SOPTIONS));
        Keys->Set(String::New("S_PREVIOUS"), Uint32::NewFromUnsigned(KEY_SPREVIOUS));
        Keys->Set(String::New("S_PRINT"), Uint32::NewFromUnsigned(KEY_SPRINT));
        Keys->Set(String::New("S_REDO"), Uint32::NewFromUnsigned(KEY_SREDO));
        Keys->Set(String::New("S_REPLACE"), Uint32::NewFromUnsigned(KEY_SREPLACE));
        Keys->Set(String::New("S_RIGHT"), Uint32::NewFromUnsigned(KEY_SRIGHT));
        Keys->Set(String::New("S_RESUME"), Uint32::NewFromUnsigned(KEY_SRSUME));
        Keys->Set(String::New("S_SAVE"), Uint32::NewFromUnsigned(KEY_SSAVE));
        Keys->Set(String::New("S_SUSPEND"), Uint32::NewFromUnsigned(KEY_SSUSPEND));
        Keys->Set(String::New("S_UNDO"), Uint32::NewFromUnsigned(KEY_SUNDO));
        Keys->Set(String::New("SUSPEND"), Uint32::NewFromUnsigned(KEY_SUSPEND));
        Keys->Set(String::New("UNDO"), Uint32::NewFromUnsigned(KEY_UNDO));
        Keys->Set(String::New("MOUSE"), Uint32::NewFromUnsigned(KEY_MOUSE));
        Keys->Set(String::New("RESIZE"), Uint32::NewFromUnsigned(KEY_RESIZE));

        Colors = Persistent<Object>::New(Object::New());
        Colors->Set(String::New("BLACK"), Uint32::NewFromUnsigned(COLOR_BLACK));
        Colors->Set(String::New("RED"), Uint32::NewFromUnsigned(COLOR_RED));
        Colors->Set(String::New("GREEN"), Uint32::NewFromUnsigned(COLOR_GREEN));
        Colors->Set(String::New("YELLOW"), Uint32::NewFromUnsigned(COLOR_YELLOW));
        Colors->Set(String::New("BLUE"), Uint32::NewFromUnsigned(COLOR_BLUE));
        Colors->Set(String::New("MAGENTA"), Uint32::NewFromUnsigned(COLOR_MAGENTA));
        Colors->Set(String::New("CYAN"), Uint32::NewFromUnsigned(COLOR_CYAN));
        Colors->Set(String::New("WHITE"), Uint32::NewFromUnsigned(COLOR_WHITE));

        Attrs = Persistent<Object>::New(Object::New());
        Attrs->Set(String::New("NORMAL"), Uint32::NewFromUnsigned(A_NORMAL));
        Attrs->Set(String::New("STANDOUT"), Uint32::NewFromUnsigned(A_STANDOUT));
        Attrs->Set(String::New("UNDERLINE"), Uint32::NewFromUnsigned(A_UNDERLINE));
        Attrs->Set(String::New("REVERSE"), Uint32::NewFromUnsigned(A_REVERSE));
        Attrs->Set(String::New("BLINK"), Uint32::NewFromUnsigned(A_BLINK));
        Attrs->Set(String::New("DIM"), Uint32::NewFromUnsigned(A_DIM));
        Attrs->Set(String::New("BOLD"), Uint32::NewFromUnsigned(A_BOLD));
        Attrs->Set(String::New("INVISIBLE"), Uint32::NewFromUnsigned(A_INVIS));
        Attrs->Set(String::New("PROTECT"), Uint32::NewFromUnsigned(A_PROTECT));
      }
    }

    MyPanel* panel() {
      return panel_;
    }

    static void on_handle_close (uv_handle_t *handle) {
      delete handle;
    }

    void close() {
      if (panel_) {
        bool wasStdscr = panel_->isStdscr();
        if (wasStdscr) {
          uv_poll_stop(read_watcher_);
          uv_close((uv_handle_t *)read_watcher_, on_handle_close);
          Unref();
        }
        delete panel_;
        panel_ = NULL;
        MyPanel::updateTopmost();
        if (wasStdscr)
          ::endwin();
      }
    }

  protected:
    static Handle<Value> New (const Arguments& args) {
      HandleScope scope;

      Window *win;

      if (stdin_fd < 0)
        win = new Window();
      else if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
        win = new Window(args[0]->Int32Value(), args[1]->Int32Value(), 0, 0);
      else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
               && args[2]->IsInt32()) {
        win = new Window(args[0]->Int32Value(), args[1]->Int32Value(),
                         args[2]->Int32Value(), 0);
      } else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32()
                 && args[2]->IsInt32() && args[3]->IsInt32()) {
        win = new Window(args[0]->Int32Value(), args[1]->Int32Value(),
                         args[2]->Int32Value(), args[3]->Int32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      win->Wrap(args.This());
      win->Ref();

      win->Emit = Persistent<Function>::New(
                    Local<Function>::Cast(win->handle_->Get(emit_symbol))
                  );

      return args.This();
    }

    static Handle<Value> Close (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      if (win->panel() != NULL)
        win->close();

      return Undefined();
    }

    static Handle<Value> Hide (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      win->panel()->hide();

      return Undefined();
    }

    static Handle<Value> Show (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      win->panel()->show();

      return Undefined();
    }

    static Handle<Value> Top (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      win->panel()->top();

      return Undefined();
    }

    static Handle<Value> Bottom (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      win->panel()->bottom();

      return Undefined();
    }

    static Handle<Value> Mvwin (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret = 0;
      if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
        ret = win->panel()->mvwin(args[0]->Int32Value(), args[1]->Int32Value());
      else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Refresh (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret = win->panel()->refresh();

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Noutrefresh (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret = win->panel()->noutrefresh();

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Redraw (const Arguments& args) {
      HandleScope scope;

      MyPanel::redraw();

      return Undefined();
    }

    static Handle<Value> Frame (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      if (args.Length() == 0) {
        win->panel()->frame(NULL, NULL);
      } else if (args.Length() == 1 && args[0]->IsString()) {
        String::Utf8Value str(args[0]->ToString());
        win->panel()->frame(ToCString(str), NULL);
      } else if (args.Length() == 2 && args[0]->IsString()
                 && args[1]->IsString()) {
        String::Utf8Value str0(args[0]->ToString());
        String::Utf8Value str1(args[1]->ToString());
        win->panel()->frame(ToCString(str0), ToCString(str1));
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return Undefined();
    }

    static Handle<Value> Boldframe (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      if (args.Length() == 0) {
        win->panel()->boldframe(NULL, NULL);
      } else if (args.Length() == 1 && args[0]->IsString()) {
        String::Utf8Value str(args[0]->ToString());
        win->panel()->boldframe(ToCString(str), NULL);
      } else if (args.Length() == 2 && args[0]->IsString()
                 && args[1]->IsString()) {
        String::Utf8Value str0(args[0]->ToString());
        String::Utf8Value str1(args[1]->ToString());
        win->panel()->boldframe(ToCString(str0), ToCString(str1));
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return Undefined();
    }

    static Handle<Value> Label (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      if (args.Length() == 1 && args[0]->IsString()) {
        String::Utf8Value str(args[0]->ToString());
        win->panel()->label(ToCString(str), NULL);
      } else if (args.Length() == 2 && args[0]->IsString()
                 && args[1]->IsString()) {
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = win->panel()->addch(args[0]->Uint32Value());
      else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
               && args[2]->IsUint32()) {
        ret = win->panel()->addch(args[0]->Int32Value(), args[1]->Int32Value(),
                                  args[2]->Uint32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Echochar (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 1 && args[0]->IsString()) {
        String::Utf8Value str(args[0]->ToString());
        ret = win->panel()->addstr(ToCString(str), -1);
      } else if (args.Length() == 2 && args[0]->IsString()
                 && args[1]->IsInt32()) {
        String::Utf8Value str(args[0]->ToString());
        ret = win->panel()->addstr(ToCString(str), args[1]->Int32Value());
      } else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
                 && args[2]->IsString()) {
        String::Utf8Value str(args[2]->ToString());
        ret = win->panel()->addstr(args[0]->Int32Value(), args[1]->Int32Value(),
                                   ToCString(str), -1);
      } else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32()
                 && args[2]->IsString() && args[3]->IsInt32()) {
        String::Utf8Value str(args[2]->ToString());
        ret = win->panel()->addstr(args[0]->Int32Value(), args[1]->Int32Value(),
                                   ToCString(str), args[3]->Int32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    // FIXME: addchstr requires a pointer to a chtype not an actual value,
    //        unlike the other ACS_*-using methods
    /*static Handle<Value> Addchstr (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = win->panel()->addchstr(args[0]->Uint32Value(), -1);
      else if (args.Length() == 2 && args[0]->IsUint32() && args[1]->IsInt32()) {
        ret = win->panel()->addchstr(args[0]->Uint32Value(),
                                     args[1]->Int32Value());
      } else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
                 && args[2]->IsUint32()) {
        ret = win->panel()->addchstr(args[0]->Int32Value(), args[1]->Int32Value(),
                                     args[2]->Uint32Value(), -1);
      } else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32()
                 && args[2]->IsUint32() && args[3]->IsInt32()) {
        ret = win->panel()->addchstr(args[0]->Int32Value(), args[1]->Int32Value(),
                                     args[2]->Uint32Value(),
                                     args[3]->Int32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }*/

    static Handle<Value> Inch (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = win->panel()->inchstr(args[0]->Uint32Value(), -1);
      else if (args.Length() == 2 && args[0]->IsUint32() && args[1]->IsInt32()) {
        ret = win->panel()->inchstr(args[0]->Uint32Value(),
                                    args[1]->Int32Value());
      } else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
                 && args[2]->IsUint32()) {
        ret = win->panel()->inchstr(args[0]->Int32Value(), args[1]->Int32Value(),
                                    args[2]->Uint32Value(), -1);
      } else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32()
                 && args[2]->IsUint32() && args[3]->IsInt32()) {
        ret = win->panel()->inchstr(args[0]->Int32Value(), args[1]->Int32Value(),
                                    args[2]->Uint32Value(),
                                    args[3]->Int32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }*/

    static Handle<Value> Insch (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = win->panel()->insch(args[0]->Uint32Value());
      else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
               && args[2]->IsUint32()) {
        ret = win->panel()->insch(args[0]->Int32Value(), args[1]->Int32Value(),
                                  args[2]->Uint32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Chgat (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsUint32()) {
        ret = win->panel()->chgat(args[0]->Int32Value(), args[1]->Uint32Value(),
                                  win->panel()->getcolor());
      } else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsUint32()
                 && args[2]->IsUint32()) {
        ret = win->panel()->chgat(args[0]->Int32Value(),
                                  (attr_t)(args[1]->Uint32Value()),
                                  args[2]->Uint32Value());
      } else if (args.Length() == 4 && args[0]->IsUint32() && args[1]->IsUint32()
                 && args[2]->IsInt32() && args[3]->IsUint32()) {
        ret = win->panel()->chgat(args[0]->Uint32Value(), args[1]->Uint32Value(),
                                  args[2]->Int32Value(),
                                  (attr_t)(args[3]->Uint32Value()),
                                  win->panel()->getcolor());
      } else if (args.Length() == 5 && args[0]->IsUint32() && args[1]->IsUint32()
                 && args[2]->IsInt32() && args[3]->IsUint32()
                 && args[4]->IsUint32()) {
        ret = win->panel()->chgat(args[0]->Uint32Value(), args[1]->Uint32Value(),
                                  args[2]->Int32Value(),
                                  (attr_t)(args[3]->Uint32Value()),
                                  args[4]->Uint32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Insertln (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret = win->panel()->insertln();

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Insdelln (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 1 && args[0]->IsString()) {
        String::Utf8Value str(args[0]->ToString());
        ret = win->panel()->insstr(ToCString(str), -1);
      } else if (args.Length() == 2 && args[0]->IsString()
                 && args[1]->IsInt32()) {
        String::Utf8Value str(args[0]->ToString());
        ret = win->panel()->insstr(ToCString(str), args[1]->Int32Value());
      } else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
                 && args[2]->IsString()) {
        String::Utf8Value str(args[2]->ToString());
        ret = win->panel()->insstr(args[0]->Int32Value(), args[1]->Int32Value(),
                                   ToCString(str), -1);
      } else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32()
                 && args[2]->IsString() && args[3]->IsInt32()) {
        String::Utf8Value str(args[2]->ToString());
        ret = win->panel()->insstr(args[0]->Int32Value(), args[1]->Int32Value(),
                                   ToCString(str), args[3]->Int32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Attron (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      unsigned int ret = win->panel()->attrget();

      return scope.Close(Integer::NewFromUnsigned(ret));
    }

    static Handle<Value> Box (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 0)
        ret = win->panel()->border(0);
      else if (args.Length() == 1 && args[0]->IsUint32())
        ret = win->panel()->border(args[0]->Uint32Value(), 0);
      else if (args.Length() == 2 && args[0]->IsUint32()
               && args[1]->IsUint32()) {
        ret = win->panel()->border(args[0]->Uint32Value(),
                                   args[1]->Uint32Value(), 0);
      } else if (args.Length() == 3 && args[0]->IsUint32() && args[1]->IsUint32()
                 && args[2]->IsUint32()) {
        ret = win->panel()->border(args[0]->Uint32Value(), args[1]->Uint32Value(),
                                   args[2]->Uint32Value(), 0);
      } else if (args.Length() == 4 && args[0]->IsUint32() && args[1]->IsUint32()
                 && args[2]->IsUint32() && args[3]->IsUint32()) {
        ret = win->panel()->border(args[0]->Uint32Value(), args[1]->Uint32Value(),
                                   args[2]->Uint32Value(),
                                   args[3]->Uint32Value(), 0);
      } else if (args.Length() == 5 && args[0]->IsUint32() && args[1]->IsUint32()
                 && args[2]->IsUint32() && args[3]->IsUint32()
                 && args[4]->IsUint32()) {
        ret = win->panel()->border(args[0]->Uint32Value(), args[1]->Uint32Value(),
                                   args[2]->Uint32Value(), args[3]->Uint32Value(),
                                   args[4]->Uint32Value(), 0);
      } else if (args.Length() == 6 && args[0]->IsUint32() && args[1]->IsUint32()
                 && args[2]->IsUint32() && args[3]->IsUint32()
                 && args[4]->IsUint32() && args[5]->IsUint32()) {
        ret = win->panel()->border(args[0]->Uint32Value(), args[1]->Uint32Value(),
                                   args[2]->Uint32Value(), args[3]->Uint32Value(),
                                   args[4]->Uint32Value(),
                                   args[5]->Uint32Value(), 0);
      } else if (args.Length() == 7 && args[0]->IsUint32() && args[1]->IsUint32()
                 && args[2]->IsUint32() && args[3]->IsUint32()
                 && args[4]->IsUint32() && args[5]->IsUint32()
                 && args[6]->IsUint32()) {
        ret = win->panel()->border(args[0]->Uint32Value(), args[1]->Uint32Value(),
                                   args[2]->Uint32Value(), args[3]->Uint32Value(),
                                   args[4]->Uint32Value(), args[5]->Uint32Value(),
                                   args[6]->Uint32Value(), 0);
      } else if (args.Length() == 8 && args[0]->IsUint32() && args[1]->IsUint32()
                 && args[2]->IsUint32() && args[3]->IsUint32()
                 && args[4]->IsUint32() && args[5]->IsUint32()
                 && args[6]->IsUint32() && args[7]->IsUint32()) {
        ret = win->panel()->border(args[0]->Uint32Value(), args[1]->Uint32Value(),
                                   args[2]->Uint32Value(), args[3]->Uint32Value(),
                                   args[4]->Uint32Value(), args[5]->Uint32Value(),
                                   args[6]->Uint32Value(), args[7]->Uint32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Hline (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 1 && args[0]->IsInt32())
        ret = win->panel()->hline(args[0]->Int32Value(), 0);
      else if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsUint32())
        ret = win->panel()->hline(args[0]->Int32Value(), args[1]->Uint32Value());
      else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
               && args[2]->IsInt32()) {
        ret = win->panel()->hline(args[0]->Int32Value(), args[1]->Int32Value(),
                                  args[2]->Int32Value(), 0);
      } else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32()
                 && args[2]->IsInt32() && args[3]->IsUint32()) {
        ret = win->panel()->hline(args[0]->Int32Value(), args[1]->Int32Value(),
                                  args[2]->Int32Value(), args[3]->Uint32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Vline (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 1 && args[0]->IsInt32())
        ret = win->panel()->vline(args[0]->Int32Value(), 0);
      else if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsUint32())
        ret = win->panel()->vline(args[0]->Int32Value(), args[1]->Uint32Value());
      else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
               && args[2]->IsInt32()) {
        ret = win->panel()->vline(args[0]->Int32Value(), args[1]->Int32Value(),
                                  args[2]->Int32Value(), 0);
      } else if (args.Length() == 4 && args[0]->IsInt32() && args[1]->IsInt32()
                 && args[2]->IsInt32() && args[3]->IsUint32()) {
        ret = win->panel()->vline(args[0]->Int32Value(), args[1]->Int32Value(),
                                  args[2]->Int32Value(), args[3]->Uint32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Erase (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret = win->panel()->erase();

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Clear (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret = win->panel()->clear();

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Clrtobot (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret = win->panel()->clrtobot();

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Clrtoeol (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret = win->panel()->clrtoeol();

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Delch (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret = win->panel()->deleteln();

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Scroll (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32()) {
        ret = win->panel()->setscrreg(args[0]->Int32Value(),
                                      args[1]->Int32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    /*static Handle<Value> Touchline (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32()) {
        ret = win->panel()->touchline(args[0]->Int32Value(),
                                      args[1]->Int32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }*/

    static Handle<Value> Touchwin (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret = win->panel()->touchwin();

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Untouchwin (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret = win->panel()->untouchwin();

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Touchln (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32()) {
        ret = win->panel()->touchln(args[0]->Int32Value(),
                                    args[1]->Int32Value(), true);
      } else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
                 && args[2]->IsBoolean()) {
        ret = win->panel()->touchln(args[0]->Int32Value(), args[1]->Int32Value(),
                                    args[2]->BooleanValue());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Is_linetouched (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32()) {
        ret = win->panel()->redrawln(args[0]->Int32Value(),
                                     args[1]->Int32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Syncdown (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      win->panel()->syncdown();

      return Undefined();
    }

    static Handle<Value> Syncup (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      win->panel()->syncup();

      return Undefined();
    }

    static Handle<Value> Cursyncup (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      win->panel()->cursyncup();

      return Undefined();
    }

    static Handle<Value> Wresize (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32()) {
        ret = win->panel()->wresize(args[0]->Int32Value(),
                                    args[1]->Int32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Print (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret;
      if (args.Length() == 1 && args[0]->IsString()) {
        String::Utf8Value str(args[0]->ToString());
        ret = win->panel()->printw("%s", ToCString(str));
      } else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
                 && args[2]->IsString()) {
        String::Utf8Value str(args[2]->ToString());
        ret = win->panel()->printw(args[0]->Int32Value(), args[1]->Int32Value(),
                                   "%s", ToCString(str));
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Clearok (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
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

    static Handle<Value> Resetscreen (const Arguments& args) {
      HandleScope scope;

      ::endwin(); //MyPanel::resetScreen();

      return Undefined();
    }

    static Handle<Value> Setescdelay (const Arguments& args) {
      HandleScope scope;

      if (args.Length() == 0 || !args[0]->IsInt32()) {
        return ThrowException(Exception::Error(
          String::New("Invalid argument")
        ));
      }

      ::set_escdelay(args[0]->Int32Value());

      return Undefined();
    }

    static Handle<Value> LeaveNcurses (const Arguments& args) {
      HandleScope scope;

      ::def_prog_mode();
      ::endwin();

      return Undefined();
    }

    static Handle<Value> RestoreNcurses (const Arguments& args) {
      HandleScope scope;

      ::reset_prog_mode();
      MyPanel::redraw();

      return Undefined();
    }

    static Handle<Value> Beep (const Arguments& args) {
      HandleScope scope;

      ::beep();

      return Undefined();
    }

    static Handle<Value> Flash (const Arguments& args) {
      HandleScope scope;

      ::flash();

      return Undefined();
    }

    static Handle<Value> DoUpdate (const Arguments& args) {
      HandleScope scope;

      MyPanel::doUpdate();

      return Undefined();
    }

    static Handle<Value> Colorpair (const Arguments& args) {
      HandleScope scope;

      int ret;
      if (args.Length() == 1 && args[0]->IsInt32())
        ret = MyPanel::pair(args[0]->Int32Value());
      else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
               && args[2]->IsInt32()) {
        ret = MyPanel::pair(args[0]->Int32Value(), (short)args[1]->Int32Value(),
                            (short)args[2]->Int32Value());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Colorfg (const Arguments& args) {
      HandleScope scope;

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = MyPanel::getFgcolor(args[0]->Uint32Value());
      else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Colorbg (const Arguments& args) {
      HandleScope scope;

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = MyPanel::getBgcolor(args[0]->Uint32Value());
      else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Dup2 (const Arguments& args) {
      HandleScope scope;

      int ret;
      if (args.Length() == 2 && args[0]->IsUint32() && args[1]->IsUint32())
        ret = dup2(args[0]->Uint32Value(), args[1]->Uint32Value());
      else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Dup (const Arguments& args) {
      HandleScope scope;

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = dup(args[0]->Uint32Value());
      else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Copywin (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;
      int ret;
      if (args.Length() == 7 && args[1]->IsUint32() && args[2]->IsUint32()
          && args[3]->IsUint32() && args[4]->IsUint32() && args[5]->IsUint32()
          && args[6]->IsUint32()) {
        ret = win->panel()->copywin(
                    *(ObjectWrap::Unwrap<Window>(args[0]->ToObject())->panel()),
                    args[1]->Uint32Value(), args[2]->Uint32Value(),
                    args[3]->Uint32Value(), args[4]->Uint32Value(),
                    args[5]->Uint32Value(), args[6]->Uint32Value());
      } else if (args.Length() == 8 && args[1]->IsUint32()
                 && args[2]->IsUint32() && args[3]->IsUint32()
                 && args[4]->IsUint32() && args[5]->IsUint32()
                 && args[6]->IsUint32() && args[7]->IsBoolean()) {
        ret = win->panel()->copywin(
                    *(ObjectWrap::Unwrap<Window>(args[0]->ToObject())->panel()),
                    args[1]->Uint32Value(), args[2]->Uint32Value(),
                    args[3]->Uint32Value(), args[4]->Uint32Value(),
                    args[5]->Uint32Value(), args[6]->Uint32Value(),
                    args[7]->BooleanValue());
      } else {
        return ThrowException(Exception::Error(
          String::New("Invalid number and/or types of arguments")
        ));
      }

      return scope.Close(Integer::New(ret));
    }

    static Handle<Value> Redrawwin (const Arguments& args) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      HandleScope scope;

      int ret = win->panel()->redrawwin();

      return scope.Close(Integer::New(ret));
    }

    // -- Getters/Setters ------------------------------------------------------
    static Handle<Value> EchoStateGetter (Local<String> property,
                                          const AccessorInfo& info) {
      assert(property == echo_state_symbol);

      HandleScope scope;

      return scope.Close(Boolean::New(MyPanel::echo()));
    }

    static void EchoStateSetter (Local<String> property, Local<Value> value,
                                 const AccessorInfo& info) {
      assert(property == echo_state_symbol);

      if (!value->IsBoolean()) {
        ThrowException(Exception::TypeError(
          String::New("echo should be of Boolean value")
        ));
      }

      MyPanel::echo(value->BooleanValue());
    }

    static Handle<Value> ShowcursorStateGetter (Local<String> property,
                                                const AccessorInfo& info) {
      assert(property == showcursor_state_symbol);

      HandleScope scope;

      return scope.Close(Boolean::New(MyPanel::showCursor()));
    }

    static void ShowcursorStateSetter (Local<String> property, Local<Value> value,
                                       const AccessorInfo& info) {
      assert(property == showcursor_state_symbol);

      if (!value->IsBoolean()) {
        ThrowException(Exception::TypeError(
          String::New("showCursor should be of Boolean value")
        ));
      }
      MyPanel::showCursor(value->BooleanValue());
    }

    static Handle<Value> LinesStateGetter (Local<String> property,
                                           const AccessorInfo& info) {
      assert(property == lines_state_symbol);

      HandleScope scope;

      return scope.Close(Integer::New(LINES));
    }

    static Handle<Value> ColsStateGetter (Local<String> property,
                                          const AccessorInfo& info) {
      assert(property == cols_state_symbol);

      HandleScope scope;

      return scope.Close(Integer::New(COLS));
    }

    static Handle<Value> TabsizeStateGetter (Local<String> property,
                                             const AccessorInfo& info) {
      assert(property == tabsize_state_symbol);

      HandleScope scope;

      return scope.Close(Integer::New(TABSIZE));
    }

    static Handle<Value> HasmouseStateGetter (Local<String> property,
                                              const AccessorInfo& info) {
      assert(property == hasmouse_state_symbol);

      HandleScope scope;

      return scope.Close(Boolean::New(MyPanel::hasMouse()));
    }

    static Handle<Value> HiddenStateGetter (Local<String> property,
                                            const AccessorInfo& info) {
      Window *win = ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == hidden_state_symbol);

      HandleScope scope;

      return scope.Close(Boolean::New(win->panel()->hidden()));
    }

    static Handle<Value> HeightStateGetter (Local<String> property,
                                            const AccessorInfo& info) {
      Window *win = ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == height_state_symbol);

      HandleScope scope;

      return scope.Close(Integer::New(win->panel()->height()));
    }

    static Handle<Value> WidthStateGetter (Local<String> property,
                                           const AccessorInfo& info) {
      Window *win = ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == width_state_symbol);

      HandleScope scope;

      return scope.Close(Integer::New(win->panel()->width()));
    }

    static Handle<Value> BegxStateGetter (Local<String> property,
                                          const AccessorInfo& info) {
      Window *win = ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == begx_state_symbol);

      HandleScope scope;

      return scope.Close(Integer::New(win->panel()->begx()));
    }

    static Handle<Value> BegyStateGetter (Local<String> property,
                                          const AccessorInfo& info) {
      Window *win = ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == begy_state_symbol);

      HandleScope scope;

      return scope.Close(Integer::New(win->panel()->begy()));
    }

    static Handle<Value> CurxStateGetter (Local<String> property,
                                          const AccessorInfo& info) {
      Window *win = ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == curx_state_symbol);

      HandleScope scope;

      return scope.Close(Integer::New(win->panel()->curx()));
    }

    static Handle<Value> CuryStateGetter (Local<String> property,
                                          const AccessorInfo& info) {
      Window *win = ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == cury_state_symbol);

      HandleScope scope;

      return scope.Close(Integer::New(win->panel()->cury()));
    }

    static Handle<Value> MaxxStateGetter (Local<String> property,
                                          const AccessorInfo& info) {
      Window *win = ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == maxx_state_symbol);

      HandleScope scope;

      return scope.Close(Integer::New(win->panel()->maxx()));
    }

    static Handle<Value> MaxyStateGetter (Local<String> property,
                                          const AccessorInfo& info) {
      Window *win = ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == maxy_state_symbol);

      HandleScope scope;

      return scope.Close(Integer::New(win->panel()->maxy()));
    }

    static Handle<Value> BkgdStateGetter (Local<String> property,
                                          const AccessorInfo& info) {
      Window *win = ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == bkgd_state_symbol);

      HandleScope scope;

      return scope.Close(Integer::NewFromUnsigned(win->panel()->getbkgd()));
    }

    static void BkgdStateSetter (Local<String> property, Local<Value> value,
                                 const AccessorInfo& info) {
      Window *win = ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == bkgd_state_symbol);
      unsigned int val = 32;

      if (!value->IsUint32() && !value->IsString()) {
        ThrowException(Exception::TypeError(
          String::New("bkgd should be of unsigned integer or a string value")
        ));
      }
      if (value->IsString()) {
        String::AsciiValue str(value->ToString());
        if (str.length() > 0)
          val = (unsigned int)((*str)[0]);
      } else
        val = value->Uint32Value();

      win->panel()->bkgd(val);
    }

    static Handle<Value> HascolorsStateGetter (Local<String> property,
                                               const AccessorInfo& info) {
      assert(property == hascolors_state_symbol);

      HandleScope scope;

      return scope.Close(Boolean::New(MyPanel::has_colors()));
    }

    static Handle<Value> NumcolorsStateGetter (Local<String> property,
                                               const AccessorInfo& info) {
      assert(property == numcolors_state_symbol);

      HandleScope scope;

      return scope.Close(Integer::New(MyPanel::num_colors()));
    }

    static Handle<Value> MaxcolorpairsStateGetter (Local<String> property,
                                                   const AccessorInfo& info) {
      assert(property == maxcolorpairs_state_symbol);

      HandleScope scope;

      return scope.Close(Integer::New(MyPanel::max_pairs()));
    }

    static Handle<Value> WintouchedStateGetter (Local<String> property,
                                                const AccessorInfo& info) {
      Window *win = ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == wintouched_state_symbol);

      HandleScope scope;

      return scope.Close(Boolean::New(win->panel()->is_wintouched()));
    }

    static Handle<Value> RawStateGetter (Local<String> property,
                                         const AccessorInfo& info) {
      assert(property == raw_state_symbol);

      HandleScope scope;

      return scope.Close(Boolean::New(MyPanel::raw()));
    }

    static void RawStateSetter (Local<String> property, Local<Value> value,
                                const AccessorInfo& info) {
      assert(property == raw_state_symbol);

      if (!value->IsBoolean()) {
        ThrowException(Exception::TypeError(
          String::New("raw should be of Boolean value")
        ));
      }

      MyPanel::raw(value->BooleanValue());
    }

    static Handle<Value> ACSConstsGetter (Local<String> property,
                                          const AccessorInfo& info) {
      HandleScope scope;

      return scope.Close(ACS_Chars);
    }

    static Handle<Value> KeyConstsGetter (Local<String> property,
                                          const AccessorInfo& info) {
      HandleScope scope;

      return scope.Close(Keys);
    }

    static Handle<Value> ColorConstsGetter (Local<String> property,
                                            const AccessorInfo& info) {
      HandleScope scope;

      return scope.Close(Colors);
    }

    static Handle<Value> AttrConstsGetter (Local<String> property,
                                           const AccessorInfo& info) {
      HandleScope scope;

      return scope.Close(Attrs);
    }

    static Handle<Value> NumwinsGetter (Local<String> property,
                                        const AccessorInfo& info) {
      HandleScope scope;

      return scope.Close(Integer::New(wincounter));
    }

    Window() : ObjectWrap() {
      panel_ = NULL;
      this->init();
      assert(panel_ != NULL);
    }

    Window(int nlines, int ncols, int begin_y, int begin_x) : ObjectWrap() {
      panel_ = NULL;
      this->init(nlines, ncols, begin_y, begin_x);
      assert(panel_ != NULL);
    }

    ~Window() {
      Emit.Dispose();
      Emit.Clear();
    }

  private:
    static void io_event (uv_poll_t* w, int status, int revents) {
      HandleScope scope;

      if (status < 0)
        return;

      if (revents & UV_READABLE) {
        wint_t chr;
        int ret;
        wchar_t tmp[2];
        tmp[1] = 0;
        while ((ret = get_wch(&chr)) != ERR) {
          // 410 == KEY_RESIZE
          if (chr == 410 || !topmost_panel || !topmost_panel->getWindow()
              || !topmost_panel->getPanel()) {
            //if (chr != 410)
            //  ungetch(chr);
            return;
          }
          tmp[0] = chr;

          Handle<Value> emit_argv[4] = {
            inputchar_symbol,
            String::New((const uint16_t*) tmp),
            Integer::New(chr),
            Boolean::New(ret == KEY_CODE_YES)
          };
          TryCatch try_catch;
          topmost_panel->getWindow()->Emit->Call(
              topmost_panel->getWindow()->handle_, 4, emit_argv
          );
          if (try_catch.HasCaught())
            FatalException(try_catch);
        }
      }
    }

    MyPanel *panel_;
};

extern "C" {
  void init (Handle<Object> target) {
    HandleScope scope;
    Window::Initialize(target);
  }

  NODE_MODULE(binding, init);
}
