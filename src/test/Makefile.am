#   Copyright (C) 2009, 2010, 2011, 2012, 2013  David Psenicka
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

check_PROGRAMS = testheads 

AM_CPPFLAGS = @FOMUS_CPPFLAGS@ -DTEST_PATH="$(top_srcdir)/src/test" -I$(top_srcdir)/src/lib/api

testheads_LDADD = $(top_builddir)/src/lib/libfomus.la
testheads_SOURCES = testheads.c 

TESTS = testheads

TESTFMS = in001.fms in002.fms in003.fms in004.fms in005.fms \
          in006.fms in007.fms in008.fms in009.fms in010.fms \
          in011.fms in012.fms in013.fms in014.fms in015.fms \
          in016.fms in017.fms in018.fms in019.fms in020.fms \
          in021.fms in022.fms in023.fms in024.fms in025.fms \
          in026.fms in027.fms in028.fms in029.fms in030.fms \
          in031.fms in032.fms in033.fms

TESTDOCS = tut001.fms tut002.fms tut003.fms tut004.fms tut005.fms \
           tut006.fms tut007.fms tut008.fms tut009.fms tut010.fms \
           tut011.fms tut012.fms \
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

# TESTCMPS = $(patsubst in%.fms, $(srcdir)/cmp%.png, $(TESTFMS)) \
#            $(patsubst score%.fms, $(srcdir)/cms%.png, $(patsubst ex%.fms, $(srcdir)/cmx%.png, $(patsubst tut%.fms, $(srcdir)/cmt%.png, $(TESTDOCS))))


TESTLISP = in001.lisp in002.lisp in003.lisp

EXTRA_DIST = $(TESTFMS) $(TESTLISP) out1.html.in out2.html.in out3.html.in

check-parse:
	@echo "  running init file test..."
	@touch testreadwrite.fms
	@rm -f testout1.fms testout2.fms testout3.fms
	@FOMUS_CONFIG_PATH=$(builddir) $(bindir)/fomus -i testreadwrite.fms -o testout1.fms >/dev/null
	@test -e testout1.fms
	@FOMUS_CONFIG_PATH=$(builddir) $(bindir)/fomus -i testout1.fms -o testout2.fms >/dev/null
	@test -e testout2.fms
	@cat testout1.fms | @SED@ -n 3,\$$p | @SED@ "s/^filename = .*$$//" > testout1a.fms
	@cat testout2.fms | @SED@ -n 3,\$$p | @SED@ "s/^filename = .*$$//" > testout2a.fms
	@cmp testout1a.fms testout2a.fms
	@rm -rf testhome
	@mkdir testhome >/dev/null 2>&1
	@cat testout2a.fms | @SED@ "s/^part default$$//" > testhome/.fomus
	@FOMUS_CONFIG_PATH=testhome $(bindir)/fomus -i testreadwrite.fms -o testout3.fms >/dev/null
	@test -e testout3.fms
	@cat testout3.fms | @SED@ -n 3,\$$p | @SED@ "s/^filename = .*$$//" > testout3a.fms
	@cmp testout2a.fms testout3a.fms

check-tests:
	@echo "  running regression tests..."
	@cat $(srcdir)/out1.html.in > $(top_builddir)/check.html; \
