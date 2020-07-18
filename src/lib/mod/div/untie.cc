/*
    Copyright (C) 2009, 2010, 2011, 2012, 2013  David Psenicka
    This file is part of FOMUS.

    FOMUS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FOMUS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/* untie.cc

   runs after the `divide' module and "unsplits" notes to make them easier to
   read.  the results are a bit unconventional and appropriate for music
   involving complex rhythms and/or many notes beginning/ending on offbeats. */

// ---------------------------------------- headers
#include <algorithm>
#include <boost/ptr_container/ptr_deque.hpp>
#include <cassert>
#include <vector>

#include "debugaux.h"
#include "ifacedumb.h" // include the API for the built-in `dumb' engine
#include "module.h"    // include the API for FOMUS modules
#include "modutil.h"   // includ the API for various utility functions

namespace untie // tuck away things that don't need to be exported
{

  // ---------------------------------------- setting ids
  int dur_range_id, untie_id;

  // ---------------------------------------- data structures
  /* stores a note object along with a flag indicating whether or not to remove
   * the note. */
  struct note {
    module_noteobj n;
    bool remove; // remove this note?
    note(module_noteobj n) : n(n), remove(false) {}
    /* in all modules note objects must be given assignments in the order than
       they were received from `module_nextnote()'.  each module has one or more
       special assign functions that it can use (this module uses
       `divide_assign_unsplit()'.  if no assignment is to be made on a note
       object then `module_skipassign()' must be called on it.  once an
       assignment has been made, the note object is free for access by other
       threads.  at this point it cannot be used in calls to
       `module_peeknextnote()', though it can
       be passed to other functions such as `module_time()'. */
    void assign() {
      /* `divide_assign_unsplit()' removes the note object that is passed to it
         and adjusts the duration of the note that is tied to its left.  actions
         on assignments that involve adding or removing notes are always
         deferred until after the module has finished its processing. */
      if (remove)
        divide_assign_unsplit(
            n); // special "unsplit" assign function for divide modules
      else
        module_skipassign(n); // skip assignment for this note
    }
  };

  // ---------------------------------------- typedefs
  typedef std::vector<note> notes_vector;
  /* boost's `ptr_deque' stores pointers to objects and automatically deletes
     them when they are erased or the container is cleared. */
  typedef boost::ptr_deque<notes_vector> chords_deque;

  // ---------------------------------------- auxiliary functions
  /* processes a chunk of note objects that have been organized into chords and
     identified as a "group" that belongs together.  `chords' is a deque filled
     with vectors (representing chords, which are filled with note objects).
     `upto' is an iterator marking one past the last chord in the deque to be
     processed.  `remove' flags on all appropriate notes are set, then all
     assignments are made and chords are popped from the front of the deque. */
  void process(chords_deque& chords, chords_deque::iterator upto) {
    for (chords_deque::iterator i(chords.begin()); i != upto;
         ++i) { // i = a chord
      /* check to see if `untie' setting is turned on for this note */
      assert(module_setting_val(i->front().n, untie_id).type ==
             module_int); // we're expecing an integer (boolean)
      if (!module_setting_ival(i->front().n, untie_id))
        continue;
      /* get the `untie-dur-range' setting value. */
      assert(module_setting_val(i->front().n, dur_range_id).type ==
             module_list); // we're expecting this to be a list of length 2
      struct module_list range =
          module_setting_val(i->front().n, dur_range_id)
              .val.l; // get `untie-dur-range' setting value from note
      assert(range.n == 2);
      /* iterate through all spans of two or more tied chords, starting with the
         largest span and working towards the smallest. */
      for (chords_deque::iterator j(upto - 1); j != i;
           --j) { // j = a chord (iterating backwards from upto - 1 to i + 1)
        /* find the total duration of all tied notes, adjusting to compensate
         * for any tuplets that are present */
        fomus_rat dur = module_beatstoadjdur(
            i->front().n,
            module_endtime(j->front().n) - module_time(i->front().n), -1);
        if (dur < range.vals[0])
          break; // if span is too small, break out of loop and continue to next
                 // i
        if (dur > range.vals[1])
          continue; // if span is too large, continue to smaller span
        if (isexpof2(
                dur)) { // only untie chords that span a valid note duration
          /* found a group of two or more chords that can be untied.  iterate
             through all note objects to the
             right of the first tied chord and set their `remove' flags to true.
           */
          for (chords_deque::iterator k(i + 1), ke(j + 1); k != ke; ++k) {
            notes_vector& chord = *k;
            for (notes_vector::iterator l(chord.begin()); l != chord.end(); ++l)
              l->remove = true; // set `remove' flag
          }
          i = j; // jump to last chord before continuing with next i iteration
          break;
        }
      }
    }
    /* send assignments to FOMUS in the order that data was received and delete
     * everything we just processed. */
    while (chords.begin() != upto) {
      notes_vector& chord = chords.front();
      for (notes_vector::iterator i(chord.begin()); i != chord.end(); ++i)
        i->assign();      // do assignments
      chords.pop_front(); // delete the chord
    }
  }

