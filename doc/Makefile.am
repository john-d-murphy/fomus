#   Copyright (C) 2009, 2010, 2011  David Psenicka
#   This file is part of FOMUS.

#   FOMUS is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.

#   FOMUS is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

CLEANFILES = doc.html

EXTRA_DIST = readme.texi comments.fms

GENS = sets_list1.texi sets_list2.texi marks_list.texi insts_list.texi percs_list.texi

if ISDEVEL
noinst_PROGRAMS = makedocs

AM_CPPFLAGS = @FOMUS_CPPFLAGS@ @BOOST_CPPFLAGS@ -I$(top_srcdir)/src/lib/api
AM_CFLAGS = @FOMUS_CFLAGSX@
AM_CXXFLAGS = @FOMUS_CXXFLAGSX@
# + @FOMUS_LDFLAGSX@
AM_LDFLAGS = @BOOST_LDFLAGS@ 

makedocs_LDADD = $(top_builddir)/src/lib/libfomus.la
makedocs_SOURCES = makedocs.cc

sets_list1.texi: makedocs
	find $(top_builddir)/src/lib -path \*/.svn -prune -o -type f \( -name \*.cc -o -name \*.h \) -print0 | xargs -0 grep docscat\{ | sed 's/^.*"\(.*\)".*docscat{\([a-z]*\)}.*$$/\2 \1/' > sets_cats.in
	FOMUS_CONFIG_PATH=$(builddir) ./makedocs sets1 > sets_list1.texi
sets_list2.texi: makedocs
	FOMUS_CONFIG_PATH=$(builddir) ./makedocs sets2 > sets_list2.texi

marks_list.texi: makedocs
	FOMUS_CONFIG_PATH=$(builddir) ./makedocs marks > marks_list.texi

insts_list.texi: makedocs
	FOMUS_CONFIG_PATH=$(builddir) ./makedocs insts > insts_list.texi
percs_list.texi: makedocs
	FOMUS_CONFIG_PATH=$(builddir) ./makedocs percs > percs_list.texi

$(srcdir)/sal_examples.texi: examples.texi
	cat examples.texi | sed 's/@node XXX/@node SAL/' | sed 's/@chapter XXX/@chapter SAL/' | \
sed 's/@node\([^,]*\)Lisp/@c @node\1Lisp/' | sed 's/@section\([^,]*\)Lisp/@c @section\1Lisp/' | \
sed 's/@node\([^,]*\)File/@c @node\1File/' | sed 's/@section\([^,]*\)File/@c @section\1File/' | \
sed 's/@node\([^,]*\)(xxx)/@node\1(SAL)/' | sed 's/@set exs/@set salexs/' | sed 's/@clear exs/@clear salexs/' > $(srcdir)/sal_examples.texi

$(srcdir)/lisp_examples.texi: examples.texi
	cat examples.texi | sed 's/@node XXX/@node Lisp/' | sed 's/@chapter XXX/@chapter Lisp/' | \
sed 's/@node\([^,]*\)SAL/@c @node\1SAL/' | sed 's/@section\([^,]*\)SAL/@c @section\1SAL/' | \
sed 's/@node\([^,]*\)File/@c @node\1File/' | sed 's/@section\([^,]*\)File/@c @section\1File/' | \
sed 's/@node\([^,]*\)(xxx)/@node\1(Lisp)/' | sed 's/@set exs/@set lispexs/' | sed 's/@clear exs/@clear lispexs/' > $(srcdir)/lisp_examples.texi

$(srcdir)/file_examples.texi: examples.texi
	cat examples.texi | sed "s/@node XXX/@node \`.fms' File/" | sed "s/@chapter XXX/@chapter \`.fms' File/" | \
sed 's/@node\([^,]*\)Lisp/@c @node\1Lisp/' | sed 's/@section\([^,]*\)Lisp/@c @section\1Lisp/' | \
sed 's/@node\([^,]*\)SAL/@c @node\1SAL/' | sed 's/@section\([^,]*\)SAL/@c @section\1SAL/' | \
sed 's/@node\([^,]*\)(xxx)/@node\1(File)/' | sed 's/@set exs/@set fileexs/' | sed 's/@clear exs/@clear fileexs/' > $(srcdir)/file_examples.texi

# NOT PART OF THE BUILD PROCESSES
examples: $(srcdir)/lisp_examples.texi $(srcdir)/sal_examples.texi $(srcdir)/file_examples.texi

comments.fms: ex007.fms
	cp ex007.fms comments.fms

update: examples comments.fms
	emacs --batch doc.texi --eval "(texinfo-multiple-files-update \"doc.texi\" t t)" --eval "(save-some-buffers t)"
	head -n `grep -n @detailmenu doc.texi | sed 's/:@detailmenu//'` doc.texi | head -n -2 > doc1.texi.in
	tail -n +`grep -n '@end detailmenu' doc.texi | sed 's/:@end detailmenu//'` doc.texi | tail -n +2 > doc2.texi.in
	cat doc1.texi.in doc2.texi.in > doc.texi
	rm -f doc1.texi doc2.texi

CLEANFILES += $(GENS) sets_cats.in doc?.texi.in
# EXTRA_DIST += $(GENS)
.PHONY: update examples
endif

dvi:
install-dvi:
# uninstall-dvi-am:

info:
install-info:
uninstall-info-am:

install-man:

pdf:
install-pdf:
# uninstall-pdf-am:

ps:
install-ps:
# uninstall-ps-am:

# exampdir = $(docdir)/examples

# uninstall-local:
# 	rm -rf $(DESTDIR)$(exampdir)

AM_MAKEINFOHTMLFLAGS = --css-include=$(srcdir)/fomus.css 

imgfiles = tut001.fms tut002.fms tut003.fms tut004.fms tut005.fms \
           tut006.fms tut007.fms tut008.fms tut009.fms tut010.fms \
           tut011.fms tut012.fms \
           \
           lsp001.lisp lsp002.lisp lsp003.lisp lsp004.lisp lsp005.lisp \
           lsp006.lisp lsp007.lisp lsp008.lisp lsp009.lisp lsp010.lisp \
           lsp011.lisp lsp012.lisp lsp013.lisp lsp014.lisp lsp015.lisp \
           lsp016.lisp lsp017.lisp lsp018.lisp lsp019.lisp lsp020.lisp \
           lsp021.lisp lsp022.lisp lsp023.lisp lsp024.lisp lsp025.lisp \
           lsp026.lisp lsp027.lisp lsp028.lisp lsp029.lisp lsp030.lisp \
           lsp031.lisp lsp032.lisp lsp033.lisp lsp034.lisp lsp035.lisp \
           lsp036.lisp lsp037.lisp lsp038.lisp lsp039.lisp lsp040.lisp \
           lsp041.lisp lsp042.lisp lsp043.lisp lsp044.lisp lsp045.lisp \
           lsp046.lisp lsp047.lisp lsp048.lisp lsp049.lisp lsp050.lisp \
           lsp051.lisp lsp052.lisp lsp053.lisp lsp054.lisp lsp055.lisp \
           lsp056.lisp lsp057.lisp lsp058.lisp lsp059.lisp lsp060.lisp \
           lsp061.lisp lsp062.lisp lsp063.lisp lsp064.lisp lsp065.lisp \
           lsp066.lisp lsp067.lisp lsp068.lisp lsp069.lisp lsp070.lisp \
           lsp071.lisp lsp072.lisp lsp073.lisp \
           \
           sal001.sal sal002.sal sal003.sal sal004.sal sal005.sal \
           sal006.sal sal007.sal sal008.sal sal009.sal sal010.sal \
           sal011.sal sal012.sal sal013.sal sal014.sal sal015.sal \
           sal016.sal sal017.sal sal018.sal sal019.sal sal020.sal \
           sal021.sal sal022.sal sal023.sal sal024.sal sal025.sal \
           sal026.sal sal027.sal sal028.sal sal029.sal sal030.sal \
           sal031.sal sal032.sal sal033.sal sal034.sal sal035.sal \
           sal036.sal sal037.sal sal038.sal sal039.sal sal040.sal \
           sal041.sal sal042.sal sal043.sal sal044.sal sal045.sal \
           sal046.sal sal047.sal sal048.sal sal049.sal sal050.sal \
           sal051.sal sal052.sal sal053.sal sal054.sal sal055.sal \
           sal056.sal sal057.sal sal058.sal sal059.sal sal060.sal \
           sal061.sal sal062.sal sal063.sal sal064.sal sal065.sal \
           sal066.sal sal067.sal sal068.sal sal069.sal sal070.sal \
           sal071.sal sal072.sal \
           \
           mid001.fms \
           \
           ex001.fms ex002.fms ex003.fms ex004.fms ex005.fms \
           ex006.fms ex007.fms ex008.fms ex009.fms ex010.fms \
           ex011.fms ex012.fms ex013.fms ex014.fms ex015.fms \
           ex016.fms ex017.fms ex018.fms ex019.fms ex020.fms \
           ex021.fms ex022.fms ex023.fms ex024.fms ex025.fms \
           ex026.fms ex027.fms ex028.fms ex029.fms ex030.fms \
           ex031.fms ex032.fms ex033.fms ex034.fms ex035.fms \
           ex036.fms ex037.fms ex038.fms ex039.fms ex040.fms \
           ex041.fms ex042.fms ex043.fms ex044.fms ex045.fms \
           ex046.fms ex047.fms ex048.fms ex049.fms ex050.fms \
           ex051.fms ex052.fms ex053.fms ex054.fms ex055.fms \
           ex056.fms ex057.fms ex058.fms ex059.fms ex060.fms \
           ex061.fms ex062.fms ex063.fms ex064.fms ex065.fms \
           ex066.fms ex067.fms ex068.fms ex069.fms ex070.fms \
           ex071.fms ex072.fms ex073.fms ex074.fms ex075.fms \
           ex076.fms ex077.fms ex078.fms ex079.fms ex080.fms \
           ex081.fms \
           \
           score001.fms score002.fms

# these are the images and get distributed--all go in srcdir
doc_PNGS = $(srcdir)/hierarchy.png \
           $(srcdir)/inheritance.png \
           $(subst .sal,.png,$(subst .lisp,.png,$(subst .fms,.png,$(patsubst %,$(srcdir)/%,$(imgfiles)))))

# these are install targets
PNGSHTML = doc.html/hierarchy.png \
           doc.html/inheritance.png \
           $(subst .sal,.png,$(subst .lisp,.png,$(subst .fms,.png,$(patsubst %,doc.html/%,$(imgfiles)))))

# these are the text sources get distributed and installed somewhere--all go in srcdir
doc_FMSS = $(patsubst %,$(srcdir)/%,$(imgfiles)) \
           $(srcdir)/mid001.mid

info_TEXINFOS = doc.texi
doc_TEXINFOS = doc.texi file_examples.texi lisp_examples.texi sal_examples.texi midi_examples.texi scores.texi \
               install.texi intro.texi reference.texi tutorial.texi usage.texi exs_index.texi extypes.texi exmods.texi \
               $(doc_PNGS) $(doc_FMSS) $(GENS) fomus.css

doc_DATA = $(top_srcdir)/README

html-local: doc.html/hierarchy.png doc.html/inheritance.png $(PNGSHTML)

doc.html/%.png: $(srcdir)/%.png
	if [[ ! -d doc.html ]]; then mkdir doc.html; fi
	install -m644 $< doc.html/

# SAMPLE MODULES
doc_EXMODS = untie.cc

untie.cc: $(top_srcdir)/src/lib/mod/div/untie.cc
	cat $< | sed '/^#include *"debugaux.h"/d' | sed 's/^#include *"\(.*\)"/#include <fomus\/\1>/' | sed 's/ *docscat{.*}//' > $@

CLEANFILES += $(doc_EXMODS)

# bug in automake? doc.html doesn't get wiped
clean-local:
	rm -rf doc.html

html: $(GENS) $(doc_PNGS) $(doc_EXMODS) $(srcdir)/version.texi html-am

if ISDEVEL
# GRAPHS
$(srcdir)/%.png: $(srcdir)/%.dot
	dot $< -o %.png
	convert %.png -background white -resize 60% $@

# EXS
$(srcdir)/tut%.png: $(srcdir)/tut%.fms 
	FOMUS_CONFIG_PATH=$(top_builddir)/src/test $(bindir)/fomus -i $< -o tut$*.ly >/dev/null 2>tut$*.log
	convert tut$*.png -resize 60% -trim +repage -background white $(srcdir)/tut$*.png
	if [[ -n `cat tut$*.log` ]]; then exit 1; else rm -f tut$*.log; fi

$(srcdir)/ex%.png: $(srcdir)/ex%.fms 
	FOMUS_CONFIG_PATH=$(top_builddir)/src/test $(bindir)/fomus -i $< -o ex$*.ly >/dev/null 2>ex$*.log
	convert ex$*.png -resize 60% -trim +repage -background white $(srcdir)/ex$*.png
	if [[ -n `cat ex$*.log` ]]; then exit 1; else rm -f ex$*.log; fi

$(srcdir)/score%.png: $(srcdir)/score%.fms 
	FOMUS_CONFIG_PATH=$(top_builddir)/src/test $(bindir)/fomus -i $< -o score$*.ly >/dev/null 2>score$*.log
	convert score$*.png -resize 60% -trim +repage -background white $(srcdir)/score$*.png
	if [[ -n `cat score$*.log` ]]; then exit 1; else rm -f score$*.log; fi

$(srcdir)/mid%.png: $(srcdir)/mid%.fms 
	FOMUS_CONFIG_PATH=$(top_builddir)/src/test $(bindir)/fomus -i $< -i $(srcdir)/mid$*.mid -o mid$*.ly >/dev/null 2>mid$*.log
	convert mid$*.png -resize 60% -trim +repage -background white $(srcdir)/mid$*.png
	if [[ -n `cat mid$*.log` ]]; then exit 1; else rm -f mid$*.log; fi

mdbg%.fms: $(srcdir)/mid%.fms 
	FOMUS_CONFIG_PATH=$(top_builddir)/src/test $(bindir)/fomus -i $< -i $(srcdir)/mid$*.mid -o mdbg$*.fms

stamp-lisp:
	FOMUS_CONFIG_PATH=$(top_builddir)/src/test sbcl \
--noinform --disable-ldb --lose-on-corruption --no-sysinit --no-userinit --disable-debugger \
--eval "(push \"$(pkgdatadir)/\" asdf:*central-registry*)" \
--eval "(asdf:operate 'asdf:load-op :fomus)" \
--eval '(quit)' >/dev/null 2>&1
	touch stamp-lisp

$(srcdir)/lsp%.png: $(srcdir)/lsp%.lisp stamp-lisp
	FOMUS_CONFIG_PATH=$(top_builddir)/src/test sbcl \
--noinform --disable-ldb --lose-on-corruption --no-sysinit --no-userinit --disable-debugger \
--eval "(push \"$(pkgdatadir)/\" asdf:*central-registry*)" \
--eval "(asdf:operate 'asdf:load-op :fomus)" \
--eval "(defparameter *filename* \"lsp$*.ly\")" \
--load $< \
--eval '(quit)' >/dev/null 2>lsp$*.log
	convert lsp$*.png -resize 60% -trim +repage -background white $(srcdir)/lsp$*.png
	if [[ -n `cat lsp$*.log` ]]; then exit 1; else rm -f lsp$*.log; fi

ldbg%.fms: lsp%.lisp
	FOMUS_CONFIG_PATH=$(top_builddir)/src/test sbcl \
--noinform --disable-ldb --lose-on-corruption --no-sysinit --no-userinit --disable-debugger \
--eval "(push \"$(pkgdatadir)/\" asdf:*central-registry*)" \
--eval "(asdf:operate 'asdf:load-op :fomus)" \
--eval "(defparameter *filename* \"ldbg$*.fms\")" \
--load $< \
--eval '(quit)'

# --eval "(load \"/home/david/local/projects/cm/scm/fomus.scm\")"
$(srcdir)/sal%.png: $(srcdir)/sal%.sal
	FOMUS_CONFIG_PATH=$(top_builddir)/src/test cm --quiet --batch \
--eval "(define *filename* \"sal$*.ly\")" \
--eval "(load \"$<\")" >sal$*.out 2>sal$*.log
	convert sal$*.png -resize 60% -trim +repage -background white $(srcdir)/sal$*.png
	if [[ -n `cat sal$*.log` ]]; then exit 1; else rm -f sal$*.log sal$*.out; fi

# --eval "(load \"/home/david/local/projects/cm/scm/fomus.scm\")"
sdbg%.fms: $(srcdir)/sal%.sal
	FOMUS_CONFIG_PATH=$(top_builddir)/src/test cm --quiet --batch \
--eval "(define *filename* \"sdbg$*.fms\")" \
--eval "(load \"$<\")"

CLEANFILES += *.png $(srcdir)/*.png \
              tut*.ly tut*.log tut*.ps tut*.pdf \
              score*.ly score*.log score*.ps score*.pdf \
              ex*.ly ex*.log ex*.ps ex*.pdf \
              mid*.ly mid*.log mid*.ps mid*.pdf mdbg*.fms \
              lsp*.ly lsp*.log lsp*.ps lsp*.pdf ldbg*.fms \
              sal*.ly sal*.log sal*.out sal*.ps sal*.pdf sdbg*.fms \
              *-page?.png stamp-lisp
endif

# README
if ISDEVEL
readme: $(srcdir)/readme.texi $(srcdir)/install.texi
	-makeinfo --no-headers $(srcdir)/readme.texi --force --fill-column=119 > readme
	cp readme $(top_srcdir)/README

dist-hook: readme
	cp readme $(top_distdir)/README

# don't clean top-level readme
CLEANFILES += readme
endif

# create a red comparison image
if ISDEVEL
$(srcdir)/cmp%.png: $(srcdir)/ex%.png
	convert $< -channel R -white-threshold -1% $@
endif
