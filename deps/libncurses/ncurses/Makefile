# $Id: Makefile.in,v 1.133 2012/08/11 21:31:56 tom Exp $
##############################################################################
# Copyright (c) 1998-2011,2012 Free Software Foundation, Inc.                #
#                                                                            #
# Permission is hereby granted, free of charge, to any person obtaining a    #
# copy of this software and associated documentation files (the "Software"), #
# to deal in the Software without restriction, including without limitation  #
# the rights to use, copy, modify, merge, publish, distribute, distribute    #
# with modifications, sublicense, and/or sell copies of the Software, and to #
# permit persons to whom the Software is furnished to do so, subject to the  #
# following conditions:                                                      #
#                                                                            #
# The above copyright notice and this permission notice shall be included in #
# all copies or substantial portions of the Software.                        #
#                                                                            #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    #
# THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        #
# DEALINGS IN THE SOFTWARE.                                                  #
#                                                                            #
# Except as contained in this notice, the name(s) of the above copyright     #
# holders shall not be used in advertising or otherwise to promote the sale, #
# use or other dealings in this Software without prior written               #
# authorization.                                                             #
##############################################################################
#
# Author: Thomas E. Dickey 1996-on
#
# Makefile for ncurses source code.
#
# This makes the following:
#	programs
#	includes
#	libraries (normal/debug/profile/shared)
#
# The variable 'srcdir' refers to the source-distribution, and can be set with
# the configure script by "--srcdir=DIR".
#
# The rules are organized to produce the libraries for the configured models,
# and the programs with the configured default model.

# turn off _all_ suffix rules; we'll generate our own
.SUFFIXES:

SHELL		= /bin/sh

THIS		= Makefile

CF_MFLAGS 	= 


x		= 
o		= .o

MODEL		= normal
DESTDIR		= 
top_srcdir	= ..
srcdir		= .
prefix		= /usr
exec_prefix	= ${prefix}
bindir		= ${exec_prefix}/bin
libdir		= ${exec_prefix}/lib
includedir	= ${prefix}/include
datarootdir	= ${prefix}/share
datadir		= ${datarootdir}

LIBTOOL		= 
LIBTOOL_CLEAN	= 
LIBTOOL_COMPILE	= 
LIBTOOL_LINK	= ${CC}
LIBTOOL_INSTALL	= 
LIBTOOL_UNINSTALL = 
LT_UNDEF	= 

INSTALL		= /usr/bin/install -c
INSTALL_LIB	= /usr/bin/install -c -m 644
INSTALL_PROG	= ${INSTALL}
INSTALL_DATA	= ${INSTALL} -m 644

AR		= ar
ARFLAGS		= -curv
AWK		= mawk
LD		= ld
LN_S		= ln -s -f

CTAGS		= 
ETAGS		= 

CC		= gcc
CPP		= gcc -E
CFLAGS		= -O2 --param max-inline-insns-single=1200

INCDIR		= $(srcdir)/../include
CPPFLAGS	= -DHAVE_CONFIG_H -I../ncurses -I$(srcdir)  -D_GNU_SOURCE -DNDEBUG -I. -I../include

CCFLAGS		= $(CPPFLAGS) $(CFLAGS)

BUILD_CPPFLAGS	= -I../include -DUSE_BUILD_CC ${CPPFLAGS}
BUILD_CC	= ${CC}
BUILD_CCFLAGS	= -DHAVE_CONFIG_H -I../ncurses -I$(srcdir) -I$(INCDIR) $(BUILD_CPPFLAGS) ${CFLAGS}
BUILD_LDFLAGS	= ${LDFLAGS}
BUILD_LIBS	= ${LIBS}

# The executables built in this directory are used for generating source that
# is compiled into the build, or are test-programs that are not installed.

BUILD_EXEEXT	= $x
x		= 

CFLAGS_LIBTOOL	= $(CCFLAGS)
CFLAGS_NORMAL	= $(CCFLAGS)
CFLAGS_DEBUG	= $(CCFLAGS) -g -DTRACE
CFLAGS_PROFILE	= $(CCFLAGS) -pg
CFLAGS_SHARED	= $(CCFLAGS) -fPIC

CFLAGS_DEFAULT	= $(CFLAGS_NORMAL)

LINK		= $(LIBTOOL)
LDFLAGS		=   

SHLIB_DIRS	= -L../lib
SHLIB_LIST	= $(SHLIB_DIRS) 
TINFO_LIST	= $(SHLIB_DIRS) 
TICS_LIST	= $(SHLIB_DIRS) 

RPATH_LIST	= ${libdir}
MK_SHARED_LIB	= ${CC} ${CFLAGS} -shared -Wl,-soname,`basename $@ .${REL_VERSION}`.${ABI_VERSION},-stats,-lc -o $@

NCURSES_MAJOR	= 5
NCURSES_MINOR	= 9
REL_VERSION	= 6.0
ABI_VERSION	= 6

RANLIB		= ranlib

LIBRARIES	=  ../lib/libncursesw.a

LINT		= 
LINT_OPTS	= 
LINT_LIBS	= -lncurses 

FALLBACK_LIST	= 

USE_BIG_STRINGS	= 1
TERMINFO_CAPS	= $(top_srcdir)/include/Caps

TERMINFO	= /usr/share/terminfo
TERMINFO_SRC	= ${top_srcdir}/misc/terminfo.src
TIC_PATH	= /usr/bin/tic

AUTO_SRC = \
	./codes.c \
	./comp_captab.c \
	./expanded.c \
	./fallback.c \
	./lib_gen.c \
	./lib_keyname.c \
	./link_test.c \
	./names.c \
	./unctrl.c \
	init_keytry.h \
	keys.list

HEADER_DEPS	= \
	$(srcdir)/curses.priv.h \
	../include/ncurses_dll.h \
	../include/ncurses_cfg.h \
	../include/curses.h \
	$(INCDIR)/nc_panel.h \
	../include/term.h \
	$(INCDIR)/term_entry.h \
	$(INCDIR)/nc_tparm.h \
	$(INCDIR)/nc_alloc.h \
	$(INCDIR)/nc_mingw.h

TEST_DEPS	= ../lib/libncursesw.a
TEST_ARGS	=  -L../lib -lformw -lmenuw -lpanelw -lncursesw   
TEST_LDFLAGS	=  $(TEST_ARGS)   

TEST_PROGS = \
	captoinfo$x \
	hardscroll$x \
	link_test$x \
	hashmap$x \
	lib_mvcur$x

base	 = $(srcdir)/base
serial	 = $(srcdir)/tty
tinfo	 = $(srcdir)/tinfo
trace	 = $(srcdir)/trace
wide	 = $(srcdir)/widechar
win32con = $(srcdir)/win32con

################################################################################
all \
libs ::		$(AUTO_SRC) ../lib $(LIBRARIES)

sources:	$(AUTO_SRC)

$(DESTDIR)$(bindir) \
$(DESTDIR)$(libdir) :
	mkdir -p $@

../lib : ; mkdir $@

./fallback.c : $(tinfo)/MKfallback.sh
	sh -e $(tinfo)/MKfallback.sh $(TERMINFO) $(TERMINFO_SRC) $(TIC_PATH) $(FALLBACK_LIST) >$@

