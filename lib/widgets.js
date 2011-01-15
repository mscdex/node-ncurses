var nc = require('../ncurses');
var origColors;

function backupColors() {
  if (!nc.hasColors)
    return;
  if (!origColors) {
    origColors = new Array(nc.maxColorPairs);
    for (var i=1; i<nc.maxColorPairs-1; i++)
      origColors[i] = [nc.colorFg(nc.colorPair(i)), nc.colorBg(nc.colorPair(i))];
  } else {
    for (var i=1; i<nc.maxColorPairs-1; i++) {
      origColors[i][0] = nc.colorFg(nc.colorPair(i));
      origColors[i][1] = nc.colorBg(nc.colorPair(i));
    }
  }
}

function restoreColors() {
  if (!nc.hasColors)
    return;
  for (var i=1; i<nc.maxColorPairs-1; i++)
    nc.colorPair(i, origColors[i][0], origColors[i][1]);
}

function times(str, num) {
  var ret = '';
  for (var i=0; i<num; i++)
    ret += str;
  return ret;
}

function pad(str, which, amount, padchar) {
  padchar = padchar || ' ';
  amount -= str.length;
  if (amount > 0) {
    var nbefore = 0, nafter = 0;
    if (which !== 'left') {
      if (which === 'center') {
        nbefore = Math.floor(amount/2);
        nafter = amount - nbefore;
      } else
        nbefore = amount;
    }
    str = times(padchar, nbefore) + str + times(padchar, nafter);
  }
  return str;
}

var Popup = function(height, width, position, title) {
  var win, pos;
  pos = position || 'center';
  pos = parsePosition(height, width, pos);

  win = new nc.Window(height, width, pos[0], pos[1]);
  if (title)
    win.frame(title);
  else if (title !== null)
    win.frame();

  return win;
};

exports.MessageBox = function(text, options, cb) {
  backupColors();
  var win, lines = text.split('\n'), btnStartCol, selection,
      width = 0, height, buttons, textAlign,
      style = {
        colors: {
          fg: undefined,
          bg: undefined,
          button: {
            fg: undefined,
            bg: undefined
          },
          hl: {
            fg: undefined,
            bg: undefined
          }
        }
      }, fnDraw;
  if (!cb && typeof options === 'function')
    cb = options;
  if (typeof options !== 'object')
    options = {};
  buttons = (options.buttons && !Array.isArray(options.buttons) ? [options.buttons] : options.buttons);
  textAlign = options.textAlign || 'center';
  selection = options.default || 0;
  height = lines.length+2+(buttons ? 2 : 0);
  extend(true, style, options.style);

  // Fit to the content
  for (var i=0,len=lines.length; i<len; i++)
    if (lines[i].length > width)
      width = lines[i].length;
  if (buttons) {
    if (buttons.length === 1)
      selection = 0;
    else if (typeof selection === 'string')
      selection = buttons.indexOf(selection);
    if (buttons.join('  ').length > width)
      width = buttons.join('  ').length;
    btnStartCol = Math.floor((width-buttons.join('  ').length)/2)+2;
  }
  width += 4;

  fnDraw = function() {
    var isRedraw = (win ? true : false);
    if (isRedraw) {
      //win.move(boxY, boxX);
      //nc.redraw();
    } else {
      win = Popup(height, width, options.pos, options.title);
      if (nc.hasColors) {
        if (!style.colors)
          style.colors = {};
        style.colors.fg = parseColor(style.colors.fg, nc.colorFg(nc.colorPair(0)));
        style.colors.bg = parseColor(style.colors.bg, nc.colorBg(nc.colorPair(0)));
        if (!style.colors.button)
          style.colors.button = {};
        style.colors.button.fg = parseColor(style.colors.button.fg, style.colors.fg);
        style.colors.button.bg = parseColor(style.colors.button.bg, style.colors.bg);
        if (!style.colors.hl)
          style.colors.hl = {};
        style.colors.hl.fg = parseColor(style.colors.hl.fg, style.colors.bg);
        style.colors.hl.bg = parseColor(style.colors.hl.bg, style.colors.fg);

        nc.colorPair(nc.maxColorPairs-1, style.colors.fg, style.colors.bg);
        nc.colorPair(nc.maxColorPairs-2, style.colors.button.fg, style.colors.button.bg);
        nc.colorPair(nc.maxColorPairs-3, style.colors.hl.fg, style.colors.hl.bg);
        win.attrset(nc.colorPair(nc.maxColorPairs-1));
        win.bkgd = ' '|nc.colorPair(nc.maxColorPairs-1);
      }
      win.on('inputChar', function(c, i) {
        if ((i === 9 || i === nc.keys.LEFT || i === nc.keys.RIGHT
            || i === nc.keys.UP || i === nc.keys.DOWN)
            && Array.isArray(buttons) && buttons.length > 1) {
          var col = btnStartCol+buttons.slice(0, selection).join('  ').length+(selection > 0 ? 2 : 0);
          if (nc.hasColors)
            win.chgat(height-2, col, buttons[selection].length, nc.attrs.NORMAL, nc.maxColorPairs-2);
          else
            win.chgat(height-2, col, buttons[selection].length, nc.attrs.NORMAL);
          if (i === 9 || i === nc.keys.RIGHT || i === nc.keys.UP)
            selection++;
          else
            selection--;
          if (selection === buttons.length)
            selection = 0;
          else if (selection < 0)
            selection = buttons.length-1;
          col = btnStartCol+buttons.slice(0, selection).join('  ').length+(selection > 0 ? 2 : 0);
          if (nc.hasColors)
            win.chgat(height-2, col, buttons[selection].length, nc.attrs.NORMAL, nc.maxColorPairs-3);
          else
            win.chgat(height-2, col, buttons[selection].length, nc.attrs.REVERSE);
          win.refresh();
          return;
        } else if (i === nc.keys.ESC || (i === nc.keys.NEWLINE && !buttons))
          selection = undefined;
        else if (i === nc.keys.NEWLINE && buttons)
          selection = buttons[selection];
        else
          return;
        win.close();
        restoreColors();
        if (cb)
          process.nextTick(function(){ cb(selection); });
        //process.removeListener('SIGWINCH', fnDraw);
      });
    }

    var ln = 1;
    for (var i=0,len=lines.length; i<len; i++,ln++) {
      lines[i] = pad(lines[i], textAlign, width-4);
      win.addstr(ln, 2, lines[i]);
    }

    if (buttons) {
      var col = btnStartCol+buttons.slice(0, selection).join('  ').length+(selection > 0 ? 2 : 0);
      win.hline(ln++, 1, width-2);
      win.addstr(ln, 2, pad(buttons.join('  '), 'center', width-4));
      if (nc.hasColors) {
        for (var i=0; i<buttons.length; i++) {
          win.chgat(ln, col, buttons[i].length, nc.attrs.NORMAL, nc.maxColorPairs-(i === 0 ? 3 : 2));
          col += buttons[i].length + 2;
        }
      } else
        win.chgat(ln++, col, buttons[selection].length, nc.attrs.REVERSE);
    }
    win.refresh();
  };
  fnDraw();
  //process.on('SIGWINCH', fnDraw);
};

