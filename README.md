FOMUS
-------------
This is a build that should work with modern versions of boost with uniform code formatting applied.
If you are building on gentoo, you may need to put in a -ltinfo next to the -lcurses/-lncurses in the src/exe Makefile when building.
Otherwise, a simple ./configure --prefix /home/user && make && make install should do the trick.

The below is kept for posterity. 

Huge thanks go to Virtualdog on the SCSynth forums for helping me get this up and running.


----------------------



FOMUS Installation and Usage
****************************

Copyright (C) 2009 David Psenicka

FOMUS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

FOMUS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.  If not, see
<http://www.gnu.org/licenses/>.

1 Installing
************

1.1 Binary Installers
=====================

   Binary installers are available for OS X and Windows.  64-bit binaries aren't available yet but will be available
soon (the 32-bit version will run on a 64-bit machine, though programs compiled in 64-bit mode won't be able to use
it.)

   Windows binaries are currently being built on a Windows XP platform.  So far it's known that they run in both XP
and Vista.

1.2 Installing from a Source Tarball
====================================

1.2.1 Source Install Requirements
---------------------------------

   To install from a source tarball, your computer must have a C++ compiler such as GCC or Apple's Xcode and recent
versions of the following:

   * Boost C++ Libraries (`http://www.boost.org/')

   * LilyPond (optional but recommended, `http://lilypond.org/')

   To install these, follow the instructions for each individual package or use a package manager such as Fink,
MacPorts or Cygwin.

1.2.2 Downloading
-----------------

   Download the latest source tarball at `http://fomus.sourceforge.net/' and unpack it with an archive manager
application or type the following at a command prompt:

     cd PATH/TO/INSTALLFROM/DIRECTORY
     tar -xzf PATH/TO/FOMUS/TARBALL.tar.gz

1.2.3 Compiling and Installing from Source
------------------------------------------

   Type the following at a command prompt:

     cd PATH/TO/FOMUS/SOURCE/DIRECTORY
     ./configure
     make
     make install

   By default, FOMUS is installed to `/usr/local/'.  To change this, add `--prefix=/PATH/TO/BASE/INSTALL/DIRECTORY' to
the `./configure' line.  If you install to a nonstandard directory, you might need to add the path to FOMUS's
executable to your `PATH' variable.  If you have more than one core or processor, you can speed up the build process
by adding `-j2' (or a higher number) to the `make' command.

   If you are updating to a newer version, make sure you uninstall the previous version first.  See the section below
on uninstalling.

   To install documentation, type:

     cd PATH/TO/FOMUS/SOURCE/DIRECTORY
     make html
     make install-html

1.2.4 Installing on Cygwin
--------------------------

   You must compile and install Boost version 1.40 or greater before building FOMUS (the version of Boost that Cygwin
offers is too old).  After untarring the Boost sources and before building anything, edit the `boost/config/user.hpp'
file in the Boost source directory and add the following line anywhere in the file:

     #define BOOST_CYGWIN_PATH

   Setting this macro causes FOMUS to run properly using both Cygwin UNIX-style and Windows pathnames.

1.2.5 Updating
--------------

   Update the software by repeating the download and `make' instructions above.  Before you do so, make sure you
uninstall the application first to insure that any old modules are deleted and won't get in the way of the new install.
See the next section for instructions on uninstalling.

1.2.6 Uninstalling
------------------

   Uninstall FOMUS with the following commands:

     cd PATH/TO/FOMUS/SOURCE/DIRECTORY
     make uninstall

1.3 Installing from the SVN Repository
======================================

   The SVN install is a bit more complicated to compile and install than the source tarball.

1.3.1 SVN Install Requirements
------------------------------

   This SVN source code is much more complicated than the source install described above.  You should only try to
build this if you want to build everything completely from scratch.  Be prepared to run into a few glitches,
especially when building the documentation.

   The development build requires that several more dependencies be installed first.  You must download the source
using Subversion as described above.  Your computer must also have a C++ compiler such as GCC or Apple's Xcode and
recent versions of the following:

   * Autoconf, Automake, Libtool and Texinfo (`http://www.gnu.org/software/')

   * The Autoconf Archive (`http://www.nongnu.org/autoconf-archive/')

   * Boost C++ Libraries (`http://www.boost.org/')

   * SWIG (`http://www.swig.org/')

   * ImageMagick (for documentation build, `http://www.imagemagick.org/')

   * Graphviz (for documentation build, `http://www.graphviz.org/')

   * SBCL (with CFFI, for documentation build, `http://sbcl.sourceforge.net/', `http://common-lisp.net/project/cffi/')

   * Grace (with cm, for documentation build, `http://commonmusic.sourceforge.net/')

   * LilyPond (optional but recommended, for documentation build and regression test, `http://lilypond.org/')

   * libxml2 (with xmllint, for regression test, `http://www.xmlsoft.org/')

   To install these, follow the instructions for each individual package or use a package manager such as Fink,
MacPorts or Cygwin.  The following commands must be in your path and executable from the command line: `lilypond',
`swig', `convert', `dot', `sbcl', `cm' and `xmllint' as well as all of the autoconf and texinfo tools and the C
compiler.  Many of these dependencies are required by the documentation build and regression test.  If you don't plan
on building these, then only `swig', the autoconf tools and C compiler needs to be in your path.

1.3.2 Repository Checkout
-------------------------

   To checkout the sources from the SVN repository type the following at a command prompt:

     cd PATH/TO/INSTALLFROM/DIRECTORY
     svn co https://fomus.svn.sourceforge.net/svnroot/fomus/trunk fomus

   This creates a `fomus' directory and downloads the latest development source tree into it.

   You can also checkout releases snapshots, which are much more stable and reliable than the trunk.  The following
command lists all of the releases currently stored in the repository:

     svn ls https://fomus.svn.sourceforge.net/svnroot/fomus/tags

   After you've selected the version you want, the command to check it out is then:

     cd PATH/TO/INSTALLFROM/DIRECTORY
     svn co https://fomus.svn.sourceforge.net/svnroot/fomus/tags/RELEASEVERSION fomus

1.3.3 Compiling and Installing from the SVN Repository
------------------------------------------------------

   Before compiling, you must run Autoconf and related tools first.  There is a shell script called `bootstrap' that
does this for you and reports whether or not it succeeded.

     cd PATH/TO/FOMUS/SOURCE/DIRECTORY
     ./bootstrap

   If the Autoconf Archive doesn't come with your system or distribution then download/unpack it somewhere and add the
path to the `m4' directory (located inside the archive) to the `./bootstrap' command.  If you've unpacked the archive
in `/usr/local/src', for example, you should then type something like this in place of what is shown above:

     ./bootstrap /usr/local/src/autoconf-archive-2008-11-07/m4

   If that doesn't work, use the following commands at a command prompt:

     cd PATH/TO/FOMUS/SOURCE/DIRECTORY
     aclocal
     libtoolize
     automake -a
     autoreconf

   If you've untarred the Autoconf Archive somewhere then type something similar to the following instead:

     cd PATH/TO/FOMUS/SOURCE/DIRECTORY
     aclocal -I /usr/local/src/autoconf-archive-2008-11-07/m4
     libtoolize
     automake -a
     ACLOCAL="aclocal -I /usr/local/src/autoconf-archive-2008-11-07/m4" autoreconf

   It is okay if `aclocal', `libtoolize' and `automake' complain about missing files, etc..  The final `autoreconf'
command should run cleanly without any output.  On Mac OS X, libtool is prefixed with the letter `g' (e.g., type
`glibtoolize' instead of `libtoolize').  After this you will only need to type `autoreconf' to repeat this step
(including the `ACLOCAL' environment variable if necessary).  However, this probably won't be necessary since `make'
should take care of any necessary autoreconfing.

   At this point, follow the instructions for the compiling and installing from the source tarball above.

1.3.4 Updating SVN Installation
-------------------------------

   To update the source tree, type:

     cd PATH/TO/FOMUS/SOURCE/DIRECTORY
     make uninstall
     svn up

   It is important to uninstall FOMUS first to insure that any old modules are deleted and won't get in the way of the
new install.  After updating the source files type `make' and `make install' to compile and reinstall the updated
sources.

1.4 Regression Tests
====================

   To run the regression test suite you must have FOMUS, SBCL, LilyPond and libxml2 installed and the `fomus', `sbcl',
`lilypond' and `xmllint' commands in your path.  For `xmllint' to work, you must either be connected to the internet
or must download the MusicXML DTDs and set up a catalog file that points `xmllint' to your local `partwise.dtd' file.

   The command is:

     cd PATH/TO/FOMUS/SOURCE/DIRECTORY
     make installcheck

   If successful, this will create two HTML files named `check.html' and `checkdocs.html' in the source directory.
Open these in a web browser to compare test files to their outputs and verify that FOMUS is running correctly.  The
install check also prints out a summary of the results, which includes several other test suites in addition to the
two mentioned above.

   On my machine, I have the MusicXML DTD files stored in `/home/david/local/share/musicxml/' and a catalog file in
the same directory simply called `catalog.xml'.  The contents of `catalog.xml' are:

     <?xml version="1.0"?>
     <!DOCTYPE catalog PUBLIC "-//OASIS//DTD Entity Resolution XML Catalog V1.0//EN"
       "http://www.oasis-open.org/committees/entity/release/1.0/catalog.dtd">
     <catalog xmlns="urn:oasis:names:tc:entity:xmlns:xml:catalog">
       <public publicId="-//Recordare//DTD MusicXML 2.0 Partwise//EN"
         uri="/home/david/local/share/musicxml/partwise.dtd"/>
     </catalog>

   Then in my `.profile' file I have

     export XML_CATALOG_FILES="/etc/xml/catalog /home/david/local/share/musicxml/catalog.xml"

2 Usage
*******

2.1 Input
=========

2.1.1 Command Line
------------------

   The command for running fomus at the command prompt is `fomus'.  To get help on command-line options, type:

     fomus -h

   To process a `.fms' file, type:

     fomus PATH/TO/FILE.fms

   The `.fms' file might have its output file set to an unexpected location or may not output the type of file you're
interested in.  To override this, specify an output file also by typing:

     fomus -oPATH/TO/OUTPUT.ly PATH/TO/FILE.fms

   The `-S' option prints out a list of FOMUS's settings.  Two useful additional options to add to this are `-u',
which changes the so-called "use level," and `-n', which specifies the number of settings to return if you are
searching for a specific one.  Use levels range from 0 (beginner) to 3 (guru).  Levels 0 through 2 contain all of the
settings that should be useful to most users while level 3 settings require some amount of technical knowledge to use.

   For example, to view all of the documentation for "intermediate" (and "beginner") level settings  type:

     fomus -S -u1

   To search for settings that might have something to do with double accidentals, type:

     fomus -S -u2 -n3 double-acc

   This gives you the top three matches for settings (up to "advanced" level) that have names similar to `double-acc'.
You can also add `-d' to the command line to search documentation strings instead (rather than names).  The `-h'
option lists all of the possibilities for searching and filtering lists of settings, marks and modules.

   See the FOMUS HTML documentation for more information on `.fms' files, settings, marks and modules.

2.1.2 Emacs
-----------

   An Emacs FOMUS mode is installed in the `/INSTALL_PREFIX/shared/emacs/site-lisp' directory (the exact location
depends on the build prefix you specified in the `./configure' line).  It offers customizable syntax highlighting for
`.fms' files and a few commands for processing them.  To use it, put the following in your `.emacs' initialization
file:

     (add-to-list 'load-path "/INSTALL_PREFIX/share/emacs/site-lisp") ; (if necessary)
     (require 'fomus)

2.1.3 Common Music and Grace
----------------------------

   Grace contains a built-in graphical front-end for FOMUS, with several dialog boxes for editing settings and objects
as well as `.fms' data files.  To use the Grace front-end, create a new score and rename it to the output file you
wish to generate (select the `Rename Score' menu option).  Renaming a score is the same as specifying an output
filename via `-o' on the command line or changing the value of the `filename' setting.  Note events are entered using
functions that are prefixed with a `fms:' package symbol.  See the examples in the FOMUS or Grace documentation for
more information on how to use them.

2.1.4 Pure Data
---------------

   If the appropriate Pd header files are installed on your system, `make' will also build a Pure Data `fomus' patch
object.  Rather than guessing the proper location to install this, it is instead placed in the
`/INSTALL_PREFIX/share/fomus' directory.  Either link/copy this to the appropriate directory or add the directory to
Pd's search path.  An example patch file `fomus.pd' is also installed to the same location.

2.1.5 Lisp
----------

   Two files, `fomus.lisp' and `fomus.asd', are provided for using FOMUS in a Lisp environment such as SBCL, CLISP or
Clozure CL.  They are located in `/INSTALL_PREFIX/share/fomus'.  The CFFI package is required for `fomus.lisp' to
compile and load, so make sure this is installed on your system.  If you use ASDF to compile and load Lisp packages,
simply create a link to `file.asd' in your ASDF system directory or make sure `/INSTALL_PREFIX/share/fomus/' is
included in the `asdf:*central-registry*' variable.  You can then type `(asdf:operate 'asdf:load-op :fomus)' in Lisp
to compile and load FOMUS.  Evaluate `(fms:version)' to test if the FOMUS library is loaded correctly (FOMUS should
return a version string).

2.1.6 Importing Data
--------------------

   FOMUS supports importing type 0 or 1 MIDI files.  Although FOMUS attempts to make correct decisions based on
information contained in the file, you will most likely need to provide FOMUS with part and instrument definitions,
measures, and also a time scale factor for this to work.  See the HTML documentation for examples of how to do this.

2.2 Output
==========

2.2.1 LilyPond
--------------

   FOMUS can output LilyPond `http://lilypond.org/' files for processing into PostScript or PDF format.  If you
specify that FOMUS output an `.ly' file and the `lilypond' command is in your path then FOMUS automatically invokes
lilypond for you, generating a `.pdf' output file.  It should also automatically open a viewer application afterwards
to show you the results.  Unless you are running Linux, this should probably work out of the box (Acrobat Reader must
be installed in the default location in Windows).  If one or both of these two things doesn't happen, the following
two lines should fix this if they are placed in the `.fomus' initialization file in your home directory:

     lily-exe-path = "PATH/TO/LILYPOND_APP"
     lily-view-exe-path = "PATH/TO/MY_PDF_VIEWER_APP"

2.2.2 Finale and Sibelius
-------------------------

   To send output to either of these two commercial applications, specify an output file with an `.xml' extension.
This generates a MusicXML file which can be loaded using the Dolet plugin or a special import XML menu selection.  See
the MusicXML web site at `http://www.recordare.com/' for a list of other software packages that import MusicXML.

2.2.3 MIDI
----------

   FOMUS also outputs standard MIDI (type 1) files.  It attempts to produce a file that accurately reflects the score
on listening but is not necessarily useful as a means of importing into other programs.  Each voice and percussion
part in the score is sent to a separate MIDI channel.  FOMUS adds additional MIDI ports to work around the 16 channel
limitation, so if you have many parts it is necessary to use a MIDI player that understands multiple ports.

2.3 Initialization File
=======================

   When FOMUS starts up, it searches for an initialization file named `.fomus' in your home directory.  You must
create yourself if it doesn't already exist.  Use this file to modify FOMUS's default behavior by changing setting
values.  The syntax is flexible, and can be one of `setting = value', `setting: value' and `setting value'.  Examples
of some useful initialization file settings are:

     // Set the documentation use level to 0, the level for total newbies.
     use-level = 0

     // Set verbosity to 2, the highest value (0 is the lowest).
     verbose = 2

     // Set the path to the LilyPond executable, so FOMUS automatically processes
     // LilyPond output files.
     lily-exe-path = "/usr/bin/lilypond"

     // Set the path to a .pdf viewer application, so FOMUS automatically displays
     // the results.
     lily-view-exe-path = "evince"
     // A Windows example:
     lily-exe-path = "C:\\Program Files\\Adobe\\Reader 9.0\\Reader\\AcroRd32.exe"

     // If you don't specify an output filename anywhere, this will be the default.
     filename = "/tmp/out.ly"

     // Always produce a .fms, .xml and .ly file.
     output = (fms xml ly)

   This is also a good place for adding extensions to FOMUS's instrument and percussion instrument libraries.

     inst-defs += (<id my-special-inst ...> <id another-special-inst ...> ...)

     percinst-defs += (<id my-special-percinst ...> <id another-special-percinst ...> ...)

     layout-defs += (my-ensemble (kazoo ukulele glockenspiel organ))

2.4 Environment Variables
=========================

   TODO

