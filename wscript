import Options
from os import unlink, symlink, popen
from os.path import exists

srcdir = '.'
blddir = 'build'
VERSION = '0.0.1'

def set_options(opt):
  opt.tool_options('compiler_cxx')

def configure(conf):
  conf.check_tool('compiler_cxx')
  conf.check_tool('node_addon')

  conf.env.append_value("LIB_NCURSES", "ncurses")
  conf.env.append_value("LIB_NCURSESPP", "ncurses++")
  conf.env.append_value("LIB_PANEL", "panel")

def build(bld):
  obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
  obj.target = 'ncurses'
  obj.source = 'ncurses.cc'
  obj.uselib = 'ncursespp panel ncurses'

def shutdown():
  # HACK to get ncurses.node out of build directory.
  # better way to do this?
  if Options.commands['clean']:
    if exists('ncurses.node'): unlink('ncurses.node')
  else:
    if exists('build/default/ncurses.node') and not exists('ncurses.node'):
      symlink('build/default/ncurses.node', 'ncurses.node')