  /* test for the beginning or end of a tuplet at any nested tuplet level. */
  bool tupbound_right(module_noteobj n) {
    /* we need to loop through all possible nested levels to determine if there
     * is a tuplet right boundary somewhere. */
    for (int l = 0;; ++l) { // l = nested tuplet level
      if (module_tuplet(n, l) == (fomus_int) 0)
        return false; // no more nested tuplets at this level
      if (module_tupletend(n, l))
        return true;
    }
  }
  bool tupbound_left(module_noteobj n) {
    for (int l = 0;; ++l) {
      if (module_tuplet(n, l) == (fomus_int) 0)
        return false;
      if (module_tupletbegin(n, l))
        return true;
    }
  }

  // ---------------------------------------- main functions
  /* this is the main entry point for the module.  the `dumb' engine simply
     passes control over to this function.  note objects are received from FOMUS
     and organized into chords.  when a rest or an untied note or tuplet
     begin/end is encountered, all chords collected so far are considered as a
     "group" and sent to the `process()' function above for processing.  `fom'
     is a handle to the entire score instance created by the user (and is rarely
     needed).  `moddata' is a pointer to a data structure created by
     `module_newdata()' below. modules instances should only read/write to their
     own data structures and not use
     global variables since multiple threads may be running concurrently. */
  extern "C" void run(FOMUS fom, void* moddata) {
    fomus_rat t1 = {
        -1,
        1}; // t = onset time of last note (used to separate notes into chords)
    fomus_rat t2 = {
        -1,
        1}; // t = onset time of last note (used to separate notes into chords)
    chords_deque chords;     // deque of chords
    bool ready = false;      // ready to process?
    bool norighttie = false; // encountered note with no right tie?
    /* loop, calling `module_nextnote()' repeatedly and finishing when FOMUS
     * returns NULL instead of a note object. */
    while (true) {
      module_noteobj n = module_nextnote(); // get the next note
      if (!n) { // if module_nextnote returns 0 then we're done
        process(chords, chords.end()); // process remaining notes
        return;
      }
      /* if note skips over a rest then we have a point where all the notes
         we've collected can be grouped and processed.  so process everything up
         to this point and continue looping.
       */
      if (module_time(n) > t2) {
        process(chords, chords.end());
        ready = norighttie = false; // reset flags
      }
      /* if the note belongs to a new chord, then create a new chord and stick
       * it onto the deque. */
      if (module_time(n) > t1) { // if note belongs in new chord then create it
        chords.push_back(new notes_vector); // push new chord onto deque
        /* if we've encountered notes with no right tie, then we're ready to
         * process. */
        if (norighttie)
          ready = true;
        norighttie = false;
        t1 = module_time(n); // update t1
      }
      t2 = std::max(t2, module_endtime(n));
      /* push the note onto the last chord in the deque. */
      assert(
          !chords
               .empty()); // chords should have at least one chord at this point
      chords.back().push_back(note(n)); // insert note into chord
      /* if this note has no right tie or is at a tuplet boundary on the right
         side, then soon we're ready to process.
         set `norighttie' so that we know this when we get to the next chord. */
      if (!module_istiedright(n) || tupbound_right(n))
        norighttie = true;
      /* if we're ready to process (or have no left tie or are at a tuplet
       * boundary on the left side) then do it. */
      if (ready || !module_istiedleft(n) || tupbound_left(n)) {
        process(chords,
                chords.end() - 1); // process everything up to the last chord
        ready = false;
      }
    }
  }