./lib_gen.c : $(base)/MKlib_gen.sh ../include/curses.h
	sh -e $(base)/MKlib_gen.sh "$(CPP) $(CPPFLAGS)" "$(AWK)" generated <../include/curses.h >$@

init_keytry.h: make_keys$(BUILD_EXEEXT) keys.list
	./make_keys$(BUILD_EXEEXT) keys.list > $@

keys.list :	$(tinfo)/MKkeys_list.sh
	AWK=$(AWK) sh $(tinfo)/MKkeys_list.sh $(TERMINFO_CAPS) | sort >$@

make_keys$(BUILD_EXEEXT) : \
		$(tinfo)/make_keys.c \
		names.c
	$(BUILD_CC) -o $@ $(BUILD_CCFLAGS) $(tinfo)/make_keys.c $(BUILD_LDFLAGS) $(BUILD_LIBS)

make_hash$(BUILD_EXEEXT) : \
		$(tinfo)/make_hash.c \
		../include/hashsize.h
	$(BUILD_CC) -o $@ $(BUILD_CCFLAGS) $(tinfo)/make_hash.c $(BUILD_LDFLAGS) $(BUILD_LIBS)

./expanded.c : $(srcdir)/curses.priv.h $(serial)/MKexpanded.sh
	sh -e $(serial)/MKexpanded.sh "$(CPP)" $(CPPFLAGS) > $@

./comp_captab.c: \
		make_hash$(BUILD_EXEEXT) \
		../include/hashsize.h \
		$(tinfo)/MKcaptab.sh \
		$(tinfo)/MKcaptab.awk
	sh -e $(tinfo)/MKcaptab.sh $(AWK) $(USE_BIG_STRINGS) $(tinfo)/MKcaptab.awk $(srcdir)/../include/Caps > $@

./lib_keyname.c: keys.list $(base)/MKkeyname.awk
	$(AWK) -f $(base)/MKkeyname.awk bigstrings=$(USE_BIG_STRINGS) keys.list > $@

./codes.c: $(tinfo)/MKcodes.awk
	$(AWK) -f $(tinfo)/MKcodes.awk bigstrings=$(USE_BIG_STRINGS) $(srcdir)/../include/Caps >$@

./names.c: $(tinfo)/MKnames.awk
	$(AWK) -f $(tinfo)/MKnames.awk bigstrings=$(USE_BIG_STRINGS) $(srcdir)/../include/Caps >$@

./unctrl.c: $(base)/MKunctrl.awk
	echo | $(AWK) -f $(base)/MKunctrl.awk bigstrings=$(USE_BIG_STRINGS) >$@

tags:
	$(CTAGS) *.[ch] */*.[ch]

#TAGS:
#	$(ETAGS) *.[ch] */*.[ch]

mostlyclean ::
	-rm -f core tags TAGS *~ *.bak *.i *.ln *.atac trace
	-rm -f $(TEST_PROGS)

clean :: mostlyclean
	-rm -f $(AUTO_SRC)
	-rm -f make_keys$(BUILD_EXEEXT)
	-rm -f make_hash$(BUILD_EXEEXT)
	-rm -rf .libs

distclean :: clean
	-rm -f Makefile

realclean :: distclean

# These rules are used to allow "make -n" to work on a clean directory-tree
../include/hashsize.h \
../include/parametrized.h \
../include/term.h :
	cd ../include; $(MAKE) $(CF_MFLAGS)

# These rules build test-programs for the modules that have test-drivers
test_progs : $(TEST_PROGS)

./link_test.c : $(base)/MKlib_gen.sh ../include/curses.h
	sh -e $(base)/MKlib_gen.sh "$(CPP) $(CPPFLAGS)" "$(AWK)" implemented <../include/curses.h >$@

captoinfo$x : $(tinfo)/captoinfo.c $(TEST_DEPS)
	 $(LIBTOOL_LINK) -o $@ $(CFLAGS_DEFAULT) -DMAIN $(tinfo)/captoinfo.c $(TEST_LDFLAGS)

hardscroll$x : $(serial)/hardscroll.c $(TEST_DEPS)
	 $(LIBTOOL_LINK) -o $@ $(CFLAGS_DEFAULT) -DSCROLLDEBUG $(serial)/hardscroll.c $(TEST_LDFLAGS)

hashmap$x : $(serial)/hashmap.c $(serial)/hardscroll.c $(TEST_DEPS)
	 $(LIBTOOL_LINK) -o $@ $(CFLAGS_DEFAULT) -DHASHDEBUG $(serial)/hashmap.c $(serial)/hardscroll.c $(TEST_LDFLAGS)

lib_mvcur$x : $(serial)/lib_mvcur.c $(TEST_DEPS) \
		../objects/dump_entry$o
	 $(LIBTOOL_LINK) -o $@ $(CFLAGS_DEFAULT) -DNCURSES_TEST -I$(serial)/../../progs $(serial)/lib_mvcur.c ../objects/dump_entry$o $(TEST_LDFLAGS)

link_test$x : ./link_test.c $(TEST_DEPS) \
		../objects/link_test$o
	 $(CC) -o $@ $(CFLAGS_DEFAULT) ../objects/link_test$o $(TEST_LDFLAGS)

../objects/dump_entry$o:
	cd ../progs && $(MAKE) ../objects/dump_entry$o

###############################################################################
# The remainder of this file is automatically generated during configuration
###############################################################################

# generated by mk-0th.awk
#   libname:    ncursesw
#   subsets:    ticlib+termlib+ext_tinfo+base+widechar+ext_funcs
#   ticlib:     
#   termlib:    

.SUFFIXES: .c .cc .h .i .ii
.c.i :
	$(CPP) $(CPPFLAGS) $< >$@
.cc.ii :
	$(CPP) $(CPPFLAGS) $< >$@
.h.i :
	$(CPP) $(CPPFLAGS) $< >$@