exports.InputBox = function(text, options, cb) {
  backupColors();
  var win, lines = text.split('\n'), buffer = "", old_curs = nc.showCursor,
      width = 0, height = lines.length+4, textAlign,
      style = {
        colors: {
          fg: undefined,
          bg: undefined,
          input: {
            fg: undefined,
            bg: undefined
          }
        }
      }, fnDraw;
  if (!cb && typeof options === 'function')
    cb = options;
  if (typeof options !== 'object')
    options = {};
  textAlign = options.textAlign || 'center';
  extend(true, style, options.style);

  // Fit to the content
  for (var i=0,len=lines.length; i<len; i++)
    if (lines[i].length > width)
      width = lines[i].length;
  width += 4;

  fnDraw = function() {
    var isRedraw = (win ? true : false);
    if (isRedraw) {
      //win.move(boxY, boxX);
      //nc.redraw();
    } else {
      win = Popup(height, width, options.pos);
      if (nc.hasColors) {
        if (!style.colors)
          style.colors = {};
        style.colors.fg = parseColor(style.colors.fg, nc.colorFg(nc.colorPair(0)));
        style.colors.bg = parseColor(style.colors.bg, nc.colorBg(nc.colorPair(0)));
        if (!style.colors.input)
          style.colors.input = {};
        style.colors.input.fg = parseColor(style.colors.input.fg, style.colors.bg);
        style.colors.input.bg = parseColor(style.colors.input.bg, style.colors.fg);

        nc.colorPair(nc.maxColorPairs-1, style.colors.fg, style.colors.bg);
        nc.colorPair(nc.maxColorPairs-2, style.colors.input.fg, style.colors.input.bg);
        win.attrset(nc.colorPair(nc.maxColorPairs-1));
        win.bkgd = ' '|nc.colorPair(nc.maxColorPairs-1);
      }
      win.on('inputChar', function(c, i) {
        if (i === nc.keys.ESC)
          buffer = undefined;
        else if (i !== nc.keys.NEWLINE) {
          if (i === nc.keys.BACKSPACE && win.curx > 1) {
            var prev_x = win.curx-1;
            win.delch(height-2, prev_x);
            buffer = buffer.substring(0, prev_x-1) + buffer.substr(prev_x);
            win.insch(height-2, width-2, 32);
            win.cursor(height-2, prev_x);
          } else if (i === nc.keys.DEL && win.curx > 0 && win.curx < width-1) {
            var prev_x = win.curx;
            win.delch(height-2, win.curx);
            buffer = buffer.substring(0, win.curx-1) + buffer.substr(win.curx);
            win.insch(height-2, width-2, 32);
            win.cursor(height-2, prev_x);
          } else if (i === nc.keys.LEFT && win.curx > 1)
            win.cursor(height-2, win.curx-1);
          else if (i === nc.keys.RIGHT && win.curx < buffer.length+1)
            win.cursor(height-2, win.curx+1);
          else if (i === nc.keys.END)
            win.cursor(height-2, buffer.length+1);
          else if (i === nc.keys.HOME)
            win.cursor(height-2, 1);
          else if (i >= 32 && i <= 126 && win.curx < width-1) {
            win.echochar(i);
            buffer += c;
          }
          if (nc.hasColors) {
            var x = win.curx;
            win.chgat(height-2, 1, width-2, nc.attrs.NORMAL, nc.maxColorPairs-1);
            win.chgat(height-2, 1, buffer.length, nc.attrs.NORMAL, nc.maxColorPairs-2);
            win.cursor(height-2, x);
          }
          win.refresh();
          return;
        }
        nc.showCursor = old_curs;
        win.close();
        restoreColors();
        if (cb)
          process.nextTick(function(){ cb(buffer); });
        //process.removeListener('SIGWINCH', fnDraw);
      });
    }

    var ln = 1;
    for (var i=0,len=lines.length; i<len; i++,ln++) {
      lines[i] = pad(lines[i], textAlign, width-4);
      win.addstr(ln, 2, lines[i]);
    }

    win.hline(ln++, 1, width-2);
    win.cursor(height-2, 1);
    nc.showCursor = true;

    if (options.title)
      win.frame(options.title);
    else
      win.frame();

    nc.redraw();
  };
  fnDraw();
  //process.on('SIGWINCH', fnDraw);
};