for E in $(patsubst %,$(srcdir)/%,$(TESTFMS)); do \
  BN=`basename $$E`; \
  PNG=`echo $$BN | @SED@ 's/in\(...\)\.fms/out\1.png/'`; \
  CMP=`echo $$BN | @SED@ 's/in\(...\)\.fms/cmp\1.png/'`; \
  XML=`echo $$BN | @SED@ 's/in\(...\)\.fms/out\1.xml/'`; \
  LY=`echo $$BN | @SED@ 's/in\(...\)\.fms/out\1.ly/'`; \
  MID=`echo $$BN | @SED@ 's/in\(...\)\.fms/out\1.mid/'`; \
  LYPS=`echo $$BN | @SED@ 's/in\(...\)\.fms/out\1.ps/'`; \
  LYPDF=`echo $$BN | @SED@ 's/in\(...\)\.fms/out\1.pdf/'`; \
  XMLLOG=`echo $$BN | @SED@ 's/in\(...\)\.fms/out\1.xml.log/'`; \
  LYLOG=`echo $$BN | @SED@ 's/in\(...\)\.fms/out\1.ly.log/'`; \
  MIDLOG=`echo $$BN | @SED@ 's/in\(...\)\.fms/out\1.mid.log/'`; \
  rm -f $$XML $$LY $$PNG $$XMLLOG $$LYLOG $$LYPS $$LYPDF $$MID $$MIDLOG; \
  FERR='0'; \
  XERR='0'; \
  MERR='0'; \
  echo "    $$BN"; \
  FOMUS_CONFIG_PATH=$(builddir) $(bindir)/fomus $$E -o $$LY >/dev/null 2>$$LYLOG || FERR='1'; \
  if [[ ! -e $$PNG ]]; then FERR='1'; fi; \
  if [[ -n `cat $$LYLOG` ]]; then FERR='1'; else rm -f $$LYLOG; fi; \
  if [[ $$FERR != '0' ]]; then FFILES="$$FFILES $$BN"; echo "      LILYPOND TEST FAILED"; fi; \
  FOMUS_CONFIG_PATH=$(builddir) $(bindir)/fomus $$E -o $$XML >/dev/null 2>$$XMLLOG || XERR='1'; \
  test -e $$XML && xmllint --noout --valid $$XML || XERR='1'; \
  if [[ -n `cat $$XMLLOG` ]]; then XERR='1'; else rm -f $$XMLLOG; fi; \
  if [[ $$XERR != '0' ]]; then XFILES="$$XFILES $$BN"; echo "      XML TEST FAILED"; fi; \
  FOMUS_CONFIG_PATH=$(builddir) $(bindir)/fomus $$E -o $$MID >/dev/null 2>$$MIDLOG || MERR='1'; \
  if [[ -n `cat $$MIDLOG` ]]; then MERR='1'; else rm -f $$MIDLOG; fi; \
  if [[ $$MERR != '0' ]]; then MFILES="$$MFILES $$BN"; echo "      MIDI TEST FAILED"; fi; \
  rm -f $$LYPS; \
  if [[ "@ISDEV@" = "1" ]]; then composite -compose darken $$PNG $(srcdir)/$$CMP $$PNG 2>/dev/null; fi; \
  I=`echo $$BN | @SED@ 's:in\(...\)\.fms:src/test/in\1.fms:'`; \
  O=`echo $$BN | @SED@ 's:in\(...\)\.fms:src/test/out\1.png:'`; \
  cat $(srcdir)/out2.html.in | @SED@ "s%IN%$$I%" | @SED@ "s%OUT%$$O%" | @SED@ "s%NAME%$$BN%" >> $(top_builddir)/check.html; \
done; \
cat $(srcdir)/out3.html.in >> $(top_builddir)/check.html; \
echo "-------------------------------------------------------------------------------"; \
if [[ -n "$$FFILES" ]]; then echo "  FAILED LILYPOND TESTS:$$FFILES"; fi; \
if [[ -n "$$XFILES" ]]; then echo "  FAILED XML TESTS:$$XFILES"; fi; \
if [[ -n "$$MFILES" ]]; then echo "  FAILED MIDI TESTS:$$MFILES"; fi; \
if [[ -z "$$FFILES" && -z "$$XFILES" && -z "$$MFILES" ]]; then echo "  SUCCESS!"; fi; \
echo "-------------------------------------------------------------------------------"; \
if test -n "$$FFILES" -o -n "$$XFILES" -o -n "$$MFILES"; then exit 1; fi

check-outfiles:
	@echo "  running output file tests..."
	@for E in $(patsubst %,$(srcdir)/%,$(TESTFMS)); do \
  BN=`basename $$E`; \
  FMS=`echo $$BN | @SED@ 's/in\(...\)\.fms/fms\1.fms/'`; \
  LYA=`echo $$BN | @SED@ 's/in\(...\)\.fms/lya\1.ly/'`; \
  LYB=`echo $$BN | @SED@ 's/in\(...\)\.fms/lyb\1.ly/'`; \
  LYC=`echo $$BN | @SED@ 's/in\(...\)\.fms/lyc\1.ly/'`; \
  LYD=`echo $$BN | @SED@ 's/in\(...\)\.fms/lyd\1.ly/'`; \
  rm -f $$FMA $$FMB $$LYA $$LYB $$LYC $$LYD; \
  FERR='0'; \
  echo "    $$BN"; \
  FOMUS_CONFIG_PATH=$(builddir) $(bindir)/fomus $$E -o $$FMS >/dev/null 2>&1 || FERR='1'; \
  FOMUS_CONFIG_PATH=$(builddir) $(bindir)/fomus $$E -o $$LYA >/dev/null 2>&1 || FERR='1'; \
  FOMUS_CONFIG_PATH=$(builddir) $(bindir)/fomus $$FMS -o $$LYB >/dev/null 2>&1 || FERR='1'; \
  tail -n+7 $$LYA > $$LYC >/dev/null 2>&1 || FERR='1'; \
  tail -n+7 $$LYB > $$LYD >/dev/null 2>&1 || FERR='1'; \
  diff $$LYC $$LYD >/dev/null 2>&1 || FERR='1'; \
  rm -f $$LYC $$LYD; \
  if [[ $$FERR != '0' ]]; then FFILES="$$FFILES $$BN"; echo "      TEST FAILED"; fi; \
