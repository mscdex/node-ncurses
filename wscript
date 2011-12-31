import Options
from os import unlink, symlink
from os.path import exists, abspath
import os

srcdir = '.'
blddir = 'build'
ncursesdir = abspath(srcdir) + "/deps/ncurses"
VERSION = '0.0.1'

def set_options(opt):
  opt.tool_options('compiler_cxx')

def configure(conf):
  conf.check_tool('compiler_cxx')
  conf.check_tool('node_addon')

  # ugly hack to append -fPIC for x64
  hack = ""
  if (conf.env['DEST_CPU'] == 'x86_64'):
    hack = "sed -i 's/^CFLAGS_NORMAL.*$/CFLAGS_NORMAL\t= $(CCFLAGS) -fPIC/' " + ncursesdir + "/c++/Makefile && sed -i 's/^CFLAGS_NORMAL.*$/CFLAGS_NORMAL\t= $(CCFLAGS) -fPIC/' " + ncursesdir + "/ncurses/Makefile && sed -i 's/^CFLAGS_NORMAL.*$/CFLAGS_NORMAL\t= $(CCFLAGS) -fPIC/' " + ncursesdir + "/panel/Makefile"

  # configure ncurses
  print "Configuring ncurses library ..."
  cmd = "cd deps/ncurses && sh configure --without-debug --without-tests --without-progs --without-ada --without-manpages --enable-widec --enable-ext-colors"
  if os.system(cmd) != 0:
    conf.fatal("Configuring ncurses failed.")
  else:
    os.system(hack)

def build(bld):
  print "Building ncurses library ..."
  cmd = "cd deps/ncurses && make"
  if os.system(cmd) != 0:
    conf.fatal("Building ncurses failed.")
  else:
    obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
    obj.target = 'ncurses_addon'
    obj.source = 'ncurses.cc'
    obj.includes = [ncursesdir + '/include', ncursesdir + '/c++']
    obj.cxxflags = ['-O2']
    obj.linkflags = [ncursesdir + '/lib/libncurses++w.a', ncursesdir + '/lib/libpanelw.a', ncursesdir + '/lib/libncursesw.a']

def shutdown():
  # HACK to get ncurses_addon.node out of build directory.
  # better way to do this?
  if Options.commands['clean']:
    if exists('ncurses_addon.node'): unlink('ncurses_addon.node')
  else:
    if exists('build/Release/ncurses_addon.node') and not exists('ncurses_addon.node'):
      symlink('build/Release/ncurses_addon.node', 'ncurses_addon.node')
    if exists('build/default/ncurses_addon.node') and not exists('ncurses_addon.node'):
      symlink('build/default/ncurses_addon.node', 'ncurses_addon.node')