C_SRC = \
	$(serial)/hardscroll.c \
	$(serial)/hashmap.c \
	$(base)/lib_addch.c \
	$(base)/lib_addstr.c \
	$(base)/lib_beep.c \
	$(base)/lib_bkgd.c \
	$(base)/lib_box.c \
	$(base)/lib_chgat.c \
	$(base)/lib_clear.c \
	$(base)/lib_clearok.c \
	$(base)/lib_clrbot.c \
	$(base)/lib_clreol.c \
	$(base)/lib_color.c \
	$(base)/lib_colorset.c \
	$(base)/lib_delch.c \
	$(base)/lib_delwin.c \
	$(base)/lib_echo.c \
	$(base)/lib_endwin.c \
	$(base)/lib_erase.c \
	$(base)/lib_flash.c \
	./lib_gen.c \
	$(base)/lib_getch.c \
	$(base)/lib_getstr.c \
	$(base)/lib_hline.c \
	$(base)/lib_immedok.c \
	$(base)/lib_inchstr.c \
	$(base)/lib_initscr.c \
	$(base)/lib_insch.c \
	$(base)/lib_insdel.c \
	$(base)/lib_insnstr.c \
	$(base)/lib_instr.c \
	$(base)/lib_isendwin.c \
	$(base)/lib_leaveok.c \
	$(base)/lib_mouse.c \
	$(base)/lib_move.c \
	$(serial)/lib_mvcur.c \
	$(base)/lib_mvwin.c \
	$(base)/lib_newterm.c \
	$(base)/lib_newwin.c \
	$(base)/lib_nl.c \
	$(base)/lib_overlay.c \
	$(base)/lib_pad.c \
	$(base)/lib_printw.c \
	$(base)/lib_redrawln.c \
	$(base)/lib_refresh.c \
	$(base)/lib_restart.c \
	$(base)/lib_scanw.c \
	$(base)/lib_screen.c \
	$(base)/lib_scroll.c \
	$(base)/lib_scrollok.c \
	$(base)/lib_scrreg.c \
	$(base)/lib_set_term.c \
	$(base)/lib_slk.c \
	$(base)/lib_slkatr_set.c \
	$(base)/lib_slkatrof.c \
	$(base)/lib_slkatron.c \
	$(base)/lib_slkatrset.c \
	$(base)/lib_slkattr.c \
	$(base)/lib_slkclear.c \
	$(base)/lib_slkcolor.c \
	$(base)/lib_slkinit.c \
	$(base)/lib_slklab.c \
	$(base)/lib_slkrefr.c \
	$(base)/lib_slkset.c \
	$(base)/lib_slktouch.c \
	$(base)/lib_touch.c \
	$(trace)/lib_tracedmp.c \
	$(trace)/lib_tracemse.c \
	$(serial)/lib_tstp.c \
	$(base)/lib_ungetch.c \
	$(serial)/lib_vidattr.c \
	$(base)/lib_vline.c \
	$(base)/lib_wattroff.c \
	$(base)/lib_wattron.c \
	$(base)/lib_winch.c \
	$(base)/lib_window.c \
	$(base)/nc_panel.c \
	$(base)/safe_sprintf.c \
	$(serial)/tty_update.c \
	$(trace)/varargs.c \
	$(base)/memmove.c \
	$(base)/vsscanf.c \
	$(base)/lib_freeall.c \
	$(wide)/charable.c \
	$(wide)/lib_add_wch.c \
	$(wide)/lib_box_set.c \
	$(wide)/lib_cchar.c \
	$(wide)/lib_erasewchar.c \
	$(wide)/lib_get_wch.c \
	$(wide)/lib_get_wstr.c \
	$(wide)/lib_hline_set.c \
	$(wide)/lib_in_wch.c \
	$(wide)/lib_in_wchnstr.c \
	$(wide)/lib_ins_wch.c \
	$(wide)/lib_inwstr.c \
	$(wide)/lib_key_name.c \
	$(wide)/lib_pecho_wchar.c \
	$(wide)/lib_slk_wset.c \
	$(wide)/lib_unget_wch.c \
	$(wide)/lib_vid_attr.c \
	$(wide)/lib_vline_set.c \
	$(wide)/lib_wacs.c \
	$(wide)/lib_wunctrl.c \
	./expanded.c \
	$(base)/legacy_coding.c \
	$(base)/lib_dft_fgbg.c \
	$(tinfo)/lib_print.c \
	$(base)/resizeterm.c \
	$(trace)/trace_xnames.c \
	$(tinfo)/use_screen.c \
	$(base)/use_window.c \
	$(base)/wresize.c \
	$(tinfo)/access.c \
	$(tinfo)/add_tries.c \
	$(tinfo)/alloc_ttype.c \
	./codes.c \
	./comp_captab.c \
	$(tinfo)/comp_error.c \
	$(tinfo)/comp_hash.c \
	$(tinfo)/db_iterator.c \
	$(tinfo)/doalloc.c \
	$(tinfo)/entries.c \
	./fallback.c \
	$(tinfo)/free_ttype.c \
	$(tinfo)/getenv_num.c \
	$(tinfo)/home_terminfo.c \
	$(tinfo)/init_keytry.c \
	$(tinfo)/lib_acs.c \
	$(tinfo)/lib_baudrate.c \
	$(tinfo)/lib_cur_term.c \
	$(tinfo)/lib_data.c \
	$(tinfo)/lib_has_cap.c \
	$(tinfo)/lib_kernel.c \
	./lib_keyname.c \
	$(tinfo)/lib_longname.c \
	$(tinfo)/lib_napms.c \
	$(tinfo)/lib_options.c \
	$(tinfo)/lib_raw.c \
	$(tinfo)/lib_setup.c \
	$(tinfo)/lib_termcap.c \
	$(tinfo)/lib_termname.c \
	$(tinfo)/lib_tgoto.c \
	$(tinfo)/lib_ti.c \
	$(tinfo)/lib_tparm.c \
	$(tinfo)/lib_tputs.c \
	$(trace)/lib_trace.c \
	$(trace)/lib_traceatr.c \
	$(trace)/lib_tracebits.c \
	$(trace)/lib_tracechr.c \
	$(tinfo)/lib_ttyflags.c \
	$(serial)/lib_twait.c \
	$(tinfo)/name_match.c \
	./names.c \
	$(tinfo)/read_entry.c \
	$(tinfo)/read_termcap.c \
	$(tinfo)/setbuf.c \
	$(tinfo)/strings.c \
	$(trace)/trace_buf.c \
	$(trace)/trace_tries.c \
	$(base)/tries.c \
	$(tinfo)/trim_sgr0.c \
	./unctrl.c \
	$(trace)/visbuf.c \
	$(tinfo)/alloc_entry.c \
	$(tinfo)/captoinfo.c \
	$(tinfo)/comp_expand.c \
	$(tinfo)/comp_parse.c \
	$(tinfo)/comp_scan.c \
	$(tinfo)/parse_entry.c \
	$(tinfo)/write_entry.c \
	$(base)/define_key.c \
	$(tinfo)/hashed_db.c \
	$(base)/key_defined.c \
	$(base)/keybound.c \
	$(base)/keyok.c \
	$(base)/version.c

# Producing llib-lncursesw is time-consuming, so there's no direct-dependency for
# it in the lintlib rule.  We'll only remove in the cleanest setup.