var months = ['January', 'February', 'March', 'April', 'May', 'June',
              'July', 'August', 'September', 'October', 'November', 'December'],
    dow = ['Su', 'Mo', 'Tu', 'We', 'Th', 'Fr', 'Sa'],
    numdays = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
exports.Calendar = function(options, cb) {
  backupColors();
  var win, width = 24, height = 11, highlight, firstDow, maxday, 
      style = {
        colors: {
          fg: undefined,
          bg: undefined,
          hl: {
            fg: undefined,
            bg: undefined
          },
          today: {
            fg: undefined,
            bg: undefined
          }
        }
      }, fnDraw;
  if (!cb && typeof options === 'function')
    cb = options;
  if (typeof options !== 'object')
    options = {};
  highlight = options.initialDate || (new Date);
  extend(true, style, options.style);

  fnDraw = function() {
    var isRedraw = (win ? true : false);
    if (isRedraw) {
      //win.move(boxY, boxX);
      //nc.redraw();
    } else {
      win = Popup(height, width, options.pos);
      if (nc.hasColors) {
        if (!style.colors)
          style.colors = {};
        style.colors.fg = parseColor(style.colors.fg, nc.colorFg(nc.colorPair(0)));
        style.colors.bg = parseColor(style.colors.bg, nc.colorBg(nc.colorPair(0)));
        if (!style.colors.hl)
          style.colors.hl = {};
        style.colors.hl.fg = parseColor(style.colors.hl.fg, style.colors.bg);
        style.colors.hl.bg = parseColor(style.colors.hl.bg, style.colors.fg);
        if (!style.colors.today)
          style.colors.today = {};
        style.colors.today.fg = parseColor(style.colors.today.fg, style.colors.bg);
        style.colors.today.bg = parseColor(style.colors.today.bg, style.colors.fg);

        nc.colorPair(nc.maxColorPairs-1, style.colors.fg, style.colors.bg);
        nc.colorPair(nc.maxColorPairs-2, style.colors.hl.fg, style.colors.hl.bg);
        nc.colorPair(nc.maxColorPairs-3, style.colors.today.fg, style.colors.today.bg);
        win.attrset(nc.colorPair(nc.maxColorPairs-1));
        win.bkgd = ' '|nc.colorPair(nc.maxColorPairs-1);
      }
      win.on('inputChar', function(c, i) {
        if (i === nc.keys.ESC)
          highlight = undefined;
        else if (i !== nc.keys.NEWLINE) {
          var dir = (i === nc.keys.LEFT || i === nc.keys.UP || i === nc.keys.PPAGE ? -1 : 1);
          if (i === nc.keys.LEFT || i === nc.keys.RIGHT)
            highlight = new Date(+highlight + (86400000*dir));
          else if (i === nc.keys.UP || i === nc.keys.DOWN)
            highlight = new Date(+highlight + (604800000*dir));
          else if (i === nc.keys.PPAGE || i === nc.keys.NPAGE) {
            var year = highlight.getFullYear(), month = highlight.getMonth()+dir,
                day = highlight.getDate();
            if (month < 0) {
              month = 11;
              year--;
            } else if (month > 11) {
              month = 0;
              year++;
            }
            maxday = numdays[month];
            if (month === 1 && year % 4 === 0)
              maxday++; // leap year
            if (day > maxday)
              day = maxday;
            highlight = new Date((month+1) + '/' + day + '/' + year);
          } else
            return;
          if (dir !== 0)
            fnDraw();
          else
            win.refresh();
          return;
        }
        win.close();
        restoreColors();
        if (cb)
          process.nextTick(function(){ cb(highlight); });
        //process.removeListener('SIGWINCH', fnDraw);
      });
    }

    var ln = 1;
    win.erase();
    maxday = numdays[highlight.getMonth()];
    if (highlight.getMonth() === 1 && highlight.getFullYear() % 4 === 0)
      maxday++; // leap year
    win.addstr(ln, 2, months[highlight.getMonth()] + ' ' + highlight.getDate());
    win.addstr(ln++, width-6, ''+highlight.getFullYear());
    win.addstr(ln++, 2, dow.join(' '));
    ln++;
    firstDow = new Date((highlight.getMonth()+1) + '/1/' + highlight.getFullYear()).getDay();
    var today = new Date;
    for (var pos=(firstDow*3)+2,day=1,curday=highlight.getDate(); ln<height-1; ln++,pos=2) {
      while (pos < 23 && day <= maxday) {
        win.addstr(ln, pos, pad(''+(day), 'right', 2, '0'));
        if (today.getFullYear() === highlight.getFullYear()
            && today.getMonth() === highlight.getMonth()
            && today.getDate() === day) {
          if (nc.hasColors)
            win.chgat(ln, pos, 2, nc.attrs.NORMAL, nc.maxColorPairs-3);
          else
            win.chgat(ln, pos, 2, nc.attrs.REVERSE);
        }
        if (day++ === curday) {
          if (nc.hasColors)
            win.chgat(ln, pos, 2, nc.attrs.NORMAL, nc.maxColorPairs-2);
          else
            win.chgat(ln, pos, 2, nc.attrs.REVERSE);
        }
        pos += 3;
      }
    }

    if (options.title)
      win.frame(options.title);
    else
      win.frame();

    win.refresh();
  };
  fnDraw();
  //process.on('SIGWINCH', fnDraw);
};

