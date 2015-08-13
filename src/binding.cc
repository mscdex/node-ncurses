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

static Nan::Persistent<FunctionTemplate> window_constructor;
static Nan::Persistent<String> emit_symbol;
static Nan::Persistent<String> inputchar_symbol;
static Nan::Persistent<String> echo_state_symbol;
static Nan::Persistent<String> showcursor_state_symbol;
static Nan::Persistent<String> lines_state_symbol;
static Nan::Persistent<String> cols_state_symbol;
static Nan::Persistent<String> tabsize_state_symbol;
static Nan::Persistent<String> hasmouse_state_symbol;
static Nan::Persistent<String> hascolors_state_symbol;
static Nan::Persistent<String> numcolors_state_symbol;
static Nan::Persistent<String> maxcolorpairs_state_symbol;
static Nan::Persistent<String> raw_state_symbol;
static Nan::Persistent<String> bkgd_state_symbol;
static Nan::Persistent<String> hidden_state_symbol;
static Nan::Persistent<String> height_state_symbol;
static Nan::Persistent<String> width_state_symbol;
static Nan::Persistent<String> begx_state_symbol;
static Nan::Persistent<String> begy_state_symbol;
static Nan::Persistent<String> curx_state_symbol;
static Nan::Persistent<String> cury_state_symbol;
static Nan::Persistent<String> maxx_state_symbol;
static Nan::Persistent<String> maxy_state_symbol;
static Nan::Persistent<String> wintouched_state_symbol;
static Nan::Persistent<String> numwins_symbol;
static Nan::Persistent<String> ACS_symbol;
static Nan::Persistent<String> keys_symbol;
static Nan::Persistent<String> colors_symbol;
static Nan::Persistent<String> attrs_symbol;
static Nan::Persistent<Object> ACS_Chars;
static Nan::Persistent<Object> Keys;
static Nan::Persistent<Object> Attrs;
static Nan::Persistent<Object> Colors;