NCURSESW_SRC = \
	$(serial)/hardscroll.c \
	$(serial)/hashmap.c \
	$(base)/lib_addch.c \
	$(base)/lib_addstr.c \
	$(base)/lib_beep.c \
	$(base)/lib_bkgd.c \
	$(base)/lib_box.c \
	$(base)/lib_chgat.c \
	$(base)/lib_clear.c \
	$(base)/lib_clearok.c \
	$(base)/lib_clrbot.c \
	$(base)/lib_clreol.c \
	$(base)/lib_color.c \
	$(base)/lib_colorset.c \
	$(base)/lib_delch.c \
	$(base)/lib_delwin.c \
	$(base)/lib_echo.c \
	$(base)/lib_endwin.c \
	$(base)/lib_erase.c \
	$(base)/lib_flash.c \
	./lib_gen.c \
	$(base)/lib_getch.c \
	$(base)/lib_getstr.c \
	$(base)/lib_hline.c \
	$(base)/lib_immedok.c \
	$(base)/lib_inchstr.c \
	$(base)/lib_initscr.c \
	$(base)/lib_insch.c \
	$(base)/lib_insdel.c \
	$(base)/lib_insnstr.c \
	$(base)/lib_instr.c \
	$(base)/lib_isendwin.c \
	$(base)/lib_leaveok.c \
	$(base)/lib_mouse.c \
	$(base)/lib_move.c \
	$(serial)/lib_mvcur.c \
	$(base)/lib_mvwin.c \
	$(base)/lib_newterm.c \
	$(base)/lib_newwin.c \
	$(base)/lib_nl.c \
	$(base)/lib_overlay.c \
	$(base)/lib_pad.c \
	$(base)/lib_printw.c \
	$(base)/lib_redrawln.c \
	$(base)/lib_refresh.c \
	$(base)/lib_restart.c \
	$(base)/lib_scanw.c \
	$(base)/lib_screen.c \
	$(base)/lib_scroll.c \
	$(base)/lib_scrollok.c \
	$(base)/lib_scrreg.c \
	$(base)/lib_set_term.c \
	$(base)/lib_slk.c \
	$(base)/lib_slkatr_set.c \
	$(base)/lib_slkatrof.c \
	$(base)/lib_slkatron.c \
	$(base)/lib_slkatrset.c \
	$(base)/lib_slkattr.c \
	$(base)/lib_slkclear.c \
	$(base)/lib_slkcolor.c \
	$(base)/lib_slkinit.c \
	$(base)/lib_slklab.c \
	$(base)/lib_slkrefr.c \
	$(base)/lib_slkset.c \
	$(base)/lib_slktouch.c \
	$(base)/lib_touch.c \
	$(trace)/lib_tracedmp.c \
	$(trace)/lib_tracemse.c \
	$(serial)/lib_tstp.c \
	$(base)/lib_ungetch.c \
	$(serial)/lib_vidattr.c \
	$(base)/lib_vline.c \
	$(base)/lib_wattroff.c \
	$(base)/lib_wattron.c \
	$(base)/lib_winch.c \
	$(base)/lib_window.c \
	$(base)/nc_panel.c \
	$(base)/safe_sprintf.c \
	$(serial)/tty_update.c \
	$(trace)/varargs.c \
	$(base)/memmove.c \
	$(base)/vsscanf.c \
	$(base)/lib_freeall.c \
	$(wide)/charable.c \
	$(wide)/lib_add_wch.c \
	$(wide)/lib_box_set.c \
	$(wide)/lib_cchar.c \
	$(wide)/lib_erasewchar.c \
	$(wide)/lib_get_wch.c \
	$(wide)/lib_get_wstr.c \
	$(wide)/lib_hline_set.c \
	$(wide)/lib_in_wch.c \
	$(wide)/lib_in_wchnstr.c \
	$(wide)/lib_ins_wch.c \
	$(wide)/lib_inwstr.c \
	$(wide)/lib_key_name.c \
	$(wide)/lib_pecho_wchar.c \
	$(wide)/lib_slk_wset.c \
	$(wide)/lib_unget_wch.c \
	$(wide)/lib_vid_attr.c \
	$(wide)/lib_vline_set.c \
	$(wide)/lib_wacs.c \
	$(wide)/lib_wunctrl.c \
	./expanded.c \
	$(base)/legacy_coding.c \
	$(base)/lib_dft_fgbg.c \
	$(tinfo)/lib_print.c \
	$(base)/resizeterm.c \
	$(trace)/trace_xnames.c \
	$(tinfo)/use_screen.c \
	$(base)/use_window.c \
	$(base)/wresize.c \
	$(tinfo)/access.c \
	$(tinfo)/add_tries.c \
	$(tinfo)/alloc_ttype.c \
	./codes.c \
	./comp_captab.c \
	$(tinfo)/comp_error.c \
	$(tinfo)/comp_hash.c \
	$(tinfo)/db_iterator.c \
	$(tinfo)/doalloc.c \
	$(tinfo)/entries.c \
	./fallback.c \
	$(tinfo)/free_ttype.c \
	$(tinfo)/getenv_num.c \
	$(tinfo)/home_terminfo.c \
	$(tinfo)/init_keytry.c \
	$(tinfo)/lib_acs.c \
	$(tinfo)/lib_baudrate.c \
	$(tinfo)/lib_cur_term.c \
	$(tinfo)/lib_data.c \
	$(tinfo)/lib_has_cap.c \
	$(tinfo)/lib_kernel.c \
	./lib_keyname.c \
	$(tinfo)/lib_longname.c \
	$(tinfo)/lib_napms.c \
	$(tinfo)/lib_options.c \
	$(tinfo)/lib_raw.c \
	$(tinfo)/lib_setup.c \
	$(tinfo)/lib_termcap.c \
	$(tinfo)/lib_termname.c \
	$(tinfo)/lib_tgoto.c \
	$(tinfo)/lib_ti.c \
	$(tinfo)/lib_tparm.c \
	$(tinfo)/lib_tputs.c \
	$(trace)/lib_trace.c \
	$(trace)/lib_traceatr.c \
	$(trace)/lib_tracebits.c \
	$(trace)/lib_tracechr.c \
	$(tinfo)/lib_ttyflags.c \
	$(serial)/lib_twait.c \
	$(tinfo)/name_match.c \
	./names.c \
	$(tinfo)/read_entry.c \
	$(tinfo)/read_termcap.c \
	$(tinfo)/setbuf.c \
	$(tinfo)/strings.c \
	$(trace)/trace_buf.c \
	$(trace)/trace_tries.c \
	$(base)/tries.c \
	$(tinfo)/trim_sgr0.c \
	./unctrl.c \
	$(trace)/visbuf.c \
	$(tinfo)/alloc_entry.c \
	$(tinfo)/captoinfo.c \
	$(tinfo)/comp_expand.c \
	$(tinfo)/comp_parse.c \
	$(tinfo)/comp_scan.c \
	$(tinfo)/parse_entry.c \
	$(tinfo)/write_entry.c \
	$(base)/define_key.c \
	$(tinfo)/hashed_db.c \
	$(base)/key_defined.c \
	$(base)/keybound.c \
	$(base)/keyok.c \
	$(base)/version.c

clean ::
	rm -f llib-lncursesw.*

realclean ::
	rm -f llib-lncursesw

llib-lncursesw : $(NCURSESW_SRC)
	cproto -a -l -DNCURSES_ENABLE_STDBOOL_H=0 -DLINT $(CPPFLAGS) $(NCURSESW_SRC) >$@

lintlib ::
	sh $(srcdir)/../misc/makellib ncursesw $(CPPFLAGS)

lint ::
	$(LINT) $(LINT_OPTS) $(CPPFLAGS) $(NCURSESW_SRC) $(LINT_LIBS)

# generated by mk-1st.awk (subset=ticlib+termlib+ext_tinfo+base+widechar+ext_funcs)
#  name:          ncursesw
#  traces:        DEBUG
#  MODEL:         NORMAL
#  model:         objects
#  prefix:        lib
#  suffix:        .a
#  subset:        ticlib+termlib+ext_tinfo+base+widechar+ext_funcs
#  driver:        no
#  ShlibVer:      rel
#  ShlibVerInfix: no
#  SymLink:       ln -s -f
#  TermlibRoot:   ncursesw
#  TermlibSuffix: .a
#  ReLink:        no
#  DoLinks:       yes
#  rmSoLocs:      no
#  ldconfig:      /sbin/ldconfig
#  overwrite:     yes
#  depend:        ../include/ncurses_cfg.h ./curses.priv.h
#  host:          x86_64-unknown-linux-gnu