exports.Viewer = function(text, options, cb) {
  backupColors();
  var win, lines = text.split('\n'), numlines = lines.length,
      width = 2, height = 2,
      style = {
        colors: {
          fg: undefined,
          bg: undefined,
        }
      }, top = 1, textAreaHeight, curline = 0, curcol = 0, fnDraw,
      maxlinewidth = lines.reduce(function(a,b){return Math.max(a,b.length)},0);
  if (!cb && typeof options === 'function')
    cb = options;
  if (typeof options !== 'object')
    options = {};
  extend(true, style, options.style);
  if (options.height)
    height = options.height;
  if (options.width)
    width = options.width;
  if (style.linecount)
    top = 2;
  textAreaHeight = height - 1 - top;

  fnDraw = function() {
    var isRedraw = (win ? true : false);
    if (isRedraw) {
      //win.move(boxY, boxX);
      //nc.redraw();
    } else {
      win = Popup(height, width, options.pos, options.title);
      if (nc.hasColors) {
        if (!style.colors)
          style.colors = {};
        style.colors.fg = parseColor(style.colors.fg, nc.colorFg(nc.colorPair(0)));
        style.colors.bg = parseColor(style.colors.bg, nc.colorBg(nc.colorPair(0)));

        nc.colorPair(nc.maxColorPairs-1, style.colors.fg, style.colors.bg);
        win.attrset(nc.colorPair(nc.maxColorPairs-1));
        win.bkgd = ' '|nc.colorPair(nc.maxColorPairs-1);
      }
      win.on('inputChar', function(c, i) {
        var old_x = win.curx, old_y = win.cury;
        if (i !== nc.keys.ESC) {
          var dir = (i === nc.keys.LEFT || i === nc.keys.UP || i === nc.keys.PPAGE ? -1 : 1);
          if (i === nc.keys.HOME) {
            curcol = 0;
            fnDraw();
            return;
          } else if (i === nc.keys.END) {
            if (maxlinewidth > (width-2)) {
              curcol = maxlinewidth - (width-2);
              fnDraw();
              return;
            }
          } else if ((i === nc.keys.LEFT && curcol+dir >= 0) || (i === nc.keys.RIGHT && curcol+(width-2)+dir <= maxlinewidth)) {
            curcol += dir;
            for (var i=0,curs=top,ln=curline; i<Math.min(numlines, textAreaHeight); i++,ln++,curs++) {
              win.cursor(curs, 1);
              win.clrtoeol();
              win.print(lines[ln].substring(curcol, curcol+(width-2)));
            }
          } else if ((i === nc.keys.UP && curline+dir >= 0) || (i === nc.keys.DOWN && curline+textAreaHeight+dir <= numlines)) {
            curline += dir;
            var which = curline;
            win.cursor(top, 1);
            if (dir < 0)
              win.insertln();
            else {
              win.deleteln();
              win.cursor(win.height-2, 1);
              win.clrtoeol();
              which = curline+textAreaHeight-1;
            }
            win.print(lines[which].substring(curcol, curcol+(width-2)));
          } else if (i === nc.keys.PPAGE || i === nc.keys.NPAGE) {
            var numscroll = textAreaHeight;
            win.cursor(top, 1);
            if (dir < 0) {
              if (curline - numscroll < 0)
                numscroll = curline;
              if (numscroll > 0) {
                win.insdelln(numscroll);
                for (var i=top,len=i+numscroll,ln=curline-numscroll; i<=numscroll+(style.linecount ? 1 : 0); i++,ln++) {
                  win.cursor(i, 1);
                  win.clrtoeol();
                  win.print(lines[ln].substring(curcol, curcol+(width-2)));
                }
              }
            } else {
              if ((curline + textAreaHeight) + numscroll > numlines)
                numscroll = numlines - (curline + textAreaHeight);
              if (numscroll > 0) {
                win.insdelln(-numscroll);
                for (var len=height-1,i=len-numscroll,ln=curline+textAreaHeight; i<len; i++,ln++) {
                  win.cursor(i, 1);
                  win.clrtoeol();
                  win.print(lines[ln].substring(curcol, curcol+(width-2)));
                }
              }
            }
            if (numscroll > 0)
              curline += numscroll*dir;
          } else
            return;
          if (style.linecount) {
            win.cursor(1, 1);
            win.clrtoeol();
            win.centertext(1, 'Lines '+ (curline+1) + '-' + (curline+textAreaHeight)
                              + ' of ' + numlines + ' ('
                              + Math.ceil(((curline+textAreaHeight)/numlines)*100)
                              + '%)'
            );
          }
          if (options.title)
            win.frame(options.title);
          else
            win.frame();
          win.refresh();
          return;
        }
        win.close();
        restoreColors();
        if (cb)
          process.nextTick(function(){ cb(); });
        //process.removeListener('SIGWINCH', fnDraw);
      });
    }

    if (style.linecount) {
      win.cursor(1, 1);
      win.clrtoeol();
      win.centertext(1, 'Lines '+ (curline+1) + '-' + (curline+textAreaHeight)
                        + ' of ' + numlines + ' ('
                        + Math.ceil(((curline+textAreaHeight)/numlines)*100)
                        + '%)'
      );
    }
    for (var i=0,curs=top,ln=curline,len=Math.min(numlines, textAreaHeight); i<len; i++,curs++,ln++) {
      win.cursor(curs, 1);
      win.clrtoeol();
      win.print(lines[ln].substring(curcol, curcol+(width-2)));
    }

    if (options.title)
      win.frame(options.title);
    else
      win.frame();

    win.refresh();
  };
  fnDraw();
  //process.on('SIGWINCH', fnDraw);
};