done; \
echo "-------------------------------------------------------------------------------"; \
if [[ -z "$$FFILES" ]]; then echo "  SUCCESS!"; else echo "  FAILED OUTPUT FILE TESTS:$$FFILES"; fi; \
echo "-------------------------------------------------------------------------------"; \
if test -n "$$FFILES"; then exit 1; fi

check-docs:
	@echo "  running documentation tests..."
	@cat $(srcdir)/out1.html.in > $(top_builddir)/checkdocs.html; \
for E in $(patsubst %,$(top_srcdir)/doc/%,$(TESTDOCS)); do \
  BN=`basename $$E`; \
  PNG=`echo $$BN | @SED@ 's/ex\(...\)\.fms/ex\1.png/' | @SED@ 's/tut\(...\)\.fms/tut\1.png/' | @SED@ 's/score\(...\)\.fms/sco\1.png/'`; \
  CMP=`echo $$BN | @SED@ 's/ex\(...\)\.fms/cmx\1.png/' | @SED@ 's/tut\(...\)\.fms/cmt\1.png/' | @SED@ 's/score\(...\)\.fms/cms\1.png/'`; \
  XML=`echo $$BN | @SED@ 's/ex\(...\)\.fms/ex\1.xml/' | @SED@ 's/tut\(...\)\.fms/tut\1.xml/' | @SED@ 's/score\(...\)\.fms/sco\1.xml/'`; \
  LY=`echo $$BN | @SED@ 's/ex\(...\)\.fms/ex\1.ly/' | @SED@ 's/tut\(...\)\.fms/tut\1.ly/' | @SED@ 's/score\(...\)\.fms/sco\1.ly/'`; \
  LYPS=`echo $$BN | @SED@ 's/ex\(...\)\.fms/ex\1.ps/' | @SED@ 's/tut\(...\)\.fms/tut\1.ps/' | @SED@ 's/score\(...\)\.fms/sco\1.ps/'`; \
  LYPDF=`echo $$BN | @SED@ 's/ex\(...\)\.fms/ex\1.pdf/' | @SED@ 's/tut\(...\)\.fms/tut\1.pdf/' | @SED@ 's/score\(...\)\.fms/sco\1.pdf/'`; \
  XMLLOG=`echo $$BN | @SED@ 's/ex\(...\)\.fms/ex\1.xml.log/' | @SED@ 's/tut\(...\)\.fms/tut\1.xml.log/' | @SED@ 's/score\(...\)\.fms/sco\1.xml.log/'`; \
  LYLOG=`echo $$BN | @SED@ 's/ex\(...\)\.fms/ex\1.ly.log/' | @SED@ 's/tut\(...\)\.fms/tut\1.ly.log/' | @SED@ 's/score\(...\)\.fms/sco\1.ly.log/'`; \
  rm -f $$XML $$LY $$PNG $$XMLLOG $$LYLOG $$LYPS $$LYPDF; \
  FERR='0'; \
  XERR='0'; \
  echo "    $$BN"; \
  FOMUS_CONFIG_PATH=$(builddir) $(bindir)/fomus $$E -o $$LY >/dev/null 2>$$LYLOG || FERR='1'; \
  if [[ ! -e $$PNG ]]; then FERR='1'; fi; \
  if [[ -n `cat $$LYLOG` ]]; then FERR='1'; else rm -f $$LYLOG; fi; \
  if [[ $$FERR != '0' ]]; then FFILES="$$FFILES $$BN"; echo "      LILYPOND TEST FAILED"; fi; \
  FOMUS_CONFIG_PATH=$(builddir) $(bindir)/fomus $$E -o $$XML >/dev/null 2>$$XMLLOG || XERR='1'; \
  test -e $$XML && xmllint --noout --valid $$XML || XERR='1'; \
  if [[ -n `cat $$XMLLOG` ]]; then XERR='1'; else rm -f $$XMLLOG; fi; \
  if [[ $$XERR != '0' ]]; then XFILES="$$XFILES $$BN"; echo "      XML TEST FAILED"; fi; \
  rm -f $$LYPS; \
  if [[ "@ISDEV@" = "1" ]]; then composite -compose darken $$PNG $(srcdir)/$$CMP $$PNG 2>/dev/null; fi; \
  I=`echo $$BN | @SED@ 's:ex\(...\)\.fms:doc/ex\1.fms:' | @SED@ 's:tut\(...\)\.fms:doc/tut\1.fms:' | @SED@ 's:score\(...\)\.fms:doc/score\1.fms:'`; \
  O=`echo $$BN | @SED@ 's:ex\(...\)\.fms:src/test/ex\1.png:' | @SED@ 's:tut\(...\)\.fms:src/test/tut\1.png:' | @SED@ 's:score\(...\)\.fms:src/test/sco\1.png:'`; \
  cat $(srcdir)/out2.html.in | @SED@ "s%IN%$$I%" | @SED@ "s%OUT%$$O%" | @SED@ "s%NAME%$$BN%" >> $(top_builddir)/checkdocs.html; \