  /* this callback function must return an error string if an error has
     occurred. the string is printed as part of an error message to the user and
     indicates to FOMUS that this module has failed.  since this module should
     never fail doesn't need to report errors, it always returns NULL. */
  extern "C" const char* err(void* moddata) {
    return 0; // this module doesn't report errors
  }

  // ---------------------------------------- setting values/functions
  /* data used by `module_get_setting()' below. */
  const char* dur_range_type = "(number>=0 number>=0)";
  int dur_range_isvalid(const struct module_value val) {
    return module_valid_listofnums(
        val, 2, 2, // the list must have a minimum and maximum size of 2
        module_makeval((fomus_int) 0), module_incl, // value must be >= 0
        module_makeval((fomus_int) 0),
        module_nobound,  // value has no upper bound
        0,               // no function to validate separate list elements
        dur_range_type); // type string is passed for error messages
  }

} // namespace untie

using namespace untie;

// ---------------------------------------- callback functions
/* FOMUS expects to find these callback functions in every module that it loads.
 * the declarations are in module.h. */

// --- info
/* these return the long name, author and documentation strings.  the short name
   is the filename of the module itself. */
const char* module_longname() {
  return "Untie";
}
const char* module_author() {
  return "(fomus)";
}
const char* module_doc() {
  return "Unties notes to make complex notation easier to read.";
}

// --- initialization
/* `module_init()' is called once when the module is first loaded.
   `module_free()' is called once when it is unloaded. */
void module_init() {}
void module_free() {}
/* `module_ready()' is called once after all modules have been loaded and
   queried for settings.  this function can be used to confirm any dependencies
   that it might have (by looking for settings in other modules) and store
   setting ids into global variables for fast access. */
void module_ready() {}
/* this callback is used only during initialization and freeing.  it should
   return a string if something has failed (e.g., if the `module_ready()'
   function didn't find the settings or dependencies that this module needs). */
const char* module_initerr() {
  return 0; // initialization never fails
}
/* once the module returns a data structure with `module_newdata()', this error
 * function is called instead. */
const char* module_err(void* data) {
  return 0;
}
/* this is called during scheduling and tells FOMUS if two separate module
   instances can be considered equivalent.  this is necessary because FOMUS
   needs to create multiple instances and do a lot of juggling around if the
   user decides to activates different modules in different sections of the
   score.  two instances might not be equivalent, for example, if the user
   specifies the `bfsearch' engine in one instance and the `dynprog' engine in
   the other or the user has specified a different module for determining the
   distance between notes (in either of these cases, the algorithm changes and
   this module behaves differently).  any of the `module_setting' functions can
   be called on the module objects passed to this function. */
int module_sameinst(module_obj a, module_obj b) {
  return true;
}

/* the module type determines the purpose of the algorithm, when it gets
   scheduled, and which assign functions it can use. */
enum module_type module_type() {
  return module_moddivide;
}
/* the iteration type tells FOMUS what notes to send this module. we only need
   to operate on one measure at a time, and
   only want to process notes (ignoring rests and grace notes) that are in the
   same voice. */
int module_itertype() {
  return module_bymeas | module_byvoice | module_norests | module_nograce;
}
/* the priority value forces this module to run either before or after other
   modules of the same type.  the default priority that most of FOMUS's modules
   are set to is 0.  1 forces this module to run immediately after FOMUS's
   `divide' module, which does all of the complicated splitting and dividing of
   notes.  when modules have the same priority, it is up to the user to
   determine the order that they run in. */
int module_priority() {
  return 1;
}