exports.ListBox = function(items, options, cb, cbHover, cbSelect) {
  backupColors();
  var values;
  if (!Array.isArray(items)) {
    values = Object.keys(items).map(function(x){return items[x]});
    items = Object.keys(items);
  }
  var win, numitems = items.length, highlight = 0, itemAreaHeight, itemAreaWidth,
      maxitemwidth = items.reduce(function(a,b){return Math.max(a,b.length)},0),
      width, height, top = 1, curwndline = 1, topline = 0, selection, old_curs = nc.showCursor,
      style = {
        colors: {
          fg: undefined,
          bg: undefined,
          hl: {
            fg: undefined,
            bg: undefined
          },
          sel: {
            fg: undefined,
            bg: undefined
          }
        }
      }, autoColors = [], fnDraw;
  if (!cb && typeof options === 'function')
    cb = options;
  if (typeof options !== 'object')
    options = {};
  width = options.width || (maxitemwidth+2);
  if (options.title && options.title.length > width)
    width = options.title.length;
  height = options.height || 3;
  itemAreaHeight = height-2;
  itemAreaWidth = width-2;
  extend(true, style, options.style);
  if (options.multi)
    selection = {};
  nc.showCursor = false;

  fnDraw = function() {
    var isRedraw = (win ? true : false);
    if (isRedraw) {
      //win.move(boxY, boxX);
      //nc.redraw();
    } else {
      win = Popup(height, width, options.pos);
      if (nc.hasColors) {
        if (!style.colors)
          style.colors = {};
        style.colors.fg = parseColor(style.colors.fg, nc.colorFg(nc.colorPair(0)));
        style.colors.bg = parseColor(style.colors.bg, nc.colorBg(nc.colorPair(0)));
        if (!style.colors.hl)
          style.colors.hl = {};
        style.colors.hl.fg = parseColor(style.colors.hl.fg, style.colors.bg);
        style.colors.hl.bg = parseColor(style.colors.hl.bg, style.colors.fg);
        if (!style.colors.sel)
          style.colors.sel = {};
        style.colors.sel.fg = parseColor(style.colors.sel.fg, style.colors.bg);
        style.colors.sel.bg = parseColor(style.colors.sel.bg, style.colors.fg);

        nc.colorPair(nc.maxColorPairs-1, style.colors.fg, style.colors.bg);
        nc.colorPair(nc.maxColorPairs-2, style.colors.hl.fg, style.colors.hl.bg);
        nc.colorPair(nc.maxColorPairs-3, style.colors.sel.fg, style.colors.sel.bg);
        win.bkgd = ' '|nc.colorPair(nc.maxColorPairs-1);
        var attrs = nc.colorPair(nc.maxColorPairs-1);
        if (style.bold === true)
          attrs |= nc.attrs.BOLD;
        if (style.underline === true)
          attrs |= nc.attrs.UNDERLINE;
        win.attrset(attrs);
      }
      win.on('inputChar', function(c, i) {
        if (i === nc.keys.ESC)
          highlight = undefined;
        else if (i === nc.keys.NEWLINE)
          highlight = (Array.isArray(items[highlight]) ? items[highlight][0] : items[highlight]);
        else if (i === nc.keys.SPACE && options.multi) {
          if (selection[highlight]) {
            if (cbSelect)
              process.nextTick(function(){cbSelect(Array.isArray(selection[highlight]) ? selection[highlight][0] : selection[highlight], false)});
            delete selection[highlight];
          } else {
            if (cbSelect)
              process.nextTick(function(){cbSelect(Array.isArray(selection[highlight]) ? selection[highlight][0] : selection[highlight], true)});
            selection[highlight] = true;
          }
          fnDraw();
          return;
        } else {
          var dir = (i === nc.keys.LEFT || i === nc.keys.UP || i === nc.keys.PPAGE ? -1 : 1);
          if (i === nc.keys.HOME) {
            if (highlight !== 0 && cbHover)
              process.nextTick(function(){cbHover(Array.isArray(items[highlight]) ? items[highlight][0] : items[highlight])});
            win.chgat(curwndline, 1, itemAreaWidth, nc.attrs.NORMAL);
            topline = 0;
            curwndline = 1;
            highlight = 0;
          } else if (i === nc.keys.END) {
            var old_hl = highlight;
            win.chgat(curwndline, 1, itemAreaWidth, nc.attrs.NORMAL);
            if (numitems-itemAreaHeight < 0)
              topline = 0;
            else
              topline = numitems-itemAreaHeight;
            highlight = numitems-1;
            curwndline = (highlight-topline)+1;
            if (highlight !== old_hl && cbHover)
              process.nextTick(function(){cbHover(Array.isArray(items[highlight]) ? items[highlight][0] : items[highlight])});
          } else if ((i === nc.keys.UP && highlight+dir >= 0) || (i === nc.keys.DOWN && highlight+dir < numitems)) {
            highlight += dir;
            if (cbHover)
              process.nextTick(function(){cbHover(Array.isArray(items[highlight]) ? items[highlight][0] : items[highlight])});
            win.chgat(curwndline, 1, itemAreaWidth, nc.attrs.NORMAL);
            if (curwndline + dir >= top && curwndline + dir < height - 1)
              curwndline += dir;
            else {
              var which = highlight;
              topline += dir;
            }
          } else if (i === nc.keys.PPAGE || i === nc.keys.NPAGE) {
            win.chgat(curwndline, 1, itemAreaWidth, nc.attrs.NORMAL);
            var numscroll = itemAreaHeight, old_hl = highlight;
            highlight += numscroll*dir;
            if (highlight >= numitems) {
              highlight = numitems-1;
              topline = (numitems-itemAreaHeight < 0 ? 0 : numitems-itemAreaHeight);
              curwndline = (highlight-topline)+1;
            } else if (highlight < 0) {
              highlight = 0;
              topline = 0;
              curwndline = 1;
            } else {
              topline += numscroll*dir;
              if (topline < 0)
                topline = 0;
              else if (topline > (numitems-itemAreaHeight) && topline <= (numitems-1))
                topline = numitems-itemAreaHeight;
              curwndline = (highlight-topline)+1;
            }
            if (old_hl !== highlight && cbHover)
              process.nextTick(function(){cbHover(Array.isArray(items[highlight]) ? items[highlight][0] : items[highlight])});
          } else
            return;

          fnDraw();
          return;
        }
        if (options.multi) {
          selection = Object.keys(selection).map(function(x) {
            var val = items[x];
            if (Array.isArray(val))
              val = val[0];
            if (values)
              val = values[x];
          });
          if (!selection.length)
            selection = undefined;
        } else {
          selection = highlight;
          if (values)
            selection = values[items.indexOf(selection)];
        }
        nc.showCursor = old_curs;
        win.close();
        restoreColors();
        if (cb)
          process.nextTick(function(){ cb(selection); });
        //process.removeListener('SIGWINCH', fnDraw);
      });
    }

    for (var i=0,curs=top,ln=topline,txt,len=Math.min(numitems, itemAreaHeight); i<len; i++,curs++,ln++) {
      txt = (Array.isArray(items[ln]) ? items[ln][0] : items[ln]);
      win.cursor(curs, 1);
      win.clrtoeol();
      win.addstr(txt, Math.min(txt.length, width-2));
      if (ln === highlight || (options.multi && selection[ln])) {
        if (nc.hasColors) {
          if (options.multi && selection[ln])
            win.chgat(curs, 1, -1, nc.attrs.NORMAL, nc.maxColorPairs-3);
          if (ln === highlight)
            win.chgat(curs, 1, -1, nc.attrs.NORMAL, nc.maxColorPairs-2);
        } else
          win.chgat(curs, 1, -1, nc.attrs.REVERSE);
      } else if (nc.hasColors && Array.isArray(items[ln]) && items[ln].length === 2
                 && typeof items[ln][1] === 'object') {
        var fg = style.colors.fg, bg = style.colors.bg;
        if (typeof items[ln][1].fg !== 'undefined')
          fg = parseColor(items[ln][1].fg);
        if (typeof items[ln][1].bg !== 'undefined')
          bg = parseColor(items[ln][1].bg);
        var found = false, pair = 1;
        for (var c=0; c<autoColors.length; c++) {
          if (autoColors[c].fg === fg && autoColors[c].bg === bg) {
            found = true;
            pair = (nc.maxColorPairs-4)-c;
            break;
          }
        }
        if (!found && autoColors.length < nc.maxColorPairs) {
          autoColors.push({fg: fg, bg: bg});
          pair = (nc.maxColorPairs-4)-(autoColors.length-1);
          nc.colorPair(pair, fg, bg);
        }
        win.chgat(curs, 1, -1, nc.attrs.NORMAL, pair);
      }
    }
    if (numitems && cbHover)
      process.nextTick(function(){cbHover(Array.isArray(items[highlight]) ? items[highlight][0] : items[highlight])});


    if (options.title)
      win.frame(options.title);
    else
      win.frame();

    win.refresh();
  };
  fnDraw();
  //process.on('SIGWINCH', fnDraw);
};