NORMAL_OBJS = \
	../objects/hardscroll$o \
	../objects/hashmap$o \
	../objects/lib_addch$o \
	../objects/lib_addstr$o \
	../objects/lib_beep$o \
	../objects/lib_bkgd$o \
	../objects/lib_box$o \
	../objects/lib_chgat$o \
	../objects/lib_clear$o \
	../objects/lib_clearok$o \
	../objects/lib_clrbot$o \
	../objects/lib_clreol$o \
	../objects/lib_color$o \
	../objects/lib_colorset$o \
	../objects/lib_delch$o \
	../objects/lib_delwin$o \
	../objects/lib_echo$o \
	../objects/lib_endwin$o \
	../objects/lib_erase$o \
	../objects/lib_flash$o \
	../objects/lib_gen$o \
	../objects/lib_getch$o \
	../objects/lib_getstr$o \
	../objects/lib_hline$o \
	../objects/lib_immedok$o \
	../objects/lib_inchstr$o \
	../objects/lib_initscr$o \
	../objects/lib_insch$o \
	../objects/lib_insdel$o \
	../objects/lib_insnstr$o \
	../objects/lib_instr$o \
	../objects/lib_isendwin$o \
	../objects/lib_leaveok$o \
	../objects/lib_mouse$o \
	../objects/lib_move$o \
	../objects/lib_mvcur$o \
	../objects/lib_mvwin$o \
	../objects/lib_newterm$o \
	../objects/lib_newwin$o \
	../objects/lib_nl$o \
	../objects/lib_overlay$o \
	../objects/lib_pad$o \
	../objects/lib_printw$o \
	../objects/lib_redrawln$o \
	../objects/lib_refresh$o \
	../objects/lib_restart$o \
	../objects/lib_scanw$o \
	../objects/lib_screen$o \
	../objects/lib_scroll$o \
	../objects/lib_scrollok$o \
	../objects/lib_scrreg$o \
	../objects/lib_set_term$o \
	../objects/lib_slk$o \
	../objects/lib_slkatr_set$o \
	../objects/lib_slkatrof$o \
	../objects/lib_slkatron$o \
	../objects/lib_slkatrset$o \
	../objects/lib_slkattr$o \
	../objects/lib_slkclear$o \
	../objects/lib_slkcolor$o \
	../objects/lib_slkinit$o \
	../objects/lib_slklab$o \
	../objects/lib_slkrefr$o \
	../objects/lib_slkset$o \
	../objects/lib_slktouch$o \
	../objects/lib_touch$o \
	../objects/lib_tstp$o \
	../objects/lib_ungetch$o \
	../objects/lib_vidattr$o \
	../objects/lib_vline$o \
	../objects/lib_wattroff$o \
	../objects/lib_wattron$o \
	../objects/lib_winch$o \
	../objects/lib_window$o \
	../objects/nc_panel$o \
	../objects/safe_sprintf$o \
	../objects/tty_update$o \
	../objects/varargs$o \
	../objects/memmove$o \
	../objects/vsscanf$o \
	../objects/lib_freeall$o \
	../objects/charable$o \
	../objects/lib_add_wch$o \
	../objects/lib_box_set$o \
	../objects/lib_cchar$o \
	../objects/lib_erasewchar$o \
	../objects/lib_get_wch$o \
	../objects/lib_get_wstr$o \
	../objects/lib_hline_set$o \
	../objects/lib_in_wch$o \
	../objects/lib_in_wchnstr$o \
	../objects/lib_ins_wch$o \
	../objects/lib_inwstr$o \
	../objects/lib_key_name$o \
	../objects/lib_pecho_wchar$o \
	../objects/lib_slk_wset$o \
	../objects/lib_unget_wch$o \
	../objects/lib_vid_attr$o \
	../objects/lib_vline_set$o \
	../objects/lib_wacs$o \
	../objects/lib_wunctrl$o \
	../objects/expanded$o \
	../objects/legacy_coding$o \
	../objects/lib_dft_fgbg$o \
	../objects/lib_print$o \
	../objects/resizeterm$o \
	../objects/use_screen$o \
	../objects/use_window$o \
	../objects/wresize$o \
	../objects/access$o \
	../objects/add_tries$o \
	../objects/alloc_ttype$o \
	../objects/codes$o \
	../objects/comp_captab$o \
	../objects/comp_error$o \
	../objects/comp_hash$o \
	../objects/db_iterator$o \
	../objects/doalloc$o \
	../objects/entries$o \
	../objects/fallback$o \
	../objects/free_ttype$o \
	../objects/getenv_num$o \
	../objects/home_terminfo$o \
	../objects/init_keytry$o \
	../objects/lib_acs$o \
	../objects/lib_baudrate$o \
	../objects/lib_cur_term$o \
	../objects/lib_data$o \
	../objects/lib_has_cap$o \
	../objects/lib_kernel$o \
	../objects/lib_keyname$o \
	../objects/lib_longname$o \
	../objects/lib_napms$o \
	../objects/lib_options$o \
	../objects/lib_raw$o \
	../objects/lib_setup$o \
	../objects/lib_termcap$o \
	../objects/lib_termname$o \
	../objects/lib_tgoto$o \
	../objects/lib_ti$o \
	../objects/lib_tparm$o \
	../objects/lib_tputs$o \
	../objects/lib_trace$o \
	../objects/lib_ttyflags$o \
	../objects/lib_twait$o \
	../objects/name_match$o \
	../objects/names$o \
	../objects/read_entry$o \
	../objects/read_termcap$o \
	../objects/setbuf$o \
	../objects/strings$o \
	../objects/tries$o \
	../objects/trim_sgr0$o \
	../objects/unctrl$o \
	../objects/visbuf$o \
	../objects/alloc_entry$o \
	../objects/captoinfo$o \
	../objects/comp_expand$o \
	../objects/comp_parse$o \
	../objects/comp_scan$o \
	../objects/parse_entry$o \
	../objects/write_entry$o \
	../objects/define_key$o \
	../objects/hashed_db$o \
	../objects/key_defined$o \
	../objects/keybound$o \
	../objects/keyok$o \
	../objects/version$o

$(NORMAL_OBJS) : ../include/ncurses_cfg.h ./curses.priv.h

../lib/libncursesw.a : $(NORMAL_OBJS)
	$(AR) $(ARFLAGS) $@ $?
	$(RANLIB) $@

install \
install.libs \
install.ncursesw :: $(DESTDIR)$(libdir) ../lib/libncursesw.a
	@echo installing ../lib/libncursesw.a as $(DESTDIR)$(libdir)/libncursesw.a
	$(INSTALL_DATA) ../lib/libncursesw.a $(DESTDIR)$(libdir)/libncursesw.a
	$(RANLIB) $(DESTDIR)$(libdir)/libncursesw.a

uninstall \
uninstall.libs \
uninstall.ncursesw ::
	@echo uninstalling $(DESTDIR)$(libdir)/libncursesw.a
	-@rm -f $(DESTDIR)$(libdir)/libncursesw.a

clean ::
	-rm -f ../lib/libncursesw.a
	-rm -f ../lib/
	-rm -f ../lib/libncursesw.a

mostlyclean::
	-rm -f $(NORMAL_OBJS)

# generated by mk-2nd.awk
#   model:      objects
#   MODEL:      NORMAL
#   echo:       yes
#   subset:     ticlib+termlib+ext_tinfo+base+widechar+ext_funcs
#   crenames:   yes
#   cxxrenames: yes
#   traces:     DEBUG
#   srcdir:     .