/* these functions are called for each module instance and are responsible for
   creating and destroying any data structures that are needed.  the module's
   code should only read/write to separate data structures since multiple
   instances may run simultaneously in separate threads. */
void* module_newdata(FOMUS f) {
  return 0;
}
void module_freedata(void* dat) {}

// --- engine
/* every module has to be driven by an engine.  the default built-in `dumb'
   module simply passes complete control over and waits until the module is
   finished. */
const char* module_engine(void*) {
  return "dumb";
}
/* since some engines are interchangeable (like `bfsearch' and `dynprog'), FOMUS
   also needs to know a unique engine "interface id" so that it can match the
   right engines with the right modules.  the header files for FOMUS's engines
   store this value in the `ENGINE_INTERFACEID' macro. */
int module_engine_iface() {
  return ENGINE_INTERFACEID;
}
/* the module must fill a data structure (defined in the engine's header file)
   with information the engine needs to drive the module.  for the `dumb'
   interface not much information is required.  the engine needs a pointer to
   data created by `module_newdata()', the address of the function called to
   pass control over to the module, and the address
   of an error function called once after the module has finished. */
void module_fill_iface(void* moddata, void* iface) {
  ((dumb_iface*) iface)->moddata = moddata;
  ((dumb_iface*) iface)->run = run;
  /* usually this function and `module_err()' should do the same thing.
     `module_err()' is called by FOMUS while `err' is called by the engine. both
     should check if an error has occurred and return an appropriate error
     string or NULL. */
  ((dumb_iface*) iface)->err = err;
}

// --- settings
/* this callback is used to query for all settings that this module uses.  the
   module might want to use settings from other modules, in which case it should
   use `module_ready()' to look for them and find their ids.  FOMUS calls the
   function multiple times, incrementing `n' by 1 each time until the function
   returns 0.  `set' is a pointer to a pre-initialized data structure that must
   be filled with information on a setting.  `id' is an id value for the
   setting which should be stored in a global variable for quick access later.
 */
int module_get_setting(int n, struct module_setting* set, int id) {
  switch (n) {
  case 0: {                        // first setting
    set->name = "untie-dur-range"; // set setting name  docscat{rhythmic}
    set->type = module_list_nums;  // set value type
    set->descdoc =                 // set documentation string
        "A list of two values specifying the minimum and maximum durations an "
        "\"untied\" note is allowed to span.  "
        "FOMUS unties notes that fall within this range to make the notation "
        "easier to read.  "
        "The results are a bit unconventional and appropriate for scores with "
        "complex rhythms and many notes beginning or ending on offbeats.  "
        "Change this setting to influence which notes FOMUS considers for "
        "untying.";
    set->typedoc = dur_range_type; // set type documentation string
    set->uselevel = 2;             // set use level
    set->loc = module_locnote;     // set location where setting may appear
    set->valid =
        dur_range_isvalid; // set function to test validity of setting value
    module_setval_list(&set->val, 2); // set default value to list of length 2
    struct module_list& x = set->val.val.l;
    module_setval_rat(x.vals + 0,
                      module_makerat(1, 2)); // set first value in list
    module_setval_rat(x.vals + 1,
                      module_makerat(3, 2)); // set second value in list
    dur_range_id = id;                       // save id in global variable
    break;
  }
  case 1: {                  // second setting
    set->name = "untie";     // set setting name  docscat{rhythmic}
    set->type = module_bool; // set value type
    set->descdoc =           // set documentation string
        "Whether or not FOMUS unties notes that can be combined into a single "
        "note.  "
        "FOMUS unties notes that fall within this range to make the notation "
        "easier to read.  "
        "The results are a bit unconventional and appropriate for scores with "
        "complex rhythms and many notes beginning or ending on offbeats.";
    // (use default `typedoc')
    set->uselevel = 2;         // set use level
    set->loc = module_locnote; // set location where setting may appear
    // (use default `valid')
    module_setval_int(&set->val, 0); // set default value false (0)
    untie_id = id;                   // save id in global variable
    break;
  }
  default:
    return 0; // return 0 when finished
  }
  return 1; // return 1 when there are more settings
}