exports.Marquee = function(text, options) {
  backupColors();
  var self = this, win, dir, width, delay, idx, textlen,
      style = {
        colors: {
          fg: undefined,
          bg: undefined
        }
      }, fnDraw, fnAnimate;
  if (typeof options !== 'object')
    options = {};
  delay = options.delay || 250; // ms
  dir = options.dir || 'left';
  width = options.width || nc.cols;
  if (dir === 'left') {
    text = times(' ', width) + text + ' ';
    idx = 0;
  } else {
    text = times(' ', width) + text;
    idx = text.length-1;
  }
  textlen = text.length;
  this._tmr = undefined;
  extend(true, style, options.style);

  fnDraw = function() {
    var isRedraw = (win ? true : false);
    if (isRedraw) {
      //win.move(boxY, boxX);
      //nc.redraw();
    } else {
      self.win = win = Popup(1, width, options.pos, null);
      if (nc.hasColors) {
        if (!style.colors)
          style.colors = {};
        style.colors.fg = parseColor(style.colors.fg, nc.colorFg(nc.colorPair(0)));
        style.colors.bg = parseColor(style.colors.bg, nc.colorBg(nc.colorPair(0)));

        nc.colorPair(nc.maxColorPairs-1, style.colors.fg, style.colors.bg);
        win.bkgd = ' '|nc.colorPair(nc.maxColorPairs-1);
        var attrs = nc.colorPair(nc.maxColorPairs-1);
        if (style.bold === true)
          attrs |= nc.attrs.BOLD;
        if (style.underline === true)
          attrs |= nc.attrs.UNDERLINE;
        win.attrset(attrs);
      }
      /*win.on('inputChar', function(c, i) {
        clearInterval(self._tmr);
        win.close();
        //process.removeListener('SIGWINCH', fnDraw);
      });*/
      self._tmr = setInterval(fnAnimate, delay);
    }
    win.refresh();
  };
  fnAnimate = function() {
    if (dir === 'left') {
      if (idx++ === textlen)
        idx = 0;
      win.addstr(0, 0, text.substring(idx, idx+width));
    } else {
      if (idx-- === 0)
        idx = textlen-1;
      win.addstr(0, 0, text.substring(idx));
    }
    win.refresh();
  };
  fnDraw();
  //process.on('SIGWINCH', fnDraw);
};
exports.Marquee.prototype.stop = function() {
  if (this._tmr) {
    clearInterval(this._tmr);
    this.win.close();
    restoreColors();
    this._tmr = undefined;
  }
};

