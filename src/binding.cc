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
#include <nan.h>

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
static bool start_rw_poll = true;

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
uv_poll_t* read_watcher_ = NULL;

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
      NanScope();

      Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
      Local<String> name = NanNew("Window");

      NanAssignPersistent(window_constructor, tpl);
      tpl->InstanceTemplate()->SetInternalFieldCount(1);
      tpl->SetClassName(name);

      NanAssignPersistent(emit_symbol, NanNew("emit"));
      NanAssignPersistent(inputchar_symbol, NanNew("inputChar"));
      NanAssignPersistent(echo_state_symbol, NanNew("echo"));
      NanAssignPersistent(showcursor_state_symbol, NanNew("showCursor"));
      NanAssignPersistent(lines_state_symbol, NanNew("lines"));
      NanAssignPersistent(cols_state_symbol, NanNew("cols"));
      NanAssignPersistent(tabsize_state_symbol, NanNew("tabsize"));
      NanAssignPersistent(hasmouse_state_symbol, NanNew("hasMouse"));
      NanAssignPersistent(hascolors_state_symbol, NanNew("hasColors"));
      NanAssignPersistent(numcolors_state_symbol, NanNew("numColors"));
      NanAssignPersistent(maxcolorpairs_state_symbol, NanNew("maxColorPairs"));
      NanAssignPersistent(raw_state_symbol, NanNew("raw"));
      NanAssignPersistent(bkgd_state_symbol, NanNew("bkgd"));
      NanAssignPersistent(hidden_state_symbol, NanNew("hidden"));
      NanAssignPersistent(height_state_symbol, NanNew("height"));
      NanAssignPersistent(width_state_symbol, NanNew("width"));
      NanAssignPersistent(begx_state_symbol, NanNew("begx"));
      NanAssignPersistent(begy_state_symbol, NanNew("begy"));
      NanAssignPersistent(curx_state_symbol, NanNew("curx"));
      NanAssignPersistent(cury_state_symbol, NanNew("cury"));
      NanAssignPersistent(maxx_state_symbol, NanNew("maxx"));
      NanAssignPersistent(maxy_state_symbol, NanNew("maxy"));
      NanAssignPersistent(wintouched_state_symbol, NanNew("touched"));
      NanAssignPersistent(numwins_symbol, NanNew("numwins"));
      NanAssignPersistent(ACS_symbol, NanNew("ACS"));
      NanAssignPersistent(keys_symbol, NanNew("keys"));
      NanAssignPersistent(colors_symbol, NanNew("colors"));
      NanAssignPersistent(attrs_symbol, NanNew("attrs"));

      /* Panel-specific methods */
      // TODO: color_set?, overlay, overwrite
      NODE_SET_PROTOTYPE_METHOD(tpl, "clearok", Clearok);
      NODE_SET_PROTOTYPE_METHOD(tpl, "scrollok", Scrollok);
      NODE_SET_PROTOTYPE_METHOD(tpl, "idlok", Idlok);
      NODE_SET_PROTOTYPE_METHOD(tpl, "idcok", Idcok);
      NODE_SET_PROTOTYPE_METHOD(tpl, "leaveok", Leaveok);
      NODE_SET_PROTOTYPE_METHOD(tpl, "syncok", Syncok);
      NODE_SET_PROTOTYPE_METHOD(tpl, "immedok", Immedok);
      NODE_SET_PROTOTYPE_METHOD(tpl, "keypad", Keypad);
      NODE_SET_PROTOTYPE_METHOD(tpl, "meta", Meta);
      NODE_SET_PROTOTYPE_METHOD(tpl, "standout", Standout);
      NODE_SET_PROTOTYPE_METHOD(tpl, "hide", Hide);
      NODE_SET_PROTOTYPE_METHOD(tpl, "show", Show);
      NODE_SET_PROTOTYPE_METHOD(tpl, "top", Top);
      NODE_SET_PROTOTYPE_METHOD(tpl, "bottom", Bottom);
      NODE_SET_PROTOTYPE_METHOD(tpl, "move", Mvwin);
      NODE_SET_PROTOTYPE_METHOD(tpl, "refresh", Refresh);
      NODE_SET_PROTOTYPE_METHOD(tpl, "noutrefresh", Noutrefresh);
      NODE_SET_PROTOTYPE_METHOD(tpl, "frame", Frame);
      NODE_SET_PROTOTYPE_METHOD(tpl, "boldframe", Boldframe);
      NODE_SET_PROTOTYPE_METHOD(tpl, "label", Label);
      NODE_SET_PROTOTYPE_METHOD(tpl, "centertext", Centertext);
      NODE_SET_PROTOTYPE_METHOD(tpl, "cursor", Move);
      NODE_SET_PROTOTYPE_METHOD(tpl, "insertln", Insertln);
      NODE_SET_PROTOTYPE_METHOD(tpl, "insdelln", Insdelln);
      NODE_SET_PROTOTYPE_METHOD(tpl, "insstr", Insstr);
      NODE_SET_PROTOTYPE_METHOD(tpl, "attron", Attron);
      NODE_SET_PROTOTYPE_METHOD(tpl, "attroff", Attroff);
      NODE_SET_PROTOTYPE_METHOD(tpl, "attrset", Attrset);
      NODE_SET_PROTOTYPE_METHOD(tpl, "attrget", Attrget);
      NODE_SET_PROTOTYPE_METHOD(tpl, "box", Box);
      NODE_SET_PROTOTYPE_METHOD(tpl, "border", Border);
      NODE_SET_PROTOTYPE_METHOD(tpl, "hline", Hline);
      NODE_SET_PROTOTYPE_METHOD(tpl, "vline", Vline);
      NODE_SET_PROTOTYPE_METHOD(tpl, "erase", Erase);
      NODE_SET_PROTOTYPE_METHOD(tpl, "clear", Clear);
      NODE_SET_PROTOTYPE_METHOD(tpl, "clrtobot", Clrtobot);
      NODE_SET_PROTOTYPE_METHOD(tpl, "clrtoeol", Clrtoeol);
      NODE_SET_PROTOTYPE_METHOD(tpl, "delch", Delch);
      NODE_SET_PROTOTYPE_METHOD(tpl, "deleteln", Deleteln);
      NODE_SET_PROTOTYPE_METHOD(tpl, "scroll", Scroll);
      NODE_SET_PROTOTYPE_METHOD(tpl, "setscrreg", Setscrreg);
      NODE_SET_PROTOTYPE_METHOD(tpl, "touchlines", Touchln);
      NODE_SET_PROTOTYPE_METHOD(tpl, "is_linetouched", Is_linetouched);
      NODE_SET_PROTOTYPE_METHOD(tpl, "redrawln", Redrawln);
      NODE_SET_PROTOTYPE_METHOD(tpl, "touch", Touchwin);
      NODE_SET_PROTOTYPE_METHOD(tpl, "untouch", Untouchwin);
      NODE_SET_PROTOTYPE_METHOD(tpl, "resize", Wresize);
      NODE_SET_PROTOTYPE_METHOD(tpl, "print", Print);
      NODE_SET_PROTOTYPE_METHOD(tpl, "addstr", Addstr);
      NODE_SET_PROTOTYPE_METHOD(tpl, "close", Close);
      NODE_SET_PROTOTYPE_METHOD(tpl, "syncdown", Syncdown);
      NODE_SET_PROTOTYPE_METHOD(tpl, "syncup", Syncup);
      NODE_SET_PROTOTYPE_METHOD(tpl, "cursyncup", Cursyncup);
      NODE_SET_PROTOTYPE_METHOD(tpl, "copywin", Copywin);
      NODE_SET_PROTOTYPE_METHOD(tpl, "redraw", Redrawwin);

      /* Attribute-related window functions */
      NODE_SET_PROTOTYPE_METHOD(tpl, "addch", Addch);
      NODE_SET_PROTOTYPE_METHOD(tpl, "echochar", Echochar);
      NODE_SET_PROTOTYPE_METHOD(tpl, "inch", Inch);
      NODE_SET_PROTOTYPE_METHOD(tpl, "insch", Insch);
      NODE_SET_PROTOTYPE_METHOD(tpl, "chgat", Chgat);
      //NODE_SET_PROTOTYPE_METHOD(tpl, "addchstr", Addchstr);
      //NODE_SET_PROTOTYPE_METHOD(tpl, "inchstr", Inchstr);

      /* Window properties */
      tpl->PrototypeTemplate()->SetAccessor(NanNew(bkgd_state_symbol),
                                                           BkgdStateGetter,
                                                           BkgdStateSetter);
      tpl->PrototypeTemplate()->SetAccessor(NanNew(hidden_state_symbol),
                                                           HiddenStateGetter);
      tpl->PrototypeTemplate()->SetAccessor(NanNew(height_state_symbol),
                                                           HeightStateGetter);
      tpl->PrototypeTemplate()->SetAccessor(NanNew(width_state_symbol),
                                                           WidthStateGetter);
      tpl->PrototypeTemplate()->SetAccessor(NanNew(begx_state_symbol),
                                                           BegxStateGetter);
      tpl->PrototypeTemplate()->SetAccessor(NanNew(begy_state_symbol),
                                                           BegyStateGetter);
      tpl->PrototypeTemplate()->SetAccessor(NanNew(curx_state_symbol),
                                                           CurxStateGetter);
      tpl->PrototypeTemplate()->SetAccessor(NanNew(cury_state_symbol),
                                                           CuryStateGetter);
      tpl->PrototypeTemplate()->SetAccessor(NanNew(maxx_state_symbol),
                                                           MaxxStateGetter);
      tpl->PrototypeTemplate()->SetAccessor(NanNew(maxy_state_symbol),
                                                           MaxyStateGetter);
      tpl->PrototypeTemplate()->SetAccessor(NanNew(wintouched_state_symbol),
                                                           WintouchedStateGetter);

      /* Global/Terminal properties and functions */
      target->SetAccessor(NanNew(echo_state_symbol), EchoStateGetter, EchoStateSetter);
      target->SetAccessor(NanNew(showcursor_state_symbol), ShowcursorStateGetter,
                          ShowcursorStateSetter);
      target->SetAccessor(NanNew(lines_state_symbol), LinesStateGetter);
      target->SetAccessor(NanNew(cols_state_symbol), ColsStateGetter);
      target->SetAccessor(NanNew(tabsize_state_symbol), TabsizeStateGetter);
      target->SetAccessor(NanNew(hasmouse_state_symbol), HasmouseStateGetter);
      target->SetAccessor(NanNew(hascolors_state_symbol), HascolorsStateGetter);
      target->SetAccessor(NanNew(numcolors_state_symbol), NumcolorsStateGetter);
      target->SetAccessor(NanNew(maxcolorpairs_state_symbol), MaxcolorpairsStateGetter);
      target->SetAccessor(NanNew(raw_state_symbol), RawStateGetter, RawStateSetter);
      target->SetAccessor(NanNew(numwins_symbol), NumwinsGetter);
      target->SetAccessor(NanNew(ACS_symbol), ACSConstsGetter);
      target->SetAccessor(NanNew(keys_symbol), KeyConstsGetter);
      target->SetAccessor(NanNew(colors_symbol), ColorConstsGetter);
      target->SetAccessor(NanNew(attrs_symbol), AttrConstsGetter);
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

      target->Set(name, tpl->GetFunction());
    }

    void init(int nlines=-1, int ncols=-1, int begin_y=-1, int begin_x=-1) {
      static bool initialize_ACS = true;
      if (stdin_fd < 0) {
        stdin_fd = STDIN_FILENO;
        int stdin_flags = fcntl(stdin_fd, F_GETFL, 0);
        int r = fcntl(stdin_fd, F_SETFL, stdin_flags | O_NONBLOCK);
        if (r < 0) {
          NanThrowError("Unable to set stdin to non-block");
          return;
        }
      }

      if (read_watcher_ == NULL) {
        read_watcher_ = new uv_poll_t;
        read_watcher_->data = this;
        // Setup input listener
        uv_poll_init(uv_default_loop(), read_watcher_, stdin_fd);
      }

      if (start_rw_poll) {
        // Start input listener
        uv_poll_start(read_watcher_, UV_READABLE, io_event);
        start_rw_poll = false;
      }

      if (nlines < 0 || ncols < 0 || begin_y < 0 || begin_x < 0)
        panel_ = new MyPanel(this);
      else
        panel_ = new MyPanel(this, nlines, ncols, begin_y, begin_x);

      if (initialize_ACS) {
        initialize_ACS = false;

        Local<Object> obj = NanNew<Object>();
        obj->Set(NanNew("ULCORNER"), NanNew<Uint32>(ACS_ULCORNER));
        obj->Set(NanNew("LLCORNER"), NanNew<Uint32>(ACS_LLCORNER));
        obj->Set(NanNew("URCORNER"), NanNew<Uint32>(ACS_URCORNER));
        obj->Set(NanNew("LRCORNER"), NanNew<Uint32>(ACS_LRCORNER));
        obj->Set(NanNew("LTEE"), NanNew<Uint32>(ACS_LTEE));
        obj->Set(NanNew("RTEE"), NanNew<Uint32>(ACS_RTEE));
        obj->Set(NanNew("BTEE"), NanNew<Uint32>(ACS_BTEE));
        obj->Set(NanNew("TTEE"), NanNew<Uint32>(ACS_TTEE));
        obj->Set(NanNew("HLINE"), NanNew<Uint32>(ACS_HLINE));
        obj->Set(NanNew("VLINE"), NanNew<Uint32>(ACS_VLINE));
        obj->Set(NanNew("PLUS"), NanNew<Uint32>(ACS_PLUS));
        obj->Set(NanNew("S1"), NanNew<Uint32>(ACS_S1));
        obj->Set(NanNew("S9"), NanNew<Uint32>(ACS_S9));
        obj->Set(NanNew("DIAMOND"), NanNew<Uint32>(ACS_DIAMOND));
        obj->Set(NanNew("CKBOARD"), NanNew<Uint32>(ACS_CKBOARD));
        obj->Set(NanNew("DEGREE"), NanNew<Uint32>(ACS_DEGREE));
        obj->Set(NanNew("PLMINUS"), NanNew<Uint32>(ACS_PLMINUS));
        obj->Set(NanNew("BULLET"), NanNew<Uint32>(ACS_BULLET));
        obj->Set(NanNew("LARROW"), NanNew<Uint32>(ACS_LARROW));
        obj->Set(NanNew("RARROW"), NanNew<Uint32>(ACS_RARROW));
        obj->Set(NanNew("DARROW"), NanNew<Uint32>(ACS_DARROW));
        obj->Set(NanNew("UARROW"), NanNew<Uint32>(ACS_UARROW));
        obj->Set(NanNew("BOARD"), NanNew<Uint32>(ACS_BOARD));
        obj->Set(NanNew("LANTERN"), NanNew<Uint32>(ACS_LANTERN));
        obj->Set(NanNew("BLOCK"), NanNew<Uint32>(ACS_BLOCK));
        NanAssignPersistent(ACS_Chars, obj);

        obj = NanNew<Object>();
        obj->Set(NanNew("SPACE"), NanNew<Uint32>(32));
        obj->Set(NanNew("NEWLINE"), NanNew<Uint32>(10));
        obj->Set(NanNew("ESC"), NanNew<Uint32>(27));
        obj->Set(NanNew("UP"), NanNew<Uint32>(KEY_UP));
        obj->Set(NanNew("DOWN"), NanNew<Uint32>(KEY_DOWN));
        obj->Set(NanNew("LEFT"), NanNew<Uint32>(KEY_LEFT));
        obj->Set(NanNew("RIGHT"), NanNew<Uint32>(KEY_RIGHT));
        obj->Set(NanNew("HOME"), NanNew<Uint32>(KEY_HOME));
        obj->Set(NanNew("BACKSPACE"), NanNew<Uint32>(KEY_BACKSPACE));
        obj->Set(NanNew("BREAK"), NanNew<Uint32>(KEY_BREAK));
        obj->Set(NanNew("F0"), NanNew<Uint32>(KEY_F(0)));
        obj->Set(NanNew("F1"), NanNew<Uint32>(KEY_F(1)));
        obj->Set(NanNew("F2"), NanNew<Uint32>(KEY_F(2)));
        obj->Set(NanNew("F3"), NanNew<Uint32>(KEY_F(3)));
        obj->Set(NanNew("F4"), NanNew<Uint32>(KEY_F(4)));
        obj->Set(NanNew("F5"), NanNew<Uint32>(KEY_F(5)));
        obj->Set(NanNew("F6"), NanNew<Uint32>(KEY_F(6)));
        obj->Set(NanNew("F7"), NanNew<Uint32>(KEY_F(7)));
        obj->Set(NanNew("F8"), NanNew<Uint32>(KEY_F(8)));
        obj->Set(NanNew("F9"), NanNew<Uint32>(KEY_F(9)));
        obj->Set(NanNew("F10"), NanNew<Uint32>(KEY_F(10)));
        obj->Set(NanNew("F11"), NanNew<Uint32>(KEY_F(11)));
        obj->Set(NanNew("F12"), NanNew<Uint32>(KEY_F(12)));
        obj->Set(NanNew("DL"), NanNew<Uint32>(KEY_DL));
        obj->Set(NanNew("IL"), NanNew<Uint32>(KEY_IL));
        obj->Set(NanNew("DEL"), NanNew<Uint32>(KEY_DC));
        obj->Set(NanNew("INS"), NanNew<Uint32>(KEY_IC));
        obj->Set(NanNew("EIC"), NanNew<Uint32>(KEY_EIC));
        obj->Set(NanNew("CLEAR"), NanNew<Uint32>(KEY_CLEAR));
        obj->Set(NanNew("EOS"), NanNew<Uint32>(KEY_EOS));
        obj->Set(NanNew("EOL"), NanNew<Uint32>(KEY_EOL));
        obj->Set(NanNew("SF"), NanNew<Uint32>(KEY_SF));
        obj->Set(NanNew("SR"), NanNew<Uint32>(KEY_SR));
        obj->Set(NanNew("NPAGE"), NanNew<Uint32>(KEY_NPAGE));
        obj->Set(NanNew("PPAGE"), NanNew<Uint32>(KEY_PPAGE));
        obj->Set(NanNew("STAB"), NanNew<Uint32>(KEY_STAB));
        obj->Set(NanNew("CTAB"), NanNew<Uint32>(KEY_CTAB));
        obj->Set(NanNew("CATAB"), NanNew<Uint32>(KEY_CATAB));
        obj->Set(NanNew("ENTER"), NanNew<Uint32>(KEY_ENTER));
        obj->Set(NanNew("SRESET"), NanNew<Uint32>(KEY_SRESET));
        obj->Set(NanNew("RESET"), NanNew<Uint32>(KEY_RESET));
        obj->Set(NanNew("PRINT"), NanNew<Uint32>(KEY_PRINT));
        obj->Set(NanNew("LL"), NanNew<Uint32>(KEY_LL));
        obj->Set(NanNew("UPLEFT"), NanNew<Uint32>(KEY_A1));
        obj->Set(NanNew("UPRIGHT"), NanNew<Uint32>(KEY_A3));
        obj->Set(NanNew("CENTER"), NanNew<Uint32>(KEY_B2));
        obj->Set(NanNew("DOWNLEFT"), NanNew<Uint32>(KEY_C1));
        obj->Set(NanNew("DOWNRIGHT"), NanNew<Uint32>(KEY_C3));
        obj->Set(NanNew("BTAB"), NanNew<Uint32>(KEY_BTAB));
        obj->Set(NanNew("BEG"), NanNew<Uint32>(KEY_BEG));
        obj->Set(NanNew("CANCEL"), NanNew<Uint32>(KEY_CANCEL));
        obj->Set(NanNew("CLOSE"), NanNew<Uint32>(KEY_CLOSE));
        obj->Set(NanNew("COMMAND"), NanNew<Uint32>(KEY_COMMAND));
        obj->Set(NanNew("COPY"), NanNew<Uint32>(KEY_COPY));
        obj->Set(NanNew("CREATE"), NanNew<Uint32>(KEY_CREATE));
        obj->Set(NanNew("END"), NanNew<Uint32>(KEY_END));
        obj->Set(NanNew("EXIT"), NanNew<Uint32>(KEY_EXIT));
        obj->Set(NanNew("FIND"), NanNew<Uint32>(KEY_FIND));
        obj->Set(NanNew("FIND"), NanNew<Uint32>(KEY_HELP));
        obj->Set(NanNew("MARK"), NanNew<Uint32>(KEY_MARK));
        obj->Set(NanNew("MESSAGE"), NanNew<Uint32>(KEY_MESSAGE));
        obj->Set(NanNew("MOVE"), NanNew<Uint32>(KEY_MOVE));
        obj->Set(NanNew("NEXT"), NanNew<Uint32>(KEY_NEXT));
        obj->Set(NanNew("OPEN"), NanNew<Uint32>(KEY_OPEN));
        obj->Set(NanNew("OPTIONS"), NanNew<Uint32>(KEY_OPTIONS));
        obj->Set(NanNew("PREVIOUS"), NanNew<Uint32>(KEY_PREVIOUS));
        obj->Set(NanNew("REDO"), NanNew<Uint32>(KEY_REDO));
        obj->Set(NanNew("REFERENCE"), NanNew<Uint32>(KEY_REFERENCE));
        obj->Set(NanNew("REFRESH"), NanNew<Uint32>(KEY_REFRESH));
        obj->Set(NanNew("REPLACE"), NanNew<Uint32>(KEY_REPLACE));
        obj->Set(NanNew("RESTART"), NanNew<Uint32>(KEY_RESTART));
        obj->Set(NanNew("RESUME"), NanNew<Uint32>(KEY_RESUME));
        obj->Set(NanNew("SAVE"), NanNew<Uint32>(KEY_SAVE));
        obj->Set(NanNew("S_BEG"), NanNew<Uint32>(KEY_SBEG));
        obj->Set(NanNew("S_CANCEL"), NanNew<Uint32>(KEY_SCANCEL));
        obj->Set(NanNew("S_COMMAND"), NanNew<Uint32>(KEY_SCOMMAND));
        obj->Set(NanNew("S_COPY"), NanNew<Uint32>(KEY_SCOPY));
        obj->Set(NanNew("S_CREATE"), NanNew<Uint32>(KEY_SCREATE));
        obj->Set(NanNew("S_DC"), NanNew<Uint32>(KEY_SDC));
        obj->Set(NanNew("S_DL"), NanNew<Uint32>(KEY_SDL));
        obj->Set(NanNew("SELECT"), NanNew<Uint32>(KEY_SELECT));
        obj->Set(NanNew("SEND"), NanNew<Uint32>(KEY_SEND));
        obj->Set(NanNew("S_EOL"), NanNew<Uint32>(KEY_SEOL));
        obj->Set(NanNew("S_EXIT"), NanNew<Uint32>(KEY_SEXIT));
        obj->Set(NanNew("S_FIND"), NanNew<Uint32>(KEY_SFIND));
        obj->Set(NanNew("S_HELP"), NanNew<Uint32>(KEY_SHELP));
        obj->Set(NanNew("S_HOME"), NanNew<Uint32>(KEY_SHOME));
        obj->Set(NanNew("S_IC"), NanNew<Uint32>(KEY_SIC));
        obj->Set(NanNew("S_LEFT"), NanNew<Uint32>(KEY_SLEFT));
        obj->Set(NanNew("S_MESSAGE"), NanNew<Uint32>(KEY_SMESSAGE));
        obj->Set(NanNew("S_MOVE"), NanNew<Uint32>(KEY_SMOVE));
        obj->Set(NanNew("S_NEXT"), NanNew<Uint32>(KEY_SNEXT));
        obj->Set(NanNew("S_OPTIONS"), NanNew<Uint32>(KEY_SOPTIONS));
        obj->Set(NanNew("S_PREVIOUS"), NanNew<Uint32>(KEY_SPREVIOUS));
        obj->Set(NanNew("S_PRINT"), NanNew<Uint32>(KEY_SPRINT));
        obj->Set(NanNew("S_REDO"), NanNew<Uint32>(KEY_SREDO));
        obj->Set(NanNew("S_REPLACE"), NanNew<Uint32>(KEY_SREPLACE));
        obj->Set(NanNew("S_RIGHT"), NanNew<Uint32>(KEY_SRIGHT));
        obj->Set(NanNew("S_RESUME"), NanNew<Uint32>(KEY_SRSUME));
        obj->Set(NanNew("S_SAVE"), NanNew<Uint32>(KEY_SSAVE));
        obj->Set(NanNew("S_SUSPEND"), NanNew<Uint32>(KEY_SSUSPEND));
        obj->Set(NanNew("S_UNDO"), NanNew<Uint32>(KEY_SUNDO));
        obj->Set(NanNew("SUSPEND"), NanNew<Uint32>(KEY_SUSPEND));
        obj->Set(NanNew("UNDO"), NanNew<Uint32>(KEY_UNDO));
        obj->Set(NanNew("MOUSE"), NanNew<Uint32>(KEY_MOUSE));
        obj->Set(NanNew("RESIZE"), NanNew<Uint32>(KEY_RESIZE));
        NanAssignPersistent(Keys, obj);

        obj = NanNew<Object>();
        obj->Set(NanNew("BLACK"), NanNew<Uint32>(COLOR_BLACK));
        obj->Set(NanNew("RED"), NanNew<Uint32>(COLOR_RED));
        obj->Set(NanNew("GREEN"), NanNew<Uint32>(COLOR_GREEN));
        obj->Set(NanNew("YELLOW"), NanNew<Uint32>(COLOR_YELLOW));
        obj->Set(NanNew("BLUE"), NanNew<Uint32>(COLOR_BLUE));
        obj->Set(NanNew("MAGENTA"), NanNew<Uint32>(COLOR_MAGENTA));
        obj->Set(NanNew("CYAN"), NanNew<Uint32>(COLOR_CYAN));
        obj->Set(NanNew("WHITE"), NanNew<Uint32>(COLOR_WHITE));
        NanAssignPersistent(Colors, obj);

        obj = NanNew<Object>();
        obj->Set(NanNew("NORMAL"), NanNew<Uint32>(A_NORMAL));
        obj->Set(NanNew("STANDOUT"), NanNew<Uint32>(A_STANDOUT));
        obj->Set(NanNew("UNDERLINE"), NanNew<Uint32>(A_UNDERLINE));
        obj->Set(NanNew("REVERSE"), NanNew<Uint32>(A_REVERSE));
        obj->Set(NanNew("BLINK"), NanNew<Uint32>(A_BLINK));
        obj->Set(NanNew("DIM"), NanNew<Uint32>(A_DIM));
        obj->Set(NanNew("BOLD"), NanNew<Uint32>(A_BOLD));
        obj->Set(NanNew("INVISIBLE"), NanNew<Uint32>(A_INVIS));
        obj->Set(NanNew("PROTECT"), NanNew<Uint32>(A_PROTECT));
        NanAssignPersistent(Attrs, obj);
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
          read_watcher_ = NULL;
          stdin_fd = -1;
          start_rw_poll = true;
        }
        delete panel_;
        panel_ = NULL;
        MyPanel::updateTopmost();
        if (wasStdscr)
          ::endwin();
      }
    }

  protected:
    static NAN_METHOD(New) {
      NanScope();

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
        return NanThrowError("Invalid number and/or types of arguments");
      }

      win->Wrap(args.This());
      win->Ref();

      NanAssignPersistent(win->Emit, NanObjectWrapHandle(win)->Get(NanNew(emit_symbol)).As<Function>());

      NanReturnValue(args.This());
    }

    static NAN_METHOD(Close) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      if (win->panel() != NULL)
        win->close();

      NanReturnUndefined();
    }

    static NAN_METHOD(Hide) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      win->panel()->hide();

      NanReturnUndefined();
    }

    static NAN_METHOD(Show) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      win->panel()->show();

      NanReturnUndefined();
    }

    static NAN_METHOD(Top) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      win->panel()->top();

      NanReturnUndefined();
    }

    static NAN_METHOD(Bottom) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      win->panel()->bottom();

      NanReturnUndefined();
    }

    static NAN_METHOD(Mvwin) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret = 0;
      if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
        ret = win->panel()->mvwin(args[0]->Int32Value(), args[1]->Int32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Refresh) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret = win->panel()->refresh();

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Noutrefresh) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret = win->panel()->noutrefresh();

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Redraw) {
      NanScope();

      MyPanel::redraw();

      NanReturnUndefined();
    }

    static NAN_METHOD(Frame) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

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
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnUndefined();
    }

    static NAN_METHOD(Boldframe) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

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
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnUndefined();
    }

    static NAN_METHOD(Label) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      if (args.Length() == 1 && args[0]->IsString()) {
        String::Utf8Value str(args[0]->ToString());
        win->panel()->label(ToCString(str), NULL);
      } else if (args.Length() == 2 && args[0]->IsString()
                 && args[1]->IsString()) {
        String::Utf8Value str0(args[0]->ToString());
        String::Utf8Value str1(args[1]->ToString());
        win->panel()->label(ToCString(str0), ToCString(str1));
      } else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnUndefined();
    }

    static NAN_METHOD(Centertext) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsString()) {
        String::Utf8Value str(args[1]->ToString());
        win->panel()->centertext(args[0]->Int32Value(), ToCString(str));
      } else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnUndefined();
    }

    static NAN_METHOD(Move) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
        ret = win->panel()->move(args[0]->Int32Value(), args[1]->Int32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Addch) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = win->panel()->addch(args[0]->Uint32Value());
      else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
               && args[2]->IsUint32()) {
        ret = win->panel()->addch(args[0]->Int32Value(), args[1]->Int32Value(),
                                  args[2]->Uint32Value());
      } else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Echochar) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = win->panel()->echochar(args[0]->Uint32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Addstr) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

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
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    // FIXME: addchstr requires a pointer to a chtype not an actual value,
    //        unlike the other ACS_*-using methods
    /*static NAN_METHOD(Addchstr) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

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
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }*/

    static NAN_METHOD(Inch) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      unsigned int ret;
      if (args.Length() == 0)
        ret = win->panel()->inch();
      else if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
        ret = win->panel()->inch(args[0]->Int32Value(), args[1]->Int32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    // FIXME: Need pointer to chtype instead of actual value
    /*static NAN_METHOD(Inchstr) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

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
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }*/

    static NAN_METHOD(Insch) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = win->panel()->insch(args[0]->Uint32Value());
      else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
               && args[2]->IsUint32()) {
        ret = win->panel()->insch(args[0]->Int32Value(), args[1]->Int32Value(),
                                  args[2]->Uint32Value());
      } else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Chgat) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

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
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Insertln) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret = win->panel()->insertln();

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Insdelln) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 0)
        ret = win->panel()->insdelln(1);
      else if (args.Length() == 1 && args[0]->IsInt32())
        ret = win->panel()->insdelln(args[0]->Int32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Insstr) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

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
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Attron) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = win->panel()->attron(args[0]->Uint32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Attroff) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = win->panel()->attroff(args[0]->Uint32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Attrset) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = win->panel()->attrset(args[0]->Uint32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Attrget) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      unsigned int ret = win->panel()->attrget();

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Box) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 0)
        ret = win->panel()->box(0, 0);
      else if (args.Length() == 1 && args[0]->IsUint32())
        ret = win->panel()->box(args[0]->Uint32Value(), 0);
      else if (args.Length() == 2 && args[0]->IsUint32() && args[1]->IsUint32())
        ret = win->panel()->box(args[0]->Uint32Value(), args[1]->Uint32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Border) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

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
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Hline) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

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
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Vline) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

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
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Erase) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret = win->panel()->erase();

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Clear) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret = win->panel()->clear();

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Clrtobot) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret = win->panel()->clrtobot();

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Clrtoeol) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret = win->panel()->clrtoeol();

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Delch) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 0)
        ret = win->panel()->delch();
      else if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32())
        ret = win->panel()->delch(args[0]->Int32Value(), args[1]->Int32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Deleteln) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret = win->panel()->deleteln();

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Scroll) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 0)
        ret = win->panel()->scroll(1);
      else if (args.Length() == 1 && args[0]->IsInt32())
        ret = win->panel()->scroll(args[0]->Int32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Setscrreg) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32()) {
        ret = win->panel()->setscrreg(args[0]->Int32Value(),
                                      args[1]->Int32Value());
      } else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    /*static NAN_METHOD(Touchline) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32()) {
        ret = win->panel()->touchline(args[0]->Int32Value(),
                                      args[1]->Int32Value());
      } else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }*/

    static NAN_METHOD(Touchwin) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret = win->panel()->touchwin();

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Untouchwin) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret = win->panel()->untouchwin();

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Touchln) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32()) {
        ret = win->panel()->touchln(args[0]->Int32Value(),
                                    args[1]->Int32Value(), true);
      } else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
                 && args[2]->IsBoolean()) {
        ret = win->panel()->touchln(args[0]->Int32Value(), args[1]->Int32Value(),
                                    args[2]->BooleanValue());
      } else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Is_linetouched) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      bool ret;
      if (args.Length() == 1 && args[0]->IsInt32())
        ret = win->panel()->is_linetouched(args[0]->Int32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Boolean>(ret));
    }

    static NAN_METHOD(Redrawln) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32()) {
        ret = win->panel()->redrawln(args[0]->Int32Value(),
                                     args[1]->Int32Value());
      } else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Syncdown) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      win->panel()->syncdown();

      NanReturnUndefined();
    }

    static NAN_METHOD(Syncup) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      win->panel()->syncup();

      NanReturnUndefined();
    }

    static NAN_METHOD(Cursyncup) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      win->panel()->cursyncup();

      NanReturnUndefined();
    }

    static NAN_METHOD(Wresize) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 2 && args[0]->IsInt32() && args[1]->IsInt32()) {
        ret = win->panel()->wresize(args[0]->Int32Value(),
                                    args[1]->Int32Value());
      } else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Print) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

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
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Clearok) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsBoolean())
        ret = win->panel()->clearok(args[0]->BooleanValue());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Scrollok) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsBoolean())
        ret = win->panel()->scrollok(args[0]->BooleanValue());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Idlok) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsBoolean())
        ret = win->panel()->idlok(args[0]->BooleanValue());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Idcok) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      if (args.Length() == 1 && args[0]->IsBoolean())
        win->panel()->idcok(args[0]->BooleanValue());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnUndefined();
    }

    static NAN_METHOD(Leaveok) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsBoolean())
        ret = win->panel()->leaveok(args[0]->BooleanValue());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Syncok) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsBoolean())
        ret = win->panel()->syncok(args[0]->BooleanValue());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Immedok) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      if (args.Length() == 1 && args[0]->IsBoolean())
        win->panel()->immedok(args[0]->BooleanValue());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnUndefined();
    }

    static NAN_METHOD(Keypad) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsBoolean())
        ret = win->panel()->keypad(args[0]->BooleanValue());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Meta) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsBoolean())
        ret = win->panel()->meta(args[0]->BooleanValue());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Standout) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsBoolean()) {
        if (args[0]->BooleanValue())
          ret = win->panel()->standout();
        else
          ret = win->panel()->standend();
      } else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Resetscreen) {
      NanScope();

      ::endwin(); //MyPanel::resetScreen();

      NanReturnUndefined();
    }

    static NAN_METHOD(Setescdelay) {
      NanScope();

      if (args.Length() == 0 || !args[0]->IsInt32()) {
        return NanThrowError("Invalid argument");
      }

      ::set_escdelay(args[0]->Int32Value());

      NanReturnUndefined();
    }

    static NAN_METHOD(LeaveNcurses) {
      NanScope();

      uv_poll_stop(read_watcher_);
      start_rw_poll = true;

      ::def_prog_mode();
      ::endwin();

      NanReturnUndefined();
    }

    static NAN_METHOD(RestoreNcurses) {
      NanScope();

      ::reset_prog_mode();
      if (start_rw_poll) {
        uv_poll_start(read_watcher_, UV_READABLE, io_event);
        start_rw_poll = false;
      }

      MyPanel::redraw();

      NanReturnUndefined();
    }

    static NAN_METHOD(Beep) {
      NanScope();

      ::beep();

      NanReturnUndefined();
    }

    static NAN_METHOD(Flash) {
      NanScope();

      ::flash();

      NanReturnUndefined();
    }

    static NAN_METHOD(DoUpdate) {
      NanScope();

      MyPanel::doUpdate();

      NanReturnUndefined();
    }

    static NAN_METHOD(Colorpair) {
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsInt32())
        ret = MyPanel::pair(args[0]->Int32Value());
      else if (args.Length() == 3 && args[0]->IsInt32() && args[1]->IsInt32()
               && args[2]->IsInt32()) {
        ret = MyPanel::pair(args[0]->Int32Value(), (short)args[1]->Int32Value(),
                            (short)args[2]->Int32Value());
      } else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Colorfg) {
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = MyPanel::getFgcolor(args[0]->Uint32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Colorbg) {
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = MyPanel::getBgcolor(args[0]->Uint32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Dup2) {
      NanScope();

      int ret;
      if (args.Length() == 2 && args[0]->IsUint32() && args[1]->IsUint32())
        ret = dup2(args[0]->Uint32Value(), args[1]->Uint32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Dup) {
      NanScope();

      int ret;
      if (args.Length() == 1 && args[0]->IsUint32())
        ret = dup(args[0]->Uint32Value());
      else {
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Copywin) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();
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
        return NanThrowError("Invalid number and/or types of arguments");
      }

      NanReturnValue(NanNew<Integer>(ret));
    }

    static NAN_METHOD(Redrawwin) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      NanScope();

      int ret = win->panel()->redrawwin();

      NanReturnValue(NanNew<Integer>(ret));
    }

    // -- Getters/Setters ------------------------------------------------------
    static NAN_GETTER(EchoStateGetter) {
      assert(property == echo_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Boolean>(MyPanel::echo()));
    }

    static NAN_SETTER(EchoStateSetter) {
      assert(property == echo_state_symbol);

      if (!value->IsBoolean()) {
          return NanThrowTypeError("echo should be of Boolean value");
      }

      MyPanel::echo(value->BooleanValue());
    }

   static NAN_GETTER(ShowcursorStateGetter) {
      assert(property == showcursor_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Boolean>(MyPanel::showCursor()));
    }

    static NAN_SETTER(ShowcursorStateSetter) {
      assert(property == showcursor_state_symbol);

      if (!value->IsBoolean()) {
          return NanThrowTypeError("showCursor should be of Boolean value");
      }
      MyPanel::showCursor(value->BooleanValue());
    }

    static NAN_GETTER(LinesStateGetter) {
      assert(property == lines_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Integer>(LINES));
    }

    static NAN_GETTER(ColsStateGetter) {
      assert(property == cols_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Integer>(COLS));
    }

    static NAN_GETTER(TabsizeStateGetter) {
      assert(property == tabsize_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Integer>(TABSIZE));
    }

    static NAN_GETTER(HasmouseStateGetter) {
      assert(property == hasmouse_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Boolean>(MyPanel::hasMouse()));
    }

    static NAN_GETTER(HiddenStateGetter) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      assert(win);
      assert(property == hidden_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Boolean>(win->panel()->hidden()));
    }

    static NAN_GETTER(HeightStateGetter) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      assert(win);
      assert(property == height_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Integer>(win->panel()->height()));
    }

    static NAN_GETTER(WidthStateGetter) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      assert(win);
      assert(property == width_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Integer>(win->panel()->width()));
    }

    static NAN_GETTER(BegxStateGetter) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      assert(win);
      assert(property == begx_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Integer>(win->panel()->begx()));
    }

    static NAN_GETTER(BegyStateGetter) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      assert(win);
      assert(property == begy_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Integer>(win->panel()->begy()));
    }

    static NAN_GETTER(CurxStateGetter) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      assert(win);
      assert(property == curx_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Integer>(win->panel()->curx()));
    }

    static NAN_GETTER(CuryStateGetter) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      assert(win);
      assert(property == cury_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Integer>(win->panel()->cury()));
    }

    static NAN_GETTER(MaxxStateGetter) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      assert(win);
      assert(property == maxx_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Integer>(win->panel()->maxx()));
    }

    static NAN_GETTER(MaxyStateGetter) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      assert(win);
      assert(property == maxy_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Integer>(win->panel()->maxy()));
    }

    static NAN_GETTER(BkgdStateGetter) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      assert(win);
      assert(property == bkgd_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Integer>(win->panel()->getbkgd()));
    }

    static NAN_SETTER(BkgdStateSetter) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      assert(win);
      assert(property == bkgd_state_symbol);
      unsigned int val = 32;

      if (!value->IsUint32() && !value->IsString()) {
          return NanThrowTypeError("bkgd should be of unsigned integer or a string value");
      }
      if (value->IsString()) {
        String::Utf8Value str(value->ToString());
        if (str.length() > 0)
          val = (unsigned int)((*str)[0]);
      } else
        val = value->Uint32Value();

      win->panel()->bkgd(val);
    }

    static NAN_GETTER(HascolorsStateGetter) {
      assert(property == hascolors_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Boolean>(MyPanel::has_colors()));
    }

    static NAN_GETTER(NumcolorsStateGetter) {
      assert(property == numcolors_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Integer>(MyPanel::num_colors()));
    }

    static NAN_GETTER(MaxcolorpairsStateGetter) {
      assert(property == maxcolorpairs_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Integer>(MyPanel::max_pairs()));
    }

    static NAN_GETTER(WintouchedStateGetter) {
      Window *win = ObjectWrap::Unwrap<Window>(args.This());
      assert(win);
      assert(property == wintouched_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Boolean>(win->panel()->is_wintouched()));
    }

    static NAN_GETTER(RawStateGetter) {
      assert(property == raw_state_symbol);

      NanScope();

      NanReturnValue(NanNew<Boolean>(MyPanel::raw()));
    }

    static NAN_SETTER(RawStateSetter) {
      assert(property == raw_state_symbol);

      if (!value->IsBoolean()) {
          return NanThrowTypeError("raw should be of Boolean value");
      }

      MyPanel::raw(value->BooleanValue());
    }

    static NAN_GETTER(ACSConstsGetter) {
      NanScope();

      NanReturnValue(NanNew(ACS_Chars));
    }

    static NAN_GETTER(KeyConstsGetter) {
      NanScope();

      NanReturnValue(NanNew(Keys));
    }

    static NAN_GETTER(ColorConstsGetter) {
      NanScope();

      NanReturnValue(NanNew(Colors));
    }

    static NAN_GETTER(AttrConstsGetter) {
      NanScope();

      NanReturnValue(NanNew(Attrs));
    }

    static NAN_GETTER(NumwinsGetter) {
      NanScope();

      NanReturnValue(NanNew<Integer>(wincounter));
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
      NanDisposePersistent(Emit);
    }

  private:
    static void io_event (uv_poll_t* w, int status, int revents) {
      NanScope();

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
            NanNew(inputchar_symbol),
            NanNew((const uint16_t*) tmp),
            NanNew<Integer>(chr),
            NanNew<Boolean>(ret == KEY_CODE_YES)
          };
          TryCatch try_catch;
          NanNew(topmost_panel->getWindow()->Emit)->Call(
              NanObjectWrapHandle(topmost_panel->getWindow()), 4, emit_argv
          );
          if (try_catch.HasCaught())
#if NODE_MODULE_VERSION < 12
            FatalException(try_catch);
#else
            FatalException(Isolate::GetCurrent(), try_catch);
#endif
        }
      }
    }

    MyPanel *panel_;
};

extern "C" {
  void init (Handle<Object> target) {
    NanScope();
    Window::Initialize(target);
  }

  NODE_MODULE(binding, init);
}