../objects/hardscroll$o :	$(serial)/hardscroll.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/hardscroll.c -o ../objects/hardscroll$o

../objects/hashmap$o :	$(serial)/hashmap.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/hashmap.c -o ../objects/hashmap$o

../objects/lib_addch$o :	$(base)/lib_addch.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_addch.c -o ../objects/lib_addch$o

../objects/lib_addstr$o :	$(base)/lib_addstr.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_addstr.c -o ../objects/lib_addstr$o

../objects/lib_beep$o :	$(base)/lib_beep.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_beep.c -o ../objects/lib_beep$o

../objects/lib_bkgd$o :	$(base)/lib_bkgd.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_bkgd.c -o ../objects/lib_bkgd$o

../objects/lib_box$o :	$(base)/lib_box.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_box.c -o ../objects/lib_box$o

../objects/lib_chgat$o :	$(base)/lib_chgat.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_chgat.c -o ../objects/lib_chgat$o

../objects/lib_clear$o :	$(base)/lib_clear.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_clear.c -o ../objects/lib_clear$o

../objects/lib_clearok$o :	$(base)/lib_clearok.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_clearok.c -o ../objects/lib_clearok$o

../objects/lib_clrbot$o :	$(base)/lib_clrbot.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_clrbot.c -o ../objects/lib_clrbot$o

../objects/lib_clreol$o :	$(base)/lib_clreol.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_clreol.c -o ../objects/lib_clreol$o

../objects/lib_color$o :	$(base)/lib_color.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_color.c -o ../objects/lib_color$o

../objects/lib_colorset$o :	$(base)/lib_colorset.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_colorset.c -o ../objects/lib_colorset$o

../objects/lib_delch$o :	$(base)/lib_delch.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_delch.c -o ../objects/lib_delch$o

../objects/lib_delwin$o :	$(base)/lib_delwin.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_delwin.c -o ../objects/lib_delwin$o

../objects/lib_echo$o :	$(base)/lib_echo.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_echo.c -o ../objects/lib_echo$o

../objects/lib_endwin$o :	$(base)/lib_endwin.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_endwin.c -o ../objects/lib_endwin$o

../objects/lib_erase$o :	$(base)/lib_erase.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_erase.c -o ../objects/lib_erase$o

../objects/lib_flash$o :	$(base)/lib_flash.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_flash.c -o ../objects/lib_flash$o

../objects/lib_gen$o :	./lib_gen.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/lib_gen.c -o ../objects/lib_gen$o

../objects/lib_getch$o :	$(base)/lib_getch.c \
			$(HEADER_DEPS) \
			$(srcdir)/fifo_defs.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_getch.c -o ../objects/lib_getch$o

../objects/lib_getstr$o :	$(base)/lib_getstr.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_getstr.c -o ../objects/lib_getstr$o

../objects/lib_hline$o :	$(base)/lib_hline.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_hline.c -o ../objects/lib_hline$o

../objects/lib_immedok$o :	$(base)/lib_immedok.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_immedok.c -o ../objects/lib_immedok$o

../objects/lib_inchstr$o :	$(base)/lib_inchstr.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_inchstr.c -o ../objects/lib_inchstr$o

../objects/lib_initscr$o :	$(base)/lib_initscr.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_initscr.c -o ../objects/lib_initscr$o

../objects/lib_insch$o :	$(base)/lib_insch.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_insch.c -o ../objects/lib_insch$o

../objects/lib_insdel$o :	$(base)/lib_insdel.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_insdel.c -o ../objects/lib_insdel$o

../objects/lib_insnstr$o :	$(base)/lib_insnstr.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_insnstr.c -o ../objects/lib_insnstr$o

../objects/lib_instr$o :	$(base)/lib_instr.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_instr.c -o ../objects/lib_instr$o

../objects/lib_isendwin$o :	$(base)/lib_isendwin.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_isendwin.c -o ../objects/lib_isendwin$o

../objects/lib_leaveok$o :	$(base)/lib_leaveok.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_leaveok.c -o ../objects/lib_leaveok$o

../objects/lib_mouse$o :	$(base)/lib_mouse.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_mouse.c -o ../objects/lib_mouse$o

../objects/lib_move$o :	$(base)/lib_move.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_move.c -o ../objects/lib_move$o

../objects/lib_mvcur$o :	$(serial)/lib_mvcur.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h \
			$(srcdir)/../progs/dump_entry.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/lib_mvcur.c -o ../objects/lib_mvcur$o

../objects/lib_mvwin$o :	$(base)/lib_mvwin.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_mvwin.c -o ../objects/lib_mvwin$o

../objects/lib_newterm$o :	$(base)/lib_newterm.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_newterm.c -o ../objects/lib_newterm$o

../objects/lib_newwin$o :	$(base)/lib_newwin.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_newwin.c -o ../objects/lib_newwin$o

../objects/lib_nl$o :	$(base)/lib_nl.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_nl.c -o ../objects/lib_nl$o

../objects/lib_overlay$o :	$(base)/lib_overlay.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_overlay.c -o ../objects/lib_overlay$o

../objects/lib_pad$o :	$(base)/lib_pad.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_pad.c -o ../objects/lib_pad$o

../objects/lib_printw$o :	$(base)/lib_printw.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_printw.c -o ../objects/lib_printw$o

../objects/lib_redrawln$o :	$(base)/lib_redrawln.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_redrawln.c -o ../objects/lib_redrawln$o

../objects/lib_refresh$o :	$(base)/lib_refresh.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_refresh.c -o ../objects/lib_refresh$o

../objects/lib_restart$o :	$(base)/lib_restart.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_restart.c -o ../objects/lib_restart$o

../objects/lib_scanw$o :	$(base)/lib_scanw.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_scanw.c -o ../objects/lib_scanw$o

../objects/lib_screen$o :	$(base)/lib_screen.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_screen.c -o ../objects/lib_screen$o

../objects/lib_scroll$o :	$(base)/lib_scroll.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_scroll.c -o ../objects/lib_scroll$o

../objects/lib_scrollok$o :	$(base)/lib_scrollok.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_scrollok.c -o ../objects/lib_scrollok$o

../objects/lib_scrreg$o :	$(base)/lib_scrreg.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_scrreg.c -o ../objects/lib_scrreg$o

../objects/lib_set_term$o :	$(base)/lib_set_term.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_set_term.c -o ../objects/lib_set_term$o

../objects/lib_slk$o :	$(base)/lib_slk.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slk.c -o ../objects/lib_slk$o

../objects/lib_slkatr_set$o :	$(base)/lib_slkatr_set.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkatr_set.c -o ../objects/lib_slkatr_set$o

../objects/lib_slkatrof$o :	$(base)/lib_slkatrof.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkatrof.c -o ../objects/lib_slkatrof$o

../objects/lib_slkatron$o :	$(base)/lib_slkatron.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkatron.c -o ../objects/lib_slkatron$o

../objects/lib_slkatrset$o :	$(base)/lib_slkatrset.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkatrset.c -o ../objects/lib_slkatrset$o

../objects/lib_slkattr$o :	$(base)/lib_slkattr.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkattr.c -o ../objects/lib_slkattr$o