done; \
cat $(srcdir)/out3.html.in >> $(top_builddir)/checkdocs.html; \
echo "-------------------------------------------------------------------------------"; \
if [[ -n "$$FFILES" ]]; then echo "  FAILED LILYPOND TESTS:$$FFILES"; fi; \
if [[ -n "$$XFILES" ]]; then echo "  FAILED XML TESTS:$$XFILES"; fi; \
if [[ -z "$$FFILES" && -z "$$XFILES" ]]; then echo "  SUCCESS!"; fi; \
echo "-------------------------------------------------------------------------------"; \
if test -n "$$FFILES" -o -n "$$XFILES"; then exit 1; fi

check-lisp:
	@echo "  running lisp test..."
	@FOMUS_CONFIG_PATH=$(top_builddir)/src/test sbcl \
--noinform --disable-ldb --lose-on-corruption --no-sysinit --no-userinit --disable-debugger \
--eval "(push \"$(pkgdatadir)/\" asdf:*central-registry*)" \
--eval "(asdf:operate 'asdf:load-op :fomus)" \
--eval "(asdf:operate 'asdf:load-op :fomusmod)" \
--eval '(quit)' >/dev/null 2>&1

installcheck-local: check-parse check-tests check-outfiles check-docs check-lisp

# create a red comparison image
if ISDEVEL
$(srcdir)/cmp%.png: $(srcdir)/in%.fms
	FOMUS_CONFIG_PATH=$(builddir) $(bindir)/fomus $(srcdir)/in$*.fms -o out$*.ly
	convert out$*.png -channel R -evaluate ThresholdWhite -1% $@

$(srcdir)/cmx%.png: $(top_srcdir)/doc/ex%.fms
	FOMUS_CONFIG_PATH=$(builddir) $(bindir)/fomus $(top_srcdir)/doc/ex$*.fms -o ex$*.ly
	convert ex$*.png -channel R -evaluate ThresholdWhite -1% $@

$(srcdir)/cmt%.png: $(top_srcdir)/doc/tut%.fms
	FOMUS_CONFIG_PATH=$(builddir) $(bindir)/fomus $(top_srcdir)/doc/tut$*.fms -o tut$*.ly
	convert tut$*.png -channel R -evaluate ThresholdWhite -1% $@

$(srcdir)/cms%.png: $(top_srcdir)/doc/score%.fms
	FOMUS_CONFIG_PATH=$(builddir) $(bindir)/fomus $(top_srcdir)/doc/score$*.fms -o sco$*.ly
	convert sco$*.png -channel R -evaluate ThresholdWhite -1% $@
endif

CLEANFILES = out???.fms out???.ly out???.xml out???.ps out???.pdf out???.png out???.mid \
             ex???.fms ex???.ly ex???.xml ex???.ps ex???.pdf ex???.png \
             tut???.fms tut???.ly tut???.xml tut???.ps tut???.pdf tut???.png \
             sco???.fms sco???.ly sco???.xml sco???.ps sco???.pdf sco???.png \
             out???.ly.log out???.xml.log out???.mid.log \
             ex???.ly.log ex???.xml.log \
             tut???.ly.log tut???.xml.log \
             sco???.ly.log sco???.xml.log \
             fms???.fms lya???.ly lyb???.ly lya???.ps lyb???.ps lya???.png lyb???.png lyc???.ly lyd???.ly \
             $(top_builddir)/check.html $(top_builddir)/checkdocs.html testreadwrite.fms testout1.fms \
             testout2.fms testout1a.fms testout2a.fms testhome/.fomus testout3.fms testout3a.fms \
             *-page?.png

clean-local:
	-rm -rf testhome

.PHONY: check-parse check-tests check-outfiles check-docs check-lisp