function parseColor(color, def) {
  var ret = undefined;

  if (typeof color === 'string') {
    if (typeof nc.colors[color.toUpperCase()] !== 'undefined')
      ret = nc.colors[color.toUpperCase()];
    else {
      try {
        ret = parseInt(color);
      } catch (err) {}
    }
  } else if (typeof color === 'number')
    ret = color;
  else if (typeof def !== 'undefined')
    ret = def;

  return ret;
}

function parsePosition(height, width, pos) {
  var boxY = 0, boxX = 0;
  if (typeof pos === 'string') {
    if (pos === 'top' || pos === 'topleft' || pos === 'topright')
      boxY = 0;
    else if (pos === 'bottom' || pos === 'bottomleft' || pos === 'bottomright')
      boxY = nc.lines-height;
    else
      boxY = Math.floor(nc.lines/2)-Math.floor(height/2);

    if (pos === 'left' || pos === 'topleft' || pos === 'bottomleft')
      boxX = 0;
    else if (pos === 'right' || pos === 'topright' || pos === 'bottomright')
      boxX = nc.cols-width;
    else
      boxX = Math.floor(nc.cols/2)-Math.floor(width/2);
  } else if (Array.isArray(pos)) {
    boxY = pos[0];
    boxX = pos[1];
  } else if (typeof pos === 'number')
    boxY = boxX = Math.floor(pos);
  return [boxY, boxX];
}