// Extracts a C string from a Nan Utf8String.
const char* ToCString(const Nan::Utf8String& value) {
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

class Window : public Nan::ObjectWrap {
  public:
    Nan::Persistent<Function> Emit;
    static NAN_MODULE_INIT(Initialize) {
      Nan::HandleScope scope;

      Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
      Local<String> name = Nan::New("Window").ToLocalChecked();

      window_constructor.Reset(tpl);
      tpl->InstanceTemplate()->SetInternalFieldCount(1);
      tpl->SetClassName(name);

      emit_symbol.Reset(Nan::New("emit").ToLocalChecked());
      inputchar_symbol.Reset(Nan::New("inputChar").ToLocalChecked());
      echo_state_symbol.Reset(Nan::New("echo").ToLocalChecked());
      showcursor_state_symbol.Reset(Nan::New("showCursor").ToLocalChecked());
      lines_state_symbol.Reset(Nan::New("lines").ToLocalChecked());
      cols_state_symbol.Reset(Nan::New("cols").ToLocalChecked());
      tabsize_state_symbol.Reset(Nan::New("tabsize").ToLocalChecked());
      hasmouse_state_symbol.Reset(Nan::New("hasMouse").ToLocalChecked());
      hascolors_state_symbol.Reset(Nan::New("hasColors").ToLocalChecked());
      numcolors_state_symbol.Reset(Nan::New("numColors").ToLocalChecked());
      maxcolorpairs_state_symbol.Reset(Nan::New("maxColorPairs").ToLocalChecked());
      raw_state_symbol.Reset(Nan::New("raw").ToLocalChecked());
      bkgd_state_symbol.Reset(Nan::New("bkgd").ToLocalChecked());
      hidden_state_symbol.Reset(Nan::New("hidden").ToLocalChecked());
      height_state_symbol.Reset(Nan::New("height").ToLocalChecked());
      width_state_symbol.Reset(Nan::New("width").ToLocalChecked());
      begx_state_symbol.Reset(Nan::New("begx").ToLocalChecked());
      begy_state_symbol.Reset(Nan::New("begy").ToLocalChecked());
      curx_state_symbol.Reset(Nan::New("curx").ToLocalChecked());
      cury_state_symbol.Reset(Nan::New("cury").ToLocalChecked());
      maxx_state_symbol.Reset(Nan::New("maxx").ToLocalChecked());
      maxy_state_symbol.Reset(Nan::New("maxy").ToLocalChecked());
      wintouched_state_symbol.Reset(Nan::New("touched").ToLocalChecked());
      numwins_symbol.Reset(Nan::New("numwins").ToLocalChecked());
      ACS_symbol.Reset(Nan::New("ACS").ToLocalChecked());
      keys_symbol.Reset(Nan::New("keys").ToLocalChecked());
      colors_symbol.Reset(Nan::New("colors").ToLocalChecked());
      attrs_symbol.Reset(Nan::New("attrs").ToLocalChecked());

      /* Panel-specific methods */
      // TODO: color_set?, overlay, overwrite
      Nan::SetPrototypeMethod(tpl, "clearok", Clearok);
      Nan::SetPrototypeMethod(tpl, "scrollok", Scrollok);
      Nan::SetPrototypeMethod(tpl, "idlok", Idlok);
      Nan::SetPrototypeMethod(tpl, "idcok", Idcok);
      Nan::SetPrototypeMethod(tpl, "leaveok", Leaveok);
      Nan::SetPrototypeMethod(tpl, "syncok", Syncok);
      Nan::SetPrototypeMethod(tpl, "immedok", Immedok);
      Nan::SetPrototypeMethod(tpl, "keypad", Keypad);
      Nan::SetPrototypeMethod(tpl, "meta", Meta);
      Nan::SetPrototypeMethod(tpl, "standout", Standout);
      Nan::SetPrototypeMethod(tpl, "hide", Hide);
      Nan::SetPrototypeMethod(tpl, "show", Show);
      Nan::SetPrototypeMethod(tpl, "top", Top);
      Nan::SetPrototypeMethod(tpl, "bottom", Bottom);
      Nan::SetPrototypeMethod(tpl, "move", Mvwin);
      Nan::SetPrototypeMethod(tpl, "refresh", Refresh);
      Nan::SetPrototypeMethod(tpl, "noutrefresh", Noutrefresh);
      Nan::SetPrototypeMethod(tpl, "frame", Frame);
      Nan::SetPrototypeMethod(tpl, "boldframe", Boldframe);
      Nan::SetPrototypeMethod(tpl, "label", Label);
      Nan::SetPrototypeMethod(tpl, "centertext", Centertext);
      Nan::SetPrototypeMethod(tpl, "cursor", Move);
      Nan::SetPrototypeMethod(tpl, "insertln", Insertln);
      Nan::SetPrototypeMethod(tpl, "insdelln", Insdelln);
      Nan::SetPrototypeMethod(tpl, "insstr", Insstr);
      Nan::SetPrototypeMethod(tpl, "attron", Attron);
      Nan::SetPrototypeMethod(tpl, "attroff", Attroff);
      Nan::SetPrototypeMethod(tpl, "attrset", Attrset);
      Nan::SetPrototypeMethod(tpl, "attrget", Attrget);
      Nan::SetPrototypeMethod(tpl, "box", Box);
      Nan::SetPrototypeMethod(tpl, "border", Border);
      Nan::SetPrototypeMethod(tpl, "hline", Hline);
      Nan::SetPrototypeMethod(tpl, "vline", Vline);
      Nan::SetPrototypeMethod(tpl, "erase", Erase);
      Nan::SetPrototypeMethod(tpl, "clear", Clear);
      Nan::SetPrototypeMethod(tpl, "clrtobot", Clrtobot);
      Nan::SetPrototypeMethod(tpl, "clrtoeol", Clrtoeol);
      Nan::SetPrototypeMethod(tpl, "delch", Delch);
      Nan::SetPrototypeMethod(tpl, "deleteln", Deleteln);
      Nan::SetPrototypeMethod(tpl, "scroll", Scroll);
      Nan::SetPrototypeMethod(tpl, "setscrreg", Setscrreg);
      Nan::SetPrototypeMethod(tpl, "touchlines", Touchln);
      Nan::SetPrototypeMethod(tpl, "is_linetouched", Is_linetouched);
      Nan::SetPrototypeMethod(tpl, "redrawln", Redrawln);
      Nan::SetPrototypeMethod(tpl, "touch", Touchwin);
      Nan::SetPrototypeMethod(tpl, "untouch", Untouchwin);
      Nan::SetPrototypeMethod(tpl, "resize", Wresize);
      Nan::SetPrototypeMethod(tpl, "print", Print);
      Nan::SetPrototypeMethod(tpl, "addstr", Addstr);
      Nan::SetPrototypeMethod(tpl, "close", Close);
      Nan::SetPrototypeMethod(tpl, "syncdown", Syncdown);
      Nan::SetPrototypeMethod(tpl, "syncup", Syncup);
      Nan::SetPrototypeMethod(tpl, "cursyncup", Cursyncup);
      Nan::SetPrototypeMethod(tpl, "copywin", Copywin);
      Nan::SetPrototypeMethod(tpl, "redraw", Redrawwin);

      /* Attribute-related window functions */
      Nan::SetPrototypeMethod(tpl, "addch", Addch);
      Nan::SetPrototypeMethod(tpl, "echochar", Echochar);
      Nan::SetPrototypeMethod(tpl, "inch", Inch);
      Nan::SetPrototypeMethod(tpl, "insch", Insch);
      Nan::SetPrototypeMethod(tpl, "chgat", Chgat);
      //Nan::SetPrototypeMethod(tpl, "addchstr", Addchstr);
      //Nan::SetPrototypeMethod(tpl, "inchstr", Inchstr);

      /* Window properties */
      Nan::SetAccessor(tpl->PrototypeTemplate(), Nan::New(bkgd_state_symbol),
                                                           BkgdStateGetter,
                                                           BkgdStateSetter);
      Nan::SetAccessor(tpl->PrototypeTemplate(), Nan::New(hidden_state_symbol),
                                                           HiddenStateGetter);
      Nan::SetAccessor(tpl->PrototypeTemplate(), Nan::New(height_state_symbol),
                                                           HeightStateGetter);
      Nan::SetAccessor(tpl->PrototypeTemplate(), Nan::New(width_state_symbol),
                                                           WidthStateGetter);
      Nan::SetAccessor(tpl->PrototypeTemplate(), Nan::New(begx_state_symbol),
                                                           BegxStateGetter);
      Nan::SetAccessor(tpl->PrototypeTemplate(), Nan::New(begy_state_symbol),
                                                           BegyStateGetter);
      Nan::SetAccessor(tpl->PrototypeTemplate(), Nan::New(curx_state_symbol),
                                                           CurxStateGetter);
      Nan::SetAccessor(tpl->PrototypeTemplate(), Nan::New(cury_state_symbol),
                                                           CuryStateGetter);
      Nan::SetAccessor(tpl->PrototypeTemplate(), Nan::New(maxx_state_symbol),
                                                           MaxxStateGetter);
      Nan::SetAccessor(tpl->PrototypeTemplate(), Nan::New(maxy_state_symbol),
                                                           MaxyStateGetter);
      Nan::SetAccessor(tpl->PrototypeTemplate(), Nan::New(wintouched_state_symbol),
                                                           WintouchedStateGetter);

      /* Global/Terminal properties and functions */
      Nan::SetAccessor(target, Nan::New(echo_state_symbol), EchoStateGetter, EchoStateSetter);
      Nan::SetAccessor(target, Nan::New(showcursor_state_symbol), ShowcursorStateGetter,
                          ShowcursorStateSetter);
      Nan::SetAccessor(target, Nan::New(lines_state_symbol), LinesStateGetter);
      Nan::SetAccessor(target, Nan::New(cols_state_symbol), ColsStateGetter);
      Nan::SetAccessor(target, Nan::New(tabsize_state_symbol), TabsizeStateGetter);
      Nan::SetAccessor(target, Nan::New(hasmouse_state_symbol), HasmouseStateGetter);
      Nan::SetAccessor(target, Nan::New(hascolors_state_symbol), HascolorsStateGetter);
      Nan::SetAccessor(target, Nan::New(numcolors_state_symbol), NumcolorsStateGetter);
      Nan::SetAccessor(target, Nan::New(maxcolorpairs_state_symbol), MaxcolorpairsStateGetter);
      Nan::SetAccessor(target, Nan::New(raw_state_symbol), RawStateGetter, RawStateSetter);
      Nan::SetAccessor(target, Nan::New(numwins_symbol), NumwinsGetter);
      Nan::SetAccessor(target, Nan::New(ACS_symbol), ACSConstsGetter);
      Nan::SetAccessor(target, Nan::New(keys_symbol), KeyConstsGetter);
      Nan::SetAccessor(target, Nan::New(colors_symbol), ColorConstsGetter);
      Nan::SetAccessor(target, Nan::New(attrs_symbol), AttrConstsGetter);
      Nan::SetMethod(target, "setEscDelay", Setescdelay);
      Nan::SetMethod(target, "cleanup", Resetscreen);
      Nan::SetMethod(target, "redraw", Redraw);
      Nan::SetMethod(target, "leave", LeaveNcurses);
      Nan::SetMethod(target, "restore", RestoreNcurses);
      Nan::SetMethod(target, "beep", Beep);
      Nan::SetMethod(target, "flash", Flash);
      Nan::SetMethod(target, "doupdate", DoUpdate);
      Nan::SetMethod(target, "colorPair", Colorpair);
      Nan::SetMethod(target, "colorFg", Colorfg);
      Nan::SetMethod(target, "colorBg", Colorbg);
      Nan::SetMethod(target, "dup2", Dup2);
      Nan::SetMethod(target, "dup", Dup);

      Nan::Set(target, name, Nan::GetFunction(tpl).ToLocalChecked());
    }

    void init(int nlines=-1, int ncols=-1, int begin_y=-1, int begin_x=-1) {
      static bool initialize_ACS = true;
      if (stdin_fd < 0) {
        stdin_fd = STDIN_FILENO;
        int stdin_flags = fcntl(stdin_fd, F_GETFL, 0);
        int r = fcntl(stdin_fd, F_SETFL, stdin_flags | O_NONBLOCK);
        if (r < 0) {
          Nan::ThrowError("Unable to set stdin to non-block");
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

        Local<Object> obj = Nan::New<Object>();
        Nan::Set(obj, Nan::New("ULCORNER").ToLocalChecked(), Nan::New<Uint32>(ACS_ULCORNER));
        Nan::Set(obj, Nan::New("LLCORNER").ToLocalChecked(), Nan::New<Uint32>(ACS_LLCORNER));
        Nan::Set(obj, Nan::New("URCORNER").ToLocalChecked(), Nan::New<Uint32>(ACS_URCORNER));
        Nan::Set(obj, Nan::New("LRCORNER").ToLocalChecked(), Nan::New<Uint32>(ACS_LRCORNER));
        Nan::Set(obj, Nan::New("LTEE").ToLocalChecked(), Nan::New<Uint32>(ACS_LTEE));
        Nan::Set(obj, Nan::New("RTEE").ToLocalChecked(), Nan::New<Uint32>(ACS_RTEE));
        Nan::Set(obj, Nan::New("BTEE").ToLocalChecked(), Nan::New<Uint32>(ACS_BTEE));
        Nan::Set(obj, Nan::New("TTEE").ToLocalChecked(), Nan::New<Uint32>(ACS_TTEE));
        Nan::Set(obj, Nan::New("HLINE").ToLocalChecked(), Nan::New<Uint32>(ACS_HLINE));
        Nan::Set(obj, Nan::New("VLINE").ToLocalChecked(), Nan::New<Uint32>(ACS_VLINE));
        Nan::Set(obj, Nan::New("PLUS").ToLocalChecked(), Nan::New<Uint32>(ACS_PLUS));
        Nan::Set(obj, Nan::New("S1").ToLocalChecked(), Nan::New<Uint32>(ACS_S1));
        Nan::Set(obj, Nan::New("S9").ToLocalChecked(), Nan::New<Uint32>(ACS_S9));
        Nan::Set(obj, Nan::New("DIAMOND").ToLocalChecked(), Nan::New<Uint32>(ACS_DIAMOND));
        Nan::Set(obj, Nan::New("CKBOARD").ToLocalChecked(), Nan::New<Uint32>(ACS_CKBOARD));
        Nan::Set(obj, Nan::New("DEGREE").ToLocalChecked(), Nan::New<Uint32>(ACS_DEGREE));
        Nan::Set(obj, Nan::New("PLMINUS").ToLocalChecked(), Nan::New<Uint32>(ACS_PLMINUS));
        Nan::Set(obj, Nan::New("BULLET").ToLocalChecked(), Nan::New<Uint32>(ACS_BULLET));
        Nan::Set(obj, Nan::New("LARROW").ToLocalChecked(), Nan::New<Uint32>(ACS_LARROW));
        Nan::Set(obj, Nan::New("RARROW").ToLocalChecked(), Nan::New<Uint32>(ACS_RARROW));
        Nan::Set(obj, Nan::New("DARROW").ToLocalChecked(), Nan::New<Uint32>(ACS_DARROW));
        Nan::Set(obj, Nan::New("UARROW").ToLocalChecked(), Nan::New<Uint32>(ACS_UARROW));
        Nan::Set(obj, Nan::New("BOARD").ToLocalChecked(), Nan::New<Uint32>(ACS_BOARD));
        Nan::Set(obj, Nan::New("LANTERN").ToLocalChecked(), Nan::New<Uint32>(ACS_LANTERN));
        Nan::Set(obj, Nan::New("BLOCK").ToLocalChecked(), Nan::New<Uint32>(ACS_BLOCK));
        ACS_Chars.Reset(obj);

        obj = Nan::New<Object>();
        Nan::Set(obj, Nan::New("SPACE").ToLocalChecked(), Nan::New<Uint32>(32));
        Nan::Set(obj, Nan::New("NEWLINE").ToLocalChecked(), Nan::New<Uint32>(10));
        Nan::Set(obj, Nan::New("ESC").ToLocalChecked(), Nan::New<Uint32>(27));
        Nan::Set(obj, Nan::New("UP").ToLocalChecked(), Nan::New<Uint32>(KEY_UP));
        Nan::Set(obj, Nan::New("DOWN").ToLocalChecked(), Nan::New<Uint32>(KEY_DOWN));
        Nan::Set(obj, Nan::New("LEFT").ToLocalChecked(), Nan::New<Uint32>(KEY_LEFT));
        Nan::Set(obj, Nan::New("RIGHT").ToLocalChecked(), Nan::New<Uint32>(KEY_RIGHT));
        Nan::Set(obj, Nan::New("HOME").ToLocalChecked(), Nan::New<Uint32>(KEY_HOME));
        Nan::Set(obj, Nan::New("BACKSPACE").ToLocalChecked(), Nan::New<Uint32>(KEY_BACKSPACE));
        Nan::Set(obj, Nan::New("BREAK").ToLocalChecked(), Nan::New<Uint32>(KEY_BREAK));
        Nan::Set(obj, Nan::New("F0").ToLocalChecked(), Nan::New<Uint32>(KEY_F(0)));
        Nan::Set(obj, Nan::New("F1").ToLocalChecked(), Nan::New<Uint32>(KEY_F(1)));
        Nan::Set(obj, Nan::New("F2").ToLocalChecked(), Nan::New<Uint32>(KEY_F(2)));
        Nan::Set(obj, Nan::New("F3").ToLocalChecked(), Nan::New<Uint32>(KEY_F(3)));
        Nan::Set(obj, Nan::New("F4").ToLocalChecked(), Nan::New<Uint32>(KEY_F(4)));
        Nan::Set(obj, Nan::New("F5").ToLocalChecked(), Nan::New<Uint32>(KEY_F(5)));
        Nan::Set(obj, Nan::New("F6").ToLocalChecked(), Nan::New<Uint32>(KEY_F(6)));
        Nan::Set(obj, Nan::New("F7").ToLocalChecked(), Nan::New<Uint32>(KEY_F(7)));
        Nan::Set(obj, Nan::New("F8").ToLocalChecked(), Nan::New<Uint32>(KEY_F(8)));
        Nan::Set(obj, Nan::New("F9").ToLocalChecked(), Nan::New<Uint32>(KEY_F(9)));
        Nan::Set(obj, Nan::New("F10").ToLocalChecked(), Nan::New<Uint32>(KEY_F(10)));
        Nan::Set(obj, Nan::New("F11").ToLocalChecked(), Nan::New<Uint32>(KEY_F(11)));
        Nan::Set(obj, Nan::New("F12").ToLocalChecked(), Nan::New<Uint32>(KEY_F(12)));
        Nan::Set(obj, Nan::New("DL").ToLocalChecked(), Nan::New<Uint32>(KEY_DL));
        Nan::Set(obj, Nan::New("IL").ToLocalChecked(), Nan::New<Uint32>(KEY_IL));
        Nan::Set(obj, Nan::New("DEL").ToLocalChecked(), Nan::New<Uint32>(KEY_DC));
        Nan::Set(obj, Nan::New("INS").ToLocalChecked(), Nan::New<Uint32>(KEY_IC));
        Nan::Set(obj, Nan::New("EIC").ToLocalChecked(), Nan::New<Uint32>(KEY_EIC));
        Nan::Set(obj, Nan::New("CLEAR").ToLocalChecked(), Nan::New<Uint32>(KEY_CLEAR));
        Nan::Set(obj, Nan::New("EOS").ToLocalChecked(), Nan::New<Uint32>(KEY_EOS));
        Nan::Set(obj, Nan::New("EOL").ToLocalChecked(), Nan::New<Uint32>(KEY_EOL));
        Nan::Set(obj, Nan::New("SF").ToLocalChecked(), Nan::New<Uint32>(KEY_SF));
        Nan::Set(obj, Nan::New("SR").ToLocalChecked(), Nan::New<Uint32>(KEY_SR));
        Nan::Set(obj, Nan::New("NPAGE").ToLocalChecked(), Nan::New<Uint32>(KEY_NPAGE));
        Nan::Set(obj, Nan::New("PPAGE").ToLocalChecked(), Nan::New<Uint32>(KEY_PPAGE));
        Nan::Set(obj, Nan::New("STAB").ToLocalChecked(), Nan::New<Uint32>(KEY_STAB));
        Nan::Set(obj, Nan::New("CTAB").ToLocalChecked(), Nan::New<Uint32>(KEY_CTAB));
        Nan::Set(obj, Nan::New("CATAB").ToLocalChecked(), Nan::New<Uint32>(KEY_CATAB));
        Nan::Set(obj, Nan::New("ENTER").ToLocalChecked(), Nan::New<Uint32>(KEY_ENTER));
        Nan::Set(obj, Nan::New("SRESET").ToLocalChecked(), Nan::New<Uint32>(KEY_SRESET));
        Nan::Set(obj, Nan::New("RESET").ToLocalChecked(), Nan::New<Uint32>(KEY_RESET));
        Nan::Set(obj, Nan::New("PRINT").ToLocalChecked(), Nan::New<Uint32>(KEY_PRINT));
        Nan::Set(obj, Nan::New("LL").ToLocalChecked(), Nan::New<Uint32>(KEY_LL));
        Nan::Set(obj, Nan::New("UPLEFT").ToLocalChecked(), Nan::New<Uint32>(KEY_A1));
        Nan::Set(obj, Nan::New("UPRIGHT").ToLocalChecked(), Nan::New<Uint32>(KEY_A3));
        Nan::Set(obj, Nan::New("CENTER").ToLocalChecked(), Nan::New<Uint32>(KEY_B2));
        Nan::Set(obj, Nan::New("DOWNLEFT").ToLocalChecked(), Nan::New<Uint32>(KEY_C1));
        Nan::Set(obj, Nan::New("DOWNRIGHT").ToLocalChecked(), Nan::New<Uint32>(KEY_C3));
        Nan::Set(obj, Nan::New("BTAB").ToLocalChecked(), Nan::New<Uint32>(KEY_BTAB));
        Nan::Set(obj, Nan::New("BEG").ToLocalChecked(), Nan::New<Uint32>(KEY_BEG));
        Nan::Set(obj, Nan::New("CANCEL").ToLocalChecked(), Nan::New<Uint32>(KEY_CANCEL));
        Nan::Set(obj, Nan::New("CLOSE").ToLocalChecked(), Nan::New<Uint32>(KEY_CLOSE));
        Nan::Set(obj, Nan::New("COMMAND").ToLocalChecked(), Nan::New<Uint32>(KEY_COMMAND));
        Nan::Set(obj, Nan::New("COPY").ToLocalChecked(), Nan::New<Uint32>(KEY_COPY));
        Nan::Set(obj, Nan::New("CREATE").ToLocalChecked(), Nan::New<Uint32>(KEY_CREATE));
        Nan::Set(obj, Nan::New("END").ToLocalChecked(), Nan::New<Uint32>(KEY_END));
        Nan::Set(obj, Nan::New("EXIT").ToLocalChecked(), Nan::New<Uint32>(KEY_EXIT));
        Nan::Set(obj, Nan::New("FIND").ToLocalChecked(), Nan::New<Uint32>(KEY_FIND));
        Nan::Set(obj, Nan::New("FIND").ToLocalChecked(), Nan::New<Uint32>(KEY_HELP));
        Nan::Set(obj, Nan::New("MARK").ToLocalChecked(), Nan::New<Uint32>(KEY_MARK));
        Nan::Set(obj, Nan::New("MESSAGE").ToLocalChecked(), Nan::New<Uint32>(KEY_MESSAGE));
        Nan::Set(obj, Nan::New("MOVE").ToLocalChecked(), Nan::New<Uint32>(KEY_MOVE));
        Nan::Set(obj, Nan::New("NEXT").ToLocalChecked(), Nan::New<Uint32>(KEY_NEXT));
        Nan::Set(obj, Nan::New("OPEN").ToLocalChecked(), Nan::New<Uint32>(KEY_OPEN));
        Nan::Set(obj, Nan::New("OPTIONS").ToLocalChecked(), Nan::New<Uint32>(KEY_OPTIONS));
        Nan::Set(obj, Nan::New("PREVIOUS").ToLocalChecked(), Nan::New<Uint32>(KEY_PREVIOUS));
        Nan::Set(obj, Nan::New("REDO").ToLocalChecked(), Nan::New<Uint32>(KEY_REDO));
        Nan::Set(obj, Nan::New("REFERENCE").ToLocalChecked(), Nan::New<Uint32>(KEY_REFERENCE));
        Nan::Set(obj, Nan::New("REFRESH").ToLocalChecked(), Nan::New<Uint32>(KEY_REFRESH));
        Nan::Set(obj, Nan::New("REPLACE").ToLocalChecked(), Nan::New<Uint32>(KEY_REPLACE));
        Nan::Set(obj, Nan::New("RESTART").ToLocalChecked(), Nan::New<Uint32>(KEY_RESTART));
        Nan::Set(obj, Nan::New("RESUME").ToLocalChecked(), Nan::New<Uint32>(KEY_RESUME));
        Nan::Set(obj, Nan::New("SAVE").ToLocalChecked(), Nan::New<Uint32>(KEY_SAVE));
        Nan::Set(obj, Nan::New("S_BEG").ToLocalChecked(), Nan::New<Uint32>(KEY_SBEG));
        Nan::Set(obj, Nan::New("S_CANCEL").ToLocalChecked(), Nan::New<Uint32>(KEY_SCANCEL));
        Nan::Set(obj, Nan::New("S_COMMAND").ToLocalChecked(), Nan::New<Uint32>(KEY_SCOMMAND));
        Nan::Set(obj, Nan::New("S_COPY").ToLocalChecked(), Nan::New<Uint32>(KEY_SCOPY));
        Nan::Set(obj, Nan::New("S_CREATE").ToLocalChecked(), Nan::New<Uint32>(KEY_SCREATE));
        Nan::Set(obj, Nan::New("S_DC").ToLocalChecked(), Nan::New<Uint32>(KEY_SDC));
        Nan::Set(obj, Nan::New("S_DL").ToLocalChecked(), Nan::New<Uint32>(KEY_SDL));
        Nan::Set(obj, Nan::New("SELECT").ToLocalChecked(), Nan::New<Uint32>(KEY_SELECT));
        Nan::Set(obj, Nan::New("SEND").ToLocalChecked(), Nan::New<Uint32>(KEY_SEND));
        Nan::Set(obj, Nan::New("S_EOL").ToLocalChecked(), Nan::New<Uint32>(KEY_SEOL));
        Nan::Set(obj, Nan::New("S_EXIT").ToLocalChecked(), Nan::New<Uint32>(KEY_SEXIT));
        Nan::Set(obj, Nan::New("S_FIND").ToLocalChecked(), Nan::New<Uint32>(KEY_SFIND));
        Nan::Set(obj, Nan::New("S_HELP").ToLocalChecked(), Nan::New<Uint32>(KEY_SHELP));
        Nan::Set(obj, Nan::New("S_HOME").ToLocalChecked(), Nan::New<Uint32>(KEY_SHOME));
        Nan::Set(obj, Nan::New("S_IC").ToLocalChecked(), Nan::New<Uint32>(KEY_SIC));
        Nan::Set(obj, Nan::New("S_LEFT").ToLocalChecked(), Nan::New<Uint32>(KEY_SLEFT));
        Nan::Set(obj, Nan::New("S_MESSAGE").ToLocalChecked(), Nan::New<Uint32>(KEY_SMESSAGE));
        Nan::Set(obj, Nan::New("S_MOVE").ToLocalChecked(), Nan::New<Uint32>(KEY_SMOVE));
        Nan::Set(obj, Nan::New("S_NEXT").ToLocalChecked(), Nan::New<Uint32>(KEY_SNEXT));
        Nan::Set(obj, Nan::New("S_OPTIONS").ToLocalChecked(), Nan::New<Uint32>(KEY_SOPTIONS));
        Nan::Set(obj, Nan::New("S_PREVIOUS").ToLocalChecked(), Nan::New<Uint32>(KEY_SPREVIOUS));
        Nan::Set(obj, Nan::New("S_PRINT").ToLocalChecked(), Nan::New<Uint32>(KEY_SPRINT));
        Nan::Set(obj, Nan::New("S_REDO").ToLocalChecked(), Nan::New<Uint32>(KEY_SREDO));
        Nan::Set(obj, Nan::New("S_REPLACE").ToLocalChecked(), Nan::New<Uint32>(KEY_SREPLACE));
        Nan::Set(obj, Nan::New("S_RIGHT").ToLocalChecked(), Nan::New<Uint32>(KEY_SRIGHT));
        Nan::Set(obj, Nan::New("S_RESUME").ToLocalChecked(), Nan::New<Uint32>(KEY_SRSUME));
        Nan::Set(obj, Nan::New("S_SAVE").ToLocalChecked(), Nan::New<Uint32>(KEY_SSAVE));
        Nan::Set(obj, Nan::New("S_SUSPEND").ToLocalChecked(), Nan::New<Uint32>(KEY_SSUSPEND));
        Nan::Set(obj, Nan::New("S_UNDO").ToLocalChecked(), Nan::New<Uint32>(KEY_SUNDO));
        Nan::Set(obj, Nan::New("SUSPEND").ToLocalChecked(), Nan::New<Uint32>(KEY_SUSPEND));
        Nan::Set(obj, Nan::New("UNDO").ToLocalChecked(), Nan::New<Uint32>(KEY_UNDO));
        Nan::Set(obj, Nan::New("MOUSE").ToLocalChecked(), Nan::New<Uint32>(KEY_MOUSE));
        Nan::Set(obj, Nan::New("RESIZE").ToLocalChecked(), Nan::New<Uint32>(KEY_RESIZE));
        Keys.Reset(obj);

        obj = Nan::New<Object>();
        Nan::Set(obj, Nan::New("BLACK").ToLocalChecked(), Nan::New<Uint32>(COLOR_BLACK));
        Nan::Set(obj, Nan::New("RED").ToLocalChecked(), Nan::New<Uint32>(COLOR_RED));
        Nan::Set(obj, Nan::New("GREEN").ToLocalChecked(), Nan::New<Uint32>(COLOR_GREEN));
        Nan::Set(obj, Nan::New("YELLOW").ToLocalChecked(), Nan::New<Uint32>(COLOR_YELLOW));
        Nan::Set(obj, Nan::New("BLUE").ToLocalChecked(), Nan::New<Uint32>(COLOR_BLUE));
        Nan::Set(obj, Nan::New("MAGENTA").ToLocalChecked(), Nan::New<Uint32>(COLOR_MAGENTA));
        Nan::Set(obj, Nan::New("CYAN").ToLocalChecked(), Nan::New<Uint32>(COLOR_CYAN));
        Nan::Set(obj, Nan::New("WHITE").ToLocalChecked(), Nan::New<Uint32>(COLOR_WHITE));
        Colors.Reset(obj);

        obj = Nan::New<Object>();
        Nan::Set(obj, Nan::New("NORMAL").ToLocalChecked(), Nan::New<Uint32>(A_NORMAL));
        Nan::Set(obj, Nan::New("STANDOUT").ToLocalChecked(), Nan::New<Uint32>(A_STANDOUT));
        Nan::Set(obj, Nan::New("UNDERLINE").ToLocalChecked(), Nan::New<Uint32>(A_UNDERLINE));
        Nan::Set(obj, Nan::New("REVERSE").ToLocalChecked(), Nan::New<Uint32>(A_REVERSE));
        Nan::Set(obj, Nan::New("BLINK").ToLocalChecked(), Nan::New<Uint32>(A_BLINK));
        Nan::Set(obj, Nan::New("DIM").ToLocalChecked(), Nan::New<Uint32>(A_DIM));
        Nan::Set(obj, Nan::New("BOLD").ToLocalChecked(), Nan::New<Uint32>(A_BOLD));
        Nan::Set(obj, Nan::New("INVISIBLE").ToLocalChecked(), Nan::New<Uint32>(A_INVIS));
        Nan::Set(obj, Nan::New("PROTECT").ToLocalChecked(), Nan::New<Uint32>(A_PROTECT));
        Attrs.Reset(obj);
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
      Window *win;

      if (stdin_fd < 0)
        win = new Window();
      else if (info.Length() == 2 && info[0]->IsInt32() && info[1]->IsInt32())
        win = new Window(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(), 0, 0);
      else if (info.Length() == 3 && info[0]->IsInt32() && info[1]->IsInt32()
               && info[2]->IsInt32()) {
        win = new Window(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                         Nan::To<int32_t>(info[2]).FromJust(), 0);
      } else if (info.Length() == 4 && info[0]->IsInt32() && info[1]->IsInt32()
                 && info[2]->IsInt32() && info[3]->IsInt32()) {
        win = new Window(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                         Nan::To<int32_t>(info[2]).FromJust(), Nan::To<int32_t>(info[3]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      win->Wrap(info.This());
      win->Ref();

      win->Emit.Reset(Nan::Get(win->handle(), Nan::New(emit_symbol)).ToLocalChecked().As<Function>());

      info.GetReturnValue().Set(info.This());
    }

    static NAN_METHOD(Close) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      if (win->panel() != NULL)
        win->close();
    }

    static NAN_METHOD(Hide) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      win->panel()->hide();
    }

    static NAN_METHOD(Show) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      win->panel()->show();
    }

    static NAN_METHOD(Top) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      win->panel()->top();
    }

    static NAN_METHOD(Bottom) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      win->panel()->bottom();
    }

    static NAN_METHOD(Mvwin) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret = 0;
      if (info.Length() == 2 && info[0]->IsInt32() && info[1]->IsInt32())
        ret = win->panel()->mvwin(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Refresh) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      int ret = win->panel()->refresh();

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Noutrefresh) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret = win->panel()->noutrefresh();

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Redraw) {
      MyPanel::redraw();
    }

    static NAN_METHOD(Frame) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      if (info.Length() == 0) {
        win->panel()->frame(NULL, NULL);
      } else if (info.Length() == 1 && info[0]->IsString()) {
        Nan::Utf8String str(info[0].As<String>());
        win->panel()->frame(ToCString(str), NULL);
      } else if (info.Length() == 2 && info[0]->IsString()
                 && info[1]->IsString()) {
        Nan::Utf8String str0(info[0].As<String>());
        Nan::Utf8String str1(info[1].As<String>());
        win->panel()->frame(ToCString(str0), ToCString(str1));
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }
    }

    static NAN_METHOD(Boldframe) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      if (info.Length() == 0) {
        win->panel()->boldframe(NULL, NULL);
      } else if (info.Length() == 1 && info[0]->IsString()) {
        Nan::Utf8String str(info[0].As<String>());
        win->panel()->boldframe(ToCString(str), NULL);
      } else if (info.Length() == 2 && info[0]->IsString()
                 && info[1]->IsString()) {
        Nan::Utf8String str0(info[0].As<String>());
        Nan::Utf8String str1(info[1].As<String>());
        win->panel()->boldframe(ToCString(str0), ToCString(str1));
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }
    }

    static NAN_METHOD(Label) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      if (info.Length() == 1 && info[0]->IsString()) {
        Nan::Utf8String str(info[0].As<String>());
        win->panel()->label(ToCString(str), NULL);
      } else if (info.Length() == 2 && info[0]->IsString()
                 && info[1]->IsString()) {
        Nan::Utf8String str0(info[0].As<String>());
        Nan::Utf8String str1(info[1].As<String>());
        win->panel()->label(ToCString(str0), ToCString(str1));
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }
    }

    static NAN_METHOD(Centertext) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      if (info.Length() == 2 && info[0]->IsInt32() && info[1]->IsString()) {
        Nan::Utf8String str(info[1].As<String>());
        win->panel()->centertext(Nan::To<int32_t>(info[0]).FromJust(), ToCString(str));
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }
    }

    static NAN_METHOD(Move) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 2 && info[0]->IsInt32() && info[1]->IsInt32())
        ret = win->panel()->move(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Addch) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsUint32())
        ret = win->panel()->addch(Nan::To<uint32_t>(info[0]).FromJust());
      else if (info.Length() == 3 && info[0]->IsInt32() && info[1]->IsInt32()
               && info[2]->IsUint32()) {
        ret = win->panel()->addch(Nan::To<uint32_t>(info[0]).FromJust(), Nan::To<uint32_t>(info[1]).FromJust(),
                                  Nan::To<uint32_t>(info[2]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Echochar) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsUint32())
        ret = win->panel()->echochar(Nan::To<uint32_t>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Addstr) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsString()) {
        Nan::Utf8String str(info[0].As<String>());
        ret = win->panel()->addstr(ToCString(str), -1);
      } else if (info.Length() == 2 && info[0]->IsString()
                 && info[1]->IsInt32()) {
        Nan::Utf8String str(info[0].As<String>());
        ret = win->panel()->addstr(ToCString(str), Nan::To<int32_t>(info[1]).FromJust());
      } else if (info.Length() == 3 && info[0]->IsInt32() && info[1]->IsInt32()
                 && info[2]->IsString()) {
        Nan::Utf8String str(info[2].As<String>());
        ret = win->panel()->addstr(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                                   ToCString(str), -1);
      } else if (info.Length() == 4 && info[0]->IsInt32() && info[1]->IsInt32()
                 && info[2]->IsString() && info[3]->IsInt32()) {
        Nan::Utf8String str(info[2].As<String>());
        ret = win->panel()->addstr(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                                   ToCString(str), Nan::To<int32_t>(info[3]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    // FIXME: addchstr requires a pointer to a chtype not an actual value,
    //        unlike the other ACS_*-using methods
    /*static NAN_METHOD(Addchstr) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsUint32())
        ret = win->panel()->addchstr(Nan::To<uint32_t>(info[0]).FromJust(), -1);
      else if (info.Length() == 2 && info[0]->IsUint32() && info[1]->IsInt32()) {
        ret = win->panel()->addchstr(Nan::To<uint32_t>(info[0]).FromJust(),
                                     Nan::To<int32_t>(info[1]).FromJust());
      } else if (info.Length() == 3 && info[0]->IsInt32() && info[1]->IsInt32()
                 && info[2]->IsUint32()) {
        ret = win->panel()->addchstr(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                                     Nan::To<uint32_t>(info[2]).FromJust(), -1);
      } else if (info.Length() == 4 && info[0]->IsInt32() && info[1]->IsInt32()
                 && info[2]->IsUint32() && info[3]->IsInt32()) {
        ret = win->panel()->addchstr(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                                     Nan::To<uint32_t>(info[2]).FromJust(),
                                     Nan::To<int32_t>(info[3]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }*/

    static NAN_METHOD(Inch) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      unsigned int ret;
      if (info.Length() == 0)
        ret = win->panel()->inch();
      else if (info.Length() == 2 && info[0]->IsInt32() && info[1]->IsInt32())
        ret = win->panel()->inch(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    // FIXME: Need pointer to chtype instead of actual value
    /*static NAN_METHOD(Inchstr) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsUint32())
        ret = win->panel()->inchstr(Nan::To<uint32_t>(info[0]).FromJust(), -1);
      else if (info.Length() == 2 && info[0]->IsUint32() && info[1]->IsInt32()) {
        ret = win->panel()->inchstr(Nan::To<uint32_t>(info[0]).FromJust(),
                                    Nan::To<int32_t>(info[1]).FromJust());
      } else if (info.Length() == 3 && info[0]->IsInt32() && info[1]->IsInt32()
                 && info[2]->IsUint32()) {
        ret = win->panel()->inchstr(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                                    Nan::To<uint32_t>(info[2]).FromJust(), -1);
      } else if (info.Length() == 4 && info[0]->IsInt32() && info[1]->IsInt32()
                 && info[2]->IsUint32() && info[3]->IsInt32()) {
        ret = win->panel()->inchstr(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                                    Nan::To<uint32_t>(info[2]).FromJust(),
                                    Nan::To<int32_t>(info[3]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }*/

    static NAN_METHOD(Insch) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsUint32())
        ret = win->panel()->insch(Nan::To<uint32_t>(info[0]).FromJust());
      else if (info.Length() == 3 && info[0]->IsInt32() && info[1]->IsInt32()
               && info[2]->IsUint32()) {
        ret = win->panel()->insch(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                                  Nan::To<uint32_t>(info[2]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Chgat) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 2 && info[0]->IsInt32() && info[1]->IsUint32()) {
        ret = win->panel()->chgat(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<uint32_t>(info[1]).FromJust(),
                                  win->panel()->getcolor());
      } else if (info.Length() == 3 && info[0]->IsInt32() && info[1]->IsUint32()
                 && info[2]->IsUint32()) {
        ret = win->panel()->chgat(Nan::To<int32_t>(info[0]).FromJust(),
                                  (attr_t)(Nan::To<uint32_t>(info[1]).FromJust()),
                                  Nan::To<uint32_t>(info[2]).FromJust());
      } else if (info.Length() == 4 && info[0]->IsUint32() && info[1]->IsUint32()
                 && info[2]->IsInt32() && info[3]->IsUint32()) {
        ret = win->panel()->chgat(Nan::To<uint32_t>(info[0]).FromJust(), Nan::To<uint32_t>(info[1]).FromJust(),
                                  Nan::To<int32_t>(info[2]).FromJust(),
                                  (attr_t)(Nan::To<uint32_t>(info[3]).FromJust()),
                                  win->panel()->getcolor());
      } else if (info.Length() == 5 && info[0]->IsUint32() && info[1]->IsUint32()
                 && info[2]->IsInt32() && info[3]->IsUint32()
                 && info[4]->IsUint32()) {
        ret = win->panel()->chgat(Nan::To<uint32_t>(info[0]).FromJust(), Nan::To<uint32_t>(info[1]).FromJust(),
                                  Nan::To<int32_t>(info[2]).FromJust(),
                                  (attr_t)(Nan::To<uint32_t>(info[3]).FromJust()),
                                  Nan::To<uint32_t>(info[4]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Insertln) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret = win->panel()->insertln();

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Insdelln) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 0)
        ret = win->panel()->insdelln(1);
      else if (info.Length() == 1 && info[0]->IsInt32())
        ret = win->panel()->insdelln(Nan::To<int32_t>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Insstr) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsString()) {
        Nan::Utf8String str(info[0].As<String>());
        ret = win->panel()->insstr(ToCString(str), -1);
      } else if (info.Length() == 2 && info[0]->IsString()
                 && info[1]->IsInt32()) {
        Nan::Utf8String str(info[0].As<String>());
        ret = win->panel()->insstr(ToCString(str), Nan::To<int32_t>(info[1]).FromJust());
      } else if (info.Length() == 3 && info[0]->IsInt32() && info[1]->IsInt32()
                 && info[2]->IsString()) {
        Nan::Utf8String str(info[2].As<String>());
        ret = win->panel()->insstr(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                                   ToCString(str), -1);
      } else if (info.Length() == 4 && info[0]->IsInt32() && info[1]->IsInt32()
                 && info[2]->IsString() && info[3]->IsInt32()) {
        Nan::Utf8String str(info[2].As<String>());
        ret = win->panel()->insstr(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                                   ToCString(str), Nan::To<int32_t>(info[3]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Attron) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsUint32())
        ret = win->panel()->attron(Nan::To<uint32_t>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Attroff) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsUint32())
        ret = win->panel()->attroff(Nan::To<uint32_t>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Attrset) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsUint32())
        ret = win->panel()->attrset(Nan::To<uint32_t>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Attrget) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      unsigned int ret = win->panel()->attrget();

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Box) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 0)
        ret = win->panel()->box(0, 0);
      else if (info.Length() == 1 && info[0]->IsUint32())
        ret = win->panel()->box(Nan::To<uint32_t>(info[0]).FromJust(), 0);
      else if (info.Length() == 2 && info[0]->IsUint32() && info[1]->IsUint32())
        ret = win->panel()->box(Nan::To<uint32_t>(info[0]).FromJust(), Nan::To<uint32_t>(info[1]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Border) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 0)
        ret = win->panel()->border(0);
      else if (info.Length() == 1 && info[0]->IsUint32())
        ret = win->panel()->border(Nan::To<uint32_t>(info[0]).FromJust(), 0);
      else if (info.Length() == 2 && info[0]->IsUint32()
               && info[1]->IsUint32()) {
        ret = win->panel()->border(Nan::To<uint32_t>(info[0]).FromJust(),
                                   Nan::To<uint32_t>(info[1]).FromJust(), 0);
      } else if (info.Length() == 3 && info[0]->IsUint32() && info[1]->IsUint32()
                 && info[2]->IsUint32()) {
        ret = win->panel()->border(Nan::To<uint32_t>(info[0]).FromJust(), Nan::To<uint32_t>(info[1]).FromJust(),
                                   Nan::To<uint32_t>(info[2]).FromJust(), 0);
      } else if (info.Length() == 4 && info[0]->IsUint32() && info[1]->IsUint32()
                 && info[2]->IsUint32() && info[3]->IsUint32()) {
        ret = win->panel()->border(Nan::To<uint32_t>(info[0]).FromJust(), Nan::To<uint32_t>(info[1]).FromJust(),
                                   Nan::To<uint32_t>(info[2]).FromJust(),
                                   Nan::To<uint32_t>(info[3]).FromJust(), 0);
      } else if (info.Length() == 5 && info[0]->IsUint32() && info[1]->IsUint32()
                 && info[2]->IsUint32() && info[3]->IsUint32()
                 && info[4]->IsUint32()) {
        ret = win->panel()->border(Nan::To<uint32_t>(info[0]).FromJust(), Nan::To<uint32_t>(info[1]).FromJust(),
                                   Nan::To<uint32_t>(info[2]).FromJust(), Nan::To<uint32_t>(info[3]).FromJust(),
                                   Nan::To<uint32_t>(info[4]).FromJust(), 0);
      } else if (info.Length() == 6 && info[0]->IsUint32() && info[1]->IsUint32()
                 && info[2]->IsUint32() && info[3]->IsUint32()
                 && info[4]->IsUint32() && info[5]->IsUint32()) {
        ret = win->panel()->border(Nan::To<uint32_t>(info[0]).FromJust(), Nan::To<uint32_t>(info[1]).FromJust(),
                                   Nan::To<uint32_t>(info[2]).FromJust(), Nan::To<uint32_t>(info[3]).FromJust(),
                                   Nan::To<uint32_t>(info[4]).FromJust(),
                                   Nan::To<uint32_t>(info[5]).FromJust(), 0);
      } else if (info.Length() == 7 && info[0]->IsUint32() && info[1]->IsUint32()
                 && info[2]->IsUint32() && info[3]->IsUint32()
                 && info[4]->IsUint32() && info[5]->IsUint32()
                 && info[6]->IsUint32()) {
        ret = win->panel()->border(Nan::To<uint32_t>(info[0]).FromJust(), Nan::To<uint32_t>(info[1]).FromJust(),
                                   Nan::To<uint32_t>(info[2]).FromJust(), Nan::To<uint32_t>(info[3]).FromJust(),
                                   Nan::To<uint32_t>(info[4]).FromJust(), Nan::To<uint32_t>(info[5]).FromJust(),
                                   Nan::To<uint32_t>(info[6]).FromJust(), 0);
      } else if (info.Length() == 8 && info[0]->IsUint32() && info[1]->IsUint32()
                 && info[2]->IsUint32() && info[3]->IsUint32()
                 && info[4]->IsUint32() && info[5]->IsUint32()
                 && info[6]->IsUint32() && info[7]->IsUint32()) {
        ret = win->panel()->border(Nan::To<uint32_t>(info[0]).FromJust(), Nan::To<uint32_t>(info[1]).FromJust(),
                                   Nan::To<uint32_t>(info[2]).FromJust(), Nan::To<uint32_t>(info[3]).FromJust(),
                                   Nan::To<uint32_t>(info[4]).FromJust(), Nan::To<uint32_t>(info[5]).FromJust(),
                                   Nan::To<uint32_t>(info[6]).FromJust(), Nan::To<uint32_t>(info[7]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Hline) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsInt32())
        ret = win->panel()->hline(Nan::To<int32_t>(info[0]).FromJust(), 0);
      else if (info.Length() == 2 && info[0]->IsInt32() && info[1]->IsUint32())
        ret = win->panel()->hline(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<uint32_t>(info[1]).FromJust());
      else if (info.Length() == 3 && info[0]->IsInt32() && info[1]->IsInt32()
               && info[2]->IsInt32()) {
        ret = win->panel()->hline(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                                  Nan::To<int32_t>(info[2]).FromJust(), 0);
      } else if (info.Length() == 4 && info[0]->IsInt32() && info[1]->IsInt32()
                 && info[2]->IsInt32() && info[3]->IsUint32()) {
        ret = win->panel()->hline(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                                  Nan::To<int32_t>(info[2]).FromJust(), Nan::To<uint32_t>(info[3]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Vline) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsInt32())
        ret = win->panel()->vline(Nan::To<int32_t>(info[0]).FromJust(), 0);
      else if (info.Length() == 2 && info[0]->IsInt32() && info[1]->IsUint32())
        ret = win->panel()->vline(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<uint32_t>(info[1]).FromJust());
      else if (info.Length() == 3 && info[0]->IsInt32() && info[1]->IsInt32()
               && info[2]->IsInt32()) {
        ret = win->panel()->vline(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                                  Nan::To<int32_t>(info[2]).FromJust(), 0);
      } else if (info.Length() == 4 && info[0]->IsInt32() && info[1]->IsInt32()
                 && info[2]->IsInt32() && info[3]->IsUint32()) {
        ret = win->panel()->vline(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                                  Nan::To<int32_t>(info[2]).FromJust(), Nan::To<uint32_t>(info[3]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Erase) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret = win->panel()->erase();

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Clear) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret = win->panel()->clear();

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Clrtobot) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret = win->panel()->clrtobot();

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Clrtoeol) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret = win->panel()->clrtoeol();

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Delch) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 0)
        ret = win->panel()->delch();
      else if (info.Length() == 2 && info[0]->IsInt32() && info[1]->IsInt32())
        ret = win->panel()->delch(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Deleteln) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret = win->panel()->deleteln();

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Scroll) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 0)
        ret = win->panel()->scroll(1);
      else if (info.Length() == 1 && info[0]->IsInt32())
        ret = win->panel()->scroll(Nan::To<int32_t>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Setscrreg) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 2 && info[0]->IsInt32() && info[1]->IsInt32()) {
        ret = win->panel()->setscrreg(Nan::To<int32_t>(info[0]).FromJust(),
                                      Nan::To<int32_t>(info[1]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    /*static NAN_METHOD(Touchline) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 2 && info[0]->IsInt32() && info[1]->IsInt32()) {
        ret = win->panel()->touchline(Nan::To<int32_t>(info[0]).FromJust(),
                                      Nan::To<int32_t>(info[1]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }*/

    static NAN_METHOD(Touchwin) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret = win->panel()->touchwin();

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Untouchwin) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret = win->panel()->untouchwin();

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Touchln) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 2 && info[0]->IsInt32() && info[1]->IsInt32()) {
        ret = win->panel()->touchln(Nan::To<int32_t>(info[0]).FromJust(),
                                    Nan::To<int32_t>(info[1]).FromJust(), true);
      } else if (info.Length() == 3 && info[0]->IsInt32() && info[1]->IsInt32()
                 && info[2]->IsBoolean()) {
        ret = win->panel()->touchln(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                                    Nan::To<bool>(info[2]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Is_linetouched) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      bool ret;
      if (info.Length() == 1 && info[0]->IsInt32())
        ret = win->panel()->is_linetouched(Nan::To<int32_t>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(Nan::New<Boolean>(ret));
    }

    static NAN_METHOD(Redrawln) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 2 && info[0]->IsInt32() && info[1]->IsInt32()) {
        ret = win->panel()->redrawln(Nan::To<int32_t>(info[0]).FromJust(),
                                     Nan::To<int32_t>(info[1]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Syncdown) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      win->panel()->syncdown();
    }

    static NAN_METHOD(Syncup) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      win->panel()->syncup();
    }

    static NAN_METHOD(Cursyncup) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      win->panel()->cursyncup();
    }

    static NAN_METHOD(Wresize) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 2 && info[0]->IsInt32() && info[1]->IsInt32()) {
        ret = win->panel()->wresize(Nan::To<int32_t>(info[0]).FromJust(),
                                    Nan::To<int32_t>(info[1]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Print) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsString()) {
        Nan::Utf8String str(info[0].As<String>());
        ret = win->panel()->printw("%s", ToCString(str));
      } else if (info.Length() == 3 && info[0]->IsInt32() && info[1]->IsInt32()
                 && info[2]->IsString()) {
        Nan::Utf8String str(info[2].As<String>());
        ret = win->panel()->printw(Nan::To<int32_t>(info[0]).FromJust(), Nan::To<int32_t>(info[1]).FromJust(),
                                   "%s", ToCString(str));
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Clearok) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsBoolean())
        ret = win->panel()->clearok(Nan::To<bool>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Scrollok) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsBoolean())
        ret = win->panel()->scrollok(Nan::To<bool>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Idlok) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsBoolean())
        ret = win->panel()->idlok(Nan::To<bool>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Idcok) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      if (info.Length() == 1 && info[0]->IsBoolean())
        win->panel()->idcok(Nan::To<bool>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }
    }

    static NAN_METHOD(Leaveok) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsBoolean())
        ret = win->panel()->leaveok(Nan::To<bool>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Syncok) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsBoolean())
        ret = win->panel()->syncok(Nan::To<bool>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Immedok) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      if (info.Length() == 1 && info[0]->IsBoolean())
        win->panel()->immedok(Nan::To<bool>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }
    }

    static NAN_METHOD(Keypad) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsBoolean())
        ret = win->panel()->keypad(Nan::To<bool>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Meta) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsBoolean())
        ret = win->panel()->meta(Nan::To<bool>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Standout) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret;
      if (info.Length() == 1 && info[0]->IsBoolean()) {
        if (Nan::To<bool>(info[0]).FromJust())
          ret = win->panel()->standout();
        else
          ret = win->panel()->standend();
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Resetscreen) {
      ::endwin(); //MyPanel::resetScreen();
    }

    static NAN_METHOD(Setescdelay) {
      if (info.Length() == 0 || !info[0]->IsInt32()) {
        return Nan::ThrowError("Invalid argument");
      }

      ::set_escdelay(Nan::To<int32_t>(info[0]).FromJust());
    }

    static NAN_METHOD(LeaveNcurses) {
      uv_poll_stop(read_watcher_);
      start_rw_poll = true;

      ::def_prog_mode();
      ::endwin();
    }

    static NAN_METHOD(RestoreNcurses) {
      ::reset_prog_mode();
      if (start_rw_poll) {
        uv_poll_start(read_watcher_, UV_READABLE, io_event);
        start_rw_poll = false;
      }

      MyPanel::redraw();
    }

    static NAN_METHOD(Beep) {
      ::beep();
    }

    static NAN_METHOD(Flash) {
      ::flash();
    }

    static NAN_METHOD(DoUpdate) {
      MyPanel::doUpdate();
    }

    static NAN_METHOD(Colorpair) {
      int ret;
      if (info.Length() == 1 && info[0]->IsInt32())
        ret = MyPanel::pair(Nan::To<int32_t>(info[0]).FromJust());
      else if (info.Length() == 3 && info[0]->IsInt32() && info[1]->IsInt32()
               && info[2]->IsInt32()) {
        ret = MyPanel::pair(Nan::To<int32_t>(info[0]).FromJust(), static_cast<short>(Nan::To<int32_t>(info[1]).FromJust()),
                            static_cast<short>(Nan::To<int32_t>(info[2]).FromJust()));
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Colorfg) {
      int ret;
      if (info.Length() == 1 && info[0]->IsUint32())
        ret = MyPanel::getFgcolor(Nan::To<uint32_t>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Colorbg) {
      int ret;
      if (info.Length() == 1 && info[0]->IsUint32())
        ret = MyPanel::getBgcolor(Nan::To<uint32_t>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Dup2) {
      int ret;
      if (info.Length() == 2 && info[0]->IsUint32() && info[1]->IsUint32())
        ret = dup2(Nan::To<uint32_t>(info[0]).FromJust(), Nan::To<uint32_t>(info[1]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Dup) {
      int ret;
      if (info.Length() == 1 && info[0]->IsUint32())
        ret = dup(Nan::To<uint32_t>(info[0]).FromJust());
      else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Copywin) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      int ret;
      if (info.Length() == 7 && info[1]->IsUint32() && info[2]->IsUint32()
          && info[3]->IsUint32() && info[4]->IsUint32() && info[5]->IsUint32()
          && info[6]->IsUint32()) {
        ret = win->panel()->copywin(
                    *(Nan::ObjectWrap::Unwrap<Window>(info[0].As<Object>())->panel()),
                    Nan::To<uint32_t>(info[1]).FromJust(), Nan::To<uint32_t>(info[2]).FromJust(),
                    Nan::To<uint32_t>(info[3]).FromJust(), Nan::To<uint32_t>(info[4]).FromJust(),
                    Nan::To<uint32_t>(info[5]).FromJust(), Nan::To<uint32_t>(info[6]).FromJust());
      } else if (info.Length() == 8 && info[1]->IsUint32()
                 && info[2]->IsUint32() && info[3]->IsUint32()
                 && info[4]->IsUint32() && info[5]->IsUint32()
                 && info[6]->IsUint32() && info[7]->IsBoolean()) {
        ret = win->panel()->copywin(
                    *(Nan::ObjectWrap::Unwrap<Window>(info[0].As<Object>())->panel()),
                    Nan::To<uint32_t>(info[1]).FromJust(), Nan::To<uint32_t>(info[2]).FromJust(),
                    Nan::To<uint32_t>(info[3]).FromJust(), Nan::To<uint32_t>(info[4]).FromJust(),
                    Nan::To<uint32_t>(info[5]).FromJust(), Nan::To<uint32_t>(info[6]).FromJust(),
                    Nan::To<bool>(info[7]).FromJust());
      } else {
        return Nan::ThrowError("Invalid number and/or types of arguments");
      }

      info.GetReturnValue().Set(ret);
    }

    static NAN_METHOD(Redrawwin) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());

      int ret = win->panel()->redrawwin();

      info.GetReturnValue().Set(ret);
    }

    // -- Getters/Setters ------------------------------------------------------
    static NAN_GETTER(EchoStateGetter) {
      assert(property == echo_state_symbol);

      info.GetReturnValue().Set(MyPanel::echo());
    }

    static NAN_SETTER(EchoStateSetter) {
      assert(property == echo_state_symbol);

      if (!value->IsBoolean()) {
          return Nan::ThrowTypeError("echo should be of Boolean value");
      }

      MyPanel::echo(Nan::To<bool>(value).FromJust());
    }

   static NAN_GETTER(ShowcursorStateGetter) {
      assert(property == showcursor_state_symbol);

      info.GetReturnValue().Set(MyPanel::showCursor());
    }

    static NAN_SETTER(ShowcursorStateSetter) {
      assert(property == showcursor_state_symbol);

      if (!value->IsBoolean()) {
          return Nan::ThrowTypeError("showCursor should be of Boolean value");
      }
      MyPanel::showCursor(Nan::To<bool>(value).FromJust());
    }

    static NAN_GETTER(LinesStateGetter) {
      assert(property == lines_state_symbol);

      info.GetReturnValue().Set(LINES);
    }

    static NAN_GETTER(ColsStateGetter) {
      assert(property == cols_state_symbol);
      info.GetReturnValue().Set(COLS);
    }

    static NAN_GETTER(TabsizeStateGetter) {
      assert(property == tabsize_state_symbol);
      info.GetReturnValue().Set(TABSIZE);
    }

    static NAN_GETTER(HasmouseStateGetter) {
      assert(property == hasmouse_state_symbol);

      info.GetReturnValue().Set(MyPanel::hasMouse());
    }

    static NAN_GETTER(HiddenStateGetter) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == hidden_state_symbol);

      info.GetReturnValue().Set(win->panel()->hidden());
    }

    static NAN_GETTER(HeightStateGetter) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == height_state_symbol);

      info.GetReturnValue().Set(win->panel()->height());
    }

    static NAN_GETTER(WidthStateGetter) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == width_state_symbol);

      info.GetReturnValue().Set(win->panel()->width());
    }

    static NAN_GETTER(BegxStateGetter) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == begx_state_symbol);

      info.GetReturnValue().Set(win->panel()->begx());
    }

    static NAN_GETTER(BegyStateGetter) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == begy_state_symbol);

      info.GetReturnValue().Set(win->panel()->begy());
    }

    static NAN_GETTER(CurxStateGetter) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == curx_state_symbol);

      info.GetReturnValue().Set(win->panel()->curx());
    }

    static NAN_GETTER(CuryStateGetter) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == cury_state_symbol);

      info.GetReturnValue().Set(win->panel()->cury());
    }

    static NAN_GETTER(MaxxStateGetter) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == maxx_state_symbol);

      info.GetReturnValue().Set(win->panel()->maxx());
    }

    static NAN_GETTER(MaxyStateGetter) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == maxy_state_symbol);

      info.GetReturnValue().Set(win->panel()->maxy());
    }

    static NAN_GETTER(BkgdStateGetter) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == bkgd_state_symbol);

      info.GetReturnValue().Set(win->panel()->getbkgd());
    }

    static NAN_SETTER(BkgdStateSetter) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == bkgd_state_symbol);
      unsigned int val = 32;

      if (!value->IsUint32() && !value->IsString()) {
          return Nan::ThrowTypeError("bkgd should be of unsigned integer or a string value");
      }
      if (value->IsString()) {
        Nan::Utf8String str(value.As<String>());
        if (str.length() > 0)
          val = (unsigned int)((*str)[0]);
      } else
        val = Nan::To<uint32_t>(value).FromJust();

      win->panel()->bkgd(val);
    }

    static NAN_GETTER(HascolorsStateGetter) {
      assert(property == hascolors_state_symbol);
      info.GetReturnValue().Set(MyPanel::has_colors());
    }

    static NAN_GETTER(NumcolorsStateGetter) {
      assert(property == numcolors_state_symbol);
      info.GetReturnValue().Set(MyPanel::num_colors());
    }

    static NAN_GETTER(MaxcolorpairsStateGetter) {
      assert(property == maxcolorpairs_state_symbol);
      info.GetReturnValue().Set(MyPanel::max_pairs());
    }

    static NAN_GETTER(WintouchedStateGetter) {
      Window *win = Nan::ObjectWrap::Unwrap<Window>(info.This());
      assert(win);
      assert(property == wintouched_state_symbol);

      info.GetReturnValue().Set(win->panel()->is_wintouched());
    }

    static NAN_GETTER(RawStateGetter) {
      assert(property == raw_state_symbol);
      info.GetReturnValue().Set(MyPanel::raw());
    }

    static NAN_SETTER(RawStateSetter) {
      assert(property == raw_state_symbol);

      if (!value->IsBoolean()) {
          return Nan::ThrowTypeError("raw should be of Boolean value");
      }

      MyPanel::raw(Nan::To<bool>(value).FromJust());
    }

    static NAN_GETTER(ACSConstsGetter) {
      info.GetReturnValue().Set(Nan::New(ACS_Chars));
    }

    static NAN_GETTER(KeyConstsGetter) {
      info.GetReturnValue().Set(Nan::New(Keys));
    }

    static NAN_GETTER(ColorConstsGetter) {
      info.GetReturnValue().Set(Nan::New(Colors));
    }

    static NAN_GETTER(AttrConstsGetter) {
      info.GetReturnValue().Set(Nan::New(Attrs));
    }

    static NAN_GETTER(NumwinsGetter) {
      info.GetReturnValue().Set(wincounter);
    }

    Window() : Nan::ObjectWrap() {
      panel_ = NULL;
      this->init();
      assert(panel_ != NULL);
    }

    Window(int nlines, int ncols, int begin_y, int begin_x) : Nan::ObjectWrap() {
      panel_ = NULL;
      this->init(nlines, ncols, begin_y, begin_x);
      assert(panel_ != NULL);
    }

    ~Window() {
      Emit.Reset();
    }

  private:
    static void io_event (uv_poll_t* w, int status, int revents) {
      Nan::HandleScope scope;

      if (status < 0)
        return;

      if (revents & UV_READABLE) {
        wint_t chr;
        uint16_t tmp;
        int ret;
        while ((ret = get_wch(&chr)) != ERR) {
          // 410 == KEY_RESIZE
          if (chr == 410 || !topmost_panel || !topmost_panel->getWindow()
              || !topmost_panel->getPanel()) {
            //if (chr != 410)
            //  ungetch(chr);
            return;
          }

          tmp = static_cast<uint16_t>(chr);

          Local<Value> emit_argv[4] = {
            Nan::New(inputchar_symbol),
            Nan::New<String>(&tmp, 1).ToLocalChecked(),
            Nan::New(chr),
            Nan::New(ret == KEY_CODE_YES)
          };
          Nan::TryCatch try_catch;
          Nan::New(topmost_panel->getWindow()->Emit)->Call(
              topmost_panel->getWindow()->handle(), 4, emit_argv
          );
          if (try_catch.HasCaught())
            Nan::FatalException(try_catch);
        }
      }
    }

    MyPanel *panel_;
};

extern "C" {
  static NAN_MODULE_INIT(init) {
    Nan::HandleScope scope;
    Window::Initialize(target);
  }

  NODE_MODULE(binding, init)
}