../objects/lib_slkclear$o :	$(base)/lib_slkclear.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkclear.c -o ../objects/lib_slkclear$o

../objects/lib_slkcolor$o :	$(base)/lib_slkcolor.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkcolor.c -o ../objects/lib_slkcolor$o

../objects/lib_slkinit$o :	$(base)/lib_slkinit.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkinit.c -o ../objects/lib_slkinit$o

../objects/lib_slklab$o :	$(base)/lib_slklab.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slklab.c -o ../objects/lib_slklab$o

../objects/lib_slkrefr$o :	$(base)/lib_slkrefr.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkrefr.c -o ../objects/lib_slkrefr$o

../objects/lib_slkset$o :	$(base)/lib_slkset.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkset.c -o ../objects/lib_slkset$o

../objects/lib_slktouch$o :	$(base)/lib_slktouch.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slktouch.c -o ../objects/lib_slktouch$o

../objects/lib_touch$o :	$(base)/lib_touch.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_touch.c -o ../objects/lib_touch$o

../objects/lib_tstp$o :	$(serial)/lib_tstp.c \
			$(HEADER_DEPS) \
			$(srcdir)/SigAction.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/lib_tstp.c -o ../objects/lib_tstp$o

../objects/lib_ungetch$o :	$(base)/lib_ungetch.c \
			$(HEADER_DEPS) \
			$(srcdir)/fifo_defs.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_ungetch.c -o ../objects/lib_ungetch$o

../objects/lib_vidattr$o :	$(serial)/lib_vidattr.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/lib_vidattr.c -o ../objects/lib_vidattr$o

../objects/lib_vline$o :	$(base)/lib_vline.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_vline.c -o ../objects/lib_vline$o

../objects/lib_wattroff$o :	$(base)/lib_wattroff.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_wattroff.c -o ../objects/lib_wattroff$o

../objects/lib_wattron$o :	$(base)/lib_wattron.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_wattron.c -o ../objects/lib_wattron$o

../objects/lib_winch$o :	$(base)/lib_winch.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_winch.c -o ../objects/lib_winch$o

../objects/lib_window$o :	$(base)/lib_window.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_window.c -o ../objects/lib_window$o

../objects/link_test$o :	./link_test.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/link_test.c -o ../objects/link_test$o

../objects/nc_panel$o :	$(base)/nc_panel.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/nc_panel.c -o ../objects/nc_panel$o

../objects/safe_sprintf$o :	$(base)/safe_sprintf.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/safe_sprintf.c -o ../objects/safe_sprintf$o

../objects/tty_update$o :	$(serial)/tty_update.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/tty_update.c -o ../objects/tty_update$o

../objects/varargs$o :	$(trace)/varargs.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(trace)/varargs.c -o ../objects/varargs$o

../objects/memmove$o :	$(base)/memmove.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/memmove.c -o ../objects/memmove$o

../objects/vsscanf$o :	$(base)/vsscanf.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/vsscanf.c -o ../objects/vsscanf$o

../objects/lib_freeall$o :	$(base)/lib_freeall.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_freeall.c -o ../objects/lib_freeall$o

../objects/charable$o :	$(wide)/charable.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/charable.c -o ../objects/charable$o

../objects/lib_add_wch$o :	$(wide)/lib_add_wch.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_add_wch.c -o ../objects/lib_add_wch$o

../objects/lib_box_set$o :	$(wide)/lib_box_set.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_box_set.c -o ../objects/lib_box_set$o

../objects/lib_cchar$o :	$(wide)/lib_cchar.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_cchar.c -o ../objects/lib_cchar$o

../objects/lib_erasewchar$o :	$(wide)/lib_erasewchar.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_erasewchar.c -o ../objects/lib_erasewchar$o

../objects/lib_get_wch$o :	$(wide)/lib_get_wch.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_get_wch.c -o ../objects/lib_get_wch$o

../objects/lib_get_wstr$o :	$(wide)/lib_get_wstr.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_get_wstr.c -o ../objects/lib_get_wstr$o

../objects/lib_hline_set$o :	$(wide)/lib_hline_set.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_hline_set.c -o ../objects/lib_hline_set$o

../objects/lib_in_wch$o :	$(wide)/lib_in_wch.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_in_wch.c -o ../objects/lib_in_wch$o

../objects/lib_in_wchnstr$o :	$(wide)/lib_in_wchnstr.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_in_wchnstr.c -o ../objects/lib_in_wchnstr$o

../objects/lib_ins_wch$o :	$(wide)/lib_ins_wch.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_ins_wch.c -o ../objects/lib_ins_wch$o

../objects/lib_inwstr$o :	$(wide)/lib_inwstr.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_inwstr.c -o ../objects/lib_inwstr$o

../objects/lib_key_name$o :	$(wide)/lib_key_name.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_key_name.c -o ../objects/lib_key_name$o

../objects/lib_pecho_wchar$o :	$(wide)/lib_pecho_wchar.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_pecho_wchar.c -o ../objects/lib_pecho_wchar$o

../objects/lib_slk_wset$o :	$(wide)/lib_slk_wset.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_slk_wset.c -o ../objects/lib_slk_wset$o

../objects/lib_unget_wch$o :	$(wide)/lib_unget_wch.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_unget_wch.c -o ../objects/lib_unget_wch$o

../objects/lib_vid_attr$o :	$(wide)/lib_vid_attr.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_vid_attr.c -o ../objects/lib_vid_attr$o

../objects/lib_vline_set$o :	$(wide)/lib_vline_set.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_vline_set.c -o ../objects/lib_vline_set$o

../objects/lib_wacs$o :	$(wide)/lib_wacs.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_wacs.c -o ../objects/lib_wacs$o

../objects/lib_wunctrl$o :	$(wide)/lib_wunctrl.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(wide)/lib_wunctrl.c -o ../objects/lib_wunctrl$o

../objects/expanded$o :	./expanded.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/expanded.c -o ../objects/expanded$o

../objects/legacy_coding$o :	$(base)/legacy_coding.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/legacy_coding.c -o ../objects/legacy_coding$o

../objects/lib_dft_fgbg$o :	$(base)/lib_dft_fgbg.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_dft_fgbg.c -o ../objects/lib_dft_fgbg$o

../objects/lib_print$o :	$(tinfo)/lib_print.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_print.c -o ../objects/lib_print$o

../objects/resizeterm$o :	$(base)/resizeterm.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/resizeterm.c -o ../objects/resizeterm$o

../objects/use_screen$o :	$(tinfo)/use_screen.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/use_screen.c -o ../objects/use_screen$o

../objects/use_window$o :	$(base)/use_window.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/use_window.c -o ../objects/use_window$o

../objects/wresize$o :	$(base)/wresize.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/wresize.c -o ../objects/wresize$o

../objects/access$o :	$(tinfo)/access.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/access.c -o ../objects/access$o

../objects/add_tries$o :	$(tinfo)/add_tries.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/add_tries.c -o ../objects/add_tries$o

../objects/alloc_ttype$o :	$(tinfo)/alloc_ttype.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/alloc_ttype.c -o ../objects/alloc_ttype$o

../objects/codes$o :	./codes.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/codes.c -o ../objects/codes$o

../objects/comp_captab$o :	./comp_captab.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h \
			../include/hashsize.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/comp_captab.c -o ../objects/comp_captab$o