/**
 * Adopted from jquery's extend method. Under the terms of MIT License.
 *
 * http://code.jquery.com/jquery-1.4.2.js
 *
 * Modified by Brian White to use Array.isArray instead of the custom isArray method
 */
function extend() {
  // copy reference to target object
  var target = arguments[0] || {}, i = 1, length = arguments.length, deep = false, options, name, src, copy;
  // Handle a deep copy situation
  if (typeof target === "boolean") {
    deep = target;
    target = arguments[1] || {};
    // skip the boolean and the target
    i = 2;
  }
  // Handle case when target is a string or something (possible in deep copy)
  if (typeof target !== "object" && !typeof target === 'function')
    target = {};
  var isPlainObject = function(obj) {
    // Must be an Object.
    // Because of IE, we also have to check the presence of the constructor property.
    // Make sure that DOM nodes and window objects don't pass through, as well
    if (!obj || toString.call(obj) !== "[object Object]" || obj.nodeType || obj.setInterval)
      return false;
    var has_own_constructor = hasOwnProperty.call(obj, "constructor");
    var has_is_property_of_method = hasOwnProperty.call(obj.constructor.prototype, "isPrototypeOf");
    // Not own constructor property must be Object
    if (obj.constructor && !has_own_constructor && !has_is_property_of_method)
      return false;
    // Own properties are enumerated firstly, so to speed up,
    // if last one is own, then all properties are own.
    var last_key;
    for (key in obj)
      last_key = key;
    return typeof last_key === "undefined" || hasOwnProperty.call(obj, last_key);
  };
  for (; i < length; i++) {
    // Only deal with non-null/undefined values
    if ((options = arguments[i]) !== null) {
      // Extend the base object
      for (name in options) {
        src = target[name];
        copy = options[name];
        // Prevent never-ending loop
        if (target === copy)
            continue;
        // Recurse if we're merging object literal values or arrays
        if (deep && copy && (isPlainObject(copy) || Array.isArray(copy))) {
          var clone = src && (isPlainObject(src) || Array.isArray(src)) ? src : Array.isArray(copy) ? [] : {};
          // Never move original objects, clone them
          target[name] = extend(deep, clone, copy);
        // Don't bring in undefined values
        } else if (typeof copy !== "undefined")
          target[name] = copy;
      }
    }
  }
  // Return the modified object
  return target;
};