../objects/comp_error$o :	$(tinfo)/comp_error.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/comp_error.c -o ../objects/comp_error$o

../objects/comp_hash$o :	$(tinfo)/comp_hash.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h \
			../include/hashsize.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/comp_hash.c -o ../objects/comp_hash$o

../objects/db_iterator$o :	$(tinfo)/db_iterator.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/db_iterator.c -o ../objects/db_iterator$o

../objects/doalloc$o :	$(tinfo)/doalloc.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/doalloc.c -o ../objects/doalloc$o

../objects/entries$o :	$(tinfo)/entries.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/entries.c -o ../objects/entries$o

../objects/fallback$o :	./fallback.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/fallback.c -o ../objects/fallback$o

../objects/free_ttype$o :	$(tinfo)/free_ttype.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/free_ttype.c -o ../objects/free_ttype$o

../objects/getenv_num$o :	$(tinfo)/getenv_num.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/getenv_num.c -o ../objects/getenv_num$o

../objects/home_terminfo$o :	$(tinfo)/home_terminfo.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/home_terminfo.c -o ../objects/home_terminfo$o

../objects/init_keytry$o :	$(tinfo)/init_keytry.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h \
			init_keytry.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/init_keytry.c -o ../objects/init_keytry$o

../objects/lib_acs$o :	$(tinfo)/lib_acs.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_acs.c -o ../objects/lib_acs$o

../objects/lib_baudrate$o :	$(tinfo)/lib_baudrate.c \
			$(HEADER_DEPS) \
			../include/termcap.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_baudrate.c -o ../objects/lib_baudrate$o

../objects/lib_cur_term$o :	$(tinfo)/lib_cur_term.c \
			$(HEADER_DEPS) \
			../include/termcap.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_cur_term.c -o ../objects/lib_cur_term$o

../objects/lib_data$o :	$(tinfo)/lib_data.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_data.c -o ../objects/lib_data$o

../objects/lib_has_cap$o :	$(tinfo)/lib_has_cap.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_has_cap.c -o ../objects/lib_has_cap$o

../objects/lib_kernel$o :	$(tinfo)/lib_kernel.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_kernel.c -o ../objects/lib_kernel$o

../objects/lib_keyname$o :	./lib_keyname.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/lib_keyname.c -o ../objects/lib_keyname$o

../objects/lib_longname$o :	$(tinfo)/lib_longname.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_longname.c -o ../objects/lib_longname$o

../objects/lib_napms$o :	$(tinfo)/lib_napms.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_napms.c -o ../objects/lib_napms$o

../objects/lib_options$o :	$(tinfo)/lib_options.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_options.c -o ../objects/lib_options$o

../objects/lib_raw$o :	$(tinfo)/lib_raw.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_raw.c -o ../objects/lib_raw$o

../objects/lib_setup$o :	$(tinfo)/lib_setup.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_setup.c -o ../objects/lib_setup$o

../objects/lib_termcap$o :	$(tinfo)/lib_termcap.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h \
			../include/termcap.h \
			$(INCDIR)/capdefaults.c
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_termcap.c -o ../objects/lib_termcap$o

../objects/lib_termname$o :	$(tinfo)/lib_termname.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_termname.c -o ../objects/lib_termname$o

../objects/lib_tgoto$o :	$(tinfo)/lib_tgoto.c \
			$(HEADER_DEPS) \
			../include/termcap.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_tgoto.c -o ../objects/lib_tgoto$o

../objects/lib_ti$o :	$(tinfo)/lib_ti.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_ti.c -o ../objects/lib_ti$o

../objects/lib_tparm$o :	$(tinfo)/lib_tparm.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_tparm.c -o ../objects/lib_tparm$o

../objects/lib_tputs$o :	$(tinfo)/lib_tputs.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h \
			../include/termcap.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_tputs.c -o ../objects/lib_tputs$o

../objects/lib_trace$o :	$(trace)/lib_trace.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(trace)/lib_trace.c -o ../objects/lib_trace$o

../objects/lib_ttyflags$o :	$(tinfo)/lib_ttyflags.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_ttyflags.c -o ../objects/lib_ttyflags$o

../objects/lib_twait$o :	$(serial)/lib_twait.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/lib_twait.c -o ../objects/lib_twait$o

../objects/name_match$o :	$(tinfo)/name_match.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/name_match.c -o ../objects/name_match$o

../objects/names$o :	./names.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/names.c -o ../objects/names$o

../objects/read_entry$o :	$(tinfo)/read_entry.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h \
			$(INCDIR)/hashed_db.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/read_entry.c -o ../objects/read_entry$o

../objects/read_termcap$o :	$(tinfo)/read_termcap.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/read_termcap.c -o ../objects/read_termcap$o

../objects/setbuf$o :	$(tinfo)/setbuf.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/setbuf.c -o ../objects/setbuf$o

../objects/strings$o :	$(tinfo)/strings.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/strings.c -o ../objects/strings$o

../objects/tries$o :	$(base)/tries.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/tries.c -o ../objects/tries$o

../objects/trim_sgr0$o :	$(tinfo)/trim_sgr0.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/trim_sgr0.c -o ../objects/trim_sgr0$o

../objects/unctrl$o :	./unctrl.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/unctrl.c -o ../objects/unctrl$o

../objects/visbuf$o :	$(trace)/visbuf.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(trace)/visbuf.c -o ../objects/visbuf$o

../objects/alloc_entry$o :	$(tinfo)/alloc_entry.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/alloc_entry.c -o ../objects/alloc_entry$o

../objects/captoinfo$o :	$(tinfo)/captoinfo.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/captoinfo.c -o ../objects/captoinfo$o

../objects/comp_expand$o :	$(tinfo)/comp_expand.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/comp_expand.c -o ../objects/comp_expand$o

../objects/comp_parse$o :	$(tinfo)/comp_parse.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/comp_parse.c -o ../objects/comp_parse$o

../objects/comp_scan$o :	$(tinfo)/comp_scan.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/comp_scan.c -o ../objects/comp_scan$o

../objects/parse_entry$o :	$(tinfo)/parse_entry.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h \
			../include/parametrized.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/parse_entry.c -o ../objects/parse_entry$o

../objects/write_entry$o :	$(tinfo)/write_entry.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h \
			$(INCDIR)/hashed_db.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/write_entry.c -o ../objects/write_entry$o

../objects/define_key$o :	$(base)/define_key.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/define_key.c -o ../objects/define_key$o

../objects/hashed_db$o :	$(tinfo)/hashed_db.c \
			$(HEADER_DEPS) \
			$(INCDIR)/tic.h \
			$(INCDIR)/hashed_db.h
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/hashed_db.c -o ../objects/hashed_db$o

../objects/key_defined$o :	$(base)/key_defined.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/key_defined.c -o ../objects/key_defined$o

../objects/keybound$o :	$(base)/keybound.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/keybound.c -o ../objects/keybound$o

../objects/keyok$o :	$(base)/keyok.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/keyok.c -o ../objects/keyok$o

../objects/version$o :	$(base)/version.c \
			$(HEADER_DEPS)
	$(LIBTOOL_COMPILE) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/version.c -o ../objects/version$o

depend : ${AUTO_SRC}
	makedepend -- ${CPPFLAGS} -- ${C_SRC}

# DO NOT DELETE THIS LINE -- make depend depends on it.
