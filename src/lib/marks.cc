// -*- c++ -*-

/*
    Copyright (C) 2009, 2010, 2011  David Psenicka
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
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "marks.h"
#include "error.h"
#include "schedr.h"

namespace fomus {

  std::set<std::pair<enum module_markids, enum module_markids>>
      spec; // 1, spec2;

  marksvect markdefs; // destroys them

  std::map<std::string, markbase*, isiless> marksbyname;

#warning                                                                       \
    "mark tasks: 1 remove invalid marks (e.g. from rests), 2 remove most marks from invisible rests"

  inline void insmark(markbase* x) {
    markdefs.push_back(x);
    marksbyname.insert(
        std::map<std::string, markbase*, isiless>::value_type(x->getname(), x));
  }

  void modprop(int& props, const char wh) {
    switch (wh) {
    case 'r':
      props = (props & ~spr_cannotspanrests) | spr_canspanrests;
      break;
    case 'n':
      props = (props & ~spr_canspanrests) | spr_cannotspanrests;
      break;
    case '1':
      props = (props & ~spr_cannotspanone) | spr_canspanone;
      break;
    case 'm':
      props = (props & ~spr_canspanone) | spr_cannotspanone;
      break;
    case '-':
      props = (props & ~spr_cannottouch) | spr_cantouch;
      break;
    case '|':
      props = (props & ~spr_cantouch) | spr_cannottouch;
      break;
#ifndef NDEBUG
    default:
      assert(false);
#endif
    }
  }

  std::list<std::string> insspannms;
  void insspanmarks(const std::string& name1a, const spanmark& spma,
                    const std::string& name1b, const spanmark& spmb,
                    const contmark& cm, const bool do1m = true,
                    const bool dodapi = true) {
    std::set<std::string> used;
    static const char rn0[2] = {'r', 'n'};
    static const char sm0[2] = {'1', 'm'};
    static const char xx0[2] = {'-', '|'};
    static const char* flarr[3] = {rn0, sm0, xx0};
    const char** ae = flarr + sizeof(flarr) / sizeof(const char*);
    for (const char** a1 = flarr; a1 < ae; ++a1) {
      for (const char** a2 = flarr; a2 < ae; ++a2) {
        if (a1 == a2)
          continue;
        for (const char** a3 = flarr; a3 < ae; ++a3) {
          if (a3 == a2 || a3 == a1)
            continue;
          for (int i1 = 0; i1 <= 2; ++i1) {
            for (int i2 = (do1m ? 0 : 2); i2 <= 2; ++i2) {
              int i3e = (i1 >= 2 && i2 >= 2 ? 1 : 2); // don't allow no flags
              for (int i3 = (dodapi ? 0 : 2); i3 <= i3e; ++i3) {
                std::string x;
                int pr = spma.getprops();
                if (i1 < 2) {
                  x += (*a1)[i1];
                  modprop(pr, (*a1)[i1]);
                }
                if (i2 < 2) {
                  x += (*a2)[i2];
                  modprop(pr, (*a2)[i2]);
                }
                if (i3 < 2) {
                  x += (*a3)[i3];
                  modprop(pr, (*a3)[i3]);
                }
                if (used.insert(x).second) {
                  insspannms.push_back(name1a + x + "..");
                  insmark(spma.clone(insspannms.back().c_str(), pr));
                  insspannms.push_back(name1b + x);
                  insmark(spmb.clone(insspannms.back().c_str(), pr));
                  insspannms.push_back(".." + name1a + x + "..");
                  insmark(cm.clone(insspannms.back().c_str(), pr));
                }
              }
            }
          }
        }
      }
    }
  }

  void initmarks() {
    markdefs.clear();
    marksbyname.clear();
    insspannms.clear();

    // basic articulations
    insmark(new markbase(".", "Staccato articulation.", module_none, move_right,
                         markpos_notehead, "[.]")); // enum{staccato}
    insmark(new markbase("!", "Staccatissimo articulation.", module_none,
                         move_right, markpos_notehead,
                         "[!]")); // enum{staccatissimo}
    insmark(new markbase(">", "Accent articulation.", module_none, move_left,
                         markpos_notehead, "[>]")); // enum{accent}
    insmark(new markbase("-", "Tenuto articulation.", module_none, move_left,
                         markpos_notehead, "[-]")); // enum{tenuto}
    insmark(new markbase("^", "Martellato articulation.", module_none,
                         move_left, markpos_notehead, "[^]")); // enum{marcato}
    insmark(new markbase("/.",
                         "Mezzo staccato or portato (combined tenuto and "
                         "staccato mark) articulation.",
                         module_none, move_left, markpos_notehead,
                         "[/.]")); // enum{mezzostaccato}

    // general/common
    insmark(new markbase("0", "Harmonic symbol.", module_none, move_left,
                         markpos_prefabove, "[0]")); // enum{harm}
    insmark(new markbase("o", "Open string/unstopped symbol.", module_none,
                         move_left, markpos_prefabove, "[o]")); // enum{open}
    insmark(new markbase("+", "Stopped/left-hand pizzicato symbol.",
                         module_none, move_left, markpos_prefabove,
                         "[+]")); // enum{stopped}
    insmark(new markbase("snap", "Snap pizzicato symbol.", module_none,
                         move_left, markpos_prefabove, "[snap]")); // enum{snap}

    insmark(new tempomark(
        "tempo",
        "Tempo marking.  "
        "The string argument is placed above the staff at the location of the "
        "mark.  "
        "If a `*' character is found in the string, it is replaced by a note "
        "that represents the value of the `beat' setting in that measure.  "
        "If two `*' characters are found with a rational or mixed number "
        "between them, then this value overrides the value of `beat'.  "
        "If a `#' character is found in the string and a numerical argument is "
        "supplied, the `#' is replaced by that number.  "
        "For example, the string \"Allegro, * = #\" and a numerical argument "
        "of 120 translates to \"Allegro, quarter-note = 120\" "
        "(where `quarter-note' is an actual notated quarter note and the value "
        "of `beat' is 1/4 for that measure).  "
        "The string \"Allegro (*1/8* = #)\" and a numerical argument of 108 "
        "translates to \"Allegro (eighth-note = 108)\".  "
        "The string \"Allegro (*3/8* = #)\" and a numerical argument of 108 "
        "translates to \"Allegro (dotted-quarter-note = 108)\".  "
        "An empty string and a numerical argument of 60 translates to "
        "\"quarter-note = 60\" (assuming `beat' is set to 1/4).  "
        "To insure that this mark appears above the top staff, it should be "
        "defined in a mark event and not attached to a note.",
        module_stringnum, move_left, markpos_above,
        "[tempo string_text real>0]",
        sin_mustdetach | sin_ispartgroupmark)); // enum{tempo}

    // strings, harp, misc.
    insmark(new markbase("upbow", "Up-bow symbol.", module_none, move_left,
                         markpos_prefabove, "[upbow]")); // enum{upbow}
    insmark(new markbase("downbow", "Down-bow symbol.", module_none, move_left,
                         markpos_prefabove, "[downbow]")); // enum{downbow}
    insmark(new markbase("damp", "Dampen symbol (cross plus circle).",
                         module_none, move_left, markpos_prefmiddleorabove,
                         "[damp]")); // enum{damp}
    insmark(new markbase("snappizz", "Snap pizzicato symbol.", module_none,
                         move_left, markpos_prefabove,
                         "[snappizz]")); // enum{snappizz}

    // single note text marks
    insmark(new markbase(
        "salt",
        "Saltando text marking."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[salt]",
        sin_isdetach)); // enum{salt}
    insmark(new markbase(
        "ric",
        "Ricochet or jeté text marking."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[ric]",
        sin_isdetach)); // enum{ric}
    insmark(new markbase(
        "lv",
        "\"Let vibrate\" text marking."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[lv]",
        sin_isdetach)); // enum{lv}
    insmark(new markbase(
        "flt",
        "Fluttertongue text marking."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[flt]",
        sin_isdetach)); // enum{flt}
    insmark(new markbase(
        "slap",
        "\"Slap tongued\" text marking."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[slap]",
        sin_isdetach)); // enum{slap}
    insmark(new markbase(
        "breath",
        "\"Breath tone\" text marking."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[breath]",
        sin_isdetach)); // enum{breath}

    // pairs/groups
    insmark(new markbase(
        "pizz",
        "String pizzicato text marking.  "
        "To use, insert `pizz' markings on every note that is to be played "
        "pizzicato.  "
        "FOMUS then places \"pizz.\" and \"arco\" texts over the correct notes "
        "automatically.  "
        "(This behavior can be disabled if desired--see the 'mark-group-defs' "
        "and 'mark-groups' settings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefabove, "[pizz]",
        sin_isdetach)); // enum{pizz}
    insmark(new markbase(
        "arco",
        "String arco text marking.  You usually shouldn't specify this "
        "directly (see the `pizz' marking)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefabove, "[arco]",
        sin_isdetach)); // enum{arco}

    insmark(new markbase(
        "mute",
        "Mute text marking.  "
        "To use, insert `mute' markings on every note that is to be played "
        "with the mute on.  "
        "FOMUS then places \"con sord.\" and \"senza sord.\" texts over the "
        "correct notes automatically.  "
        "(This behavior can be disabled if desired--see the 'mark-group-defs' "
        "and 'mark-groups' settings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefabove, "[mute]",
        sin_isdetach)); // enum{mute}
    insmark(new markbase(
        "unmute",
        "Unmute text marking.  You usually shouldn't specify this directly "
        "(see the `mute' marking)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefabove, "[unmute]",
        sin_isdetach)); // enum{unmute}

    insmark(new markbase(
        "vib",
        "Vibrato text marking.  "
        "To use, insert `vib' markings on every note that is to be played with "
        "vibrato.  "
        "FOMUS then places \"vib.\" and \"non vib.\" texts over the correct "
        "notes automatically.  "
        "(This behavior can be disabled if desired--see the 'mark-group-defs' "
        "and 'mark-groups' settings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefabove, "[vib]",
        sin_isdetach)); // enum{vib}
    insmark(new markbase(
        "moltovib",
        "Molto vibrato text marking.  "
        "To use, insert `moltovib' markings on every note that is to be played "
        "molto vibrato.  "
        "FOMUS then places \"molto vib.\" and \"non vib.\" texts over the "
        "correct notes automatically.  "
        "(This behavior can be disabled if desired--see the 'mark-group-defs' "
        "and 'mark-groups' settings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefabove, "[moltovib]",
        sin_isdetach)); // enum{moltovib}
    insmark(new markbase(
        "nonvib",
        "Non vibrato text marking.  You usually shouldn't specify this "
        "directly (see the `vib' and `moltovib' markings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefabove, "[nonvib]",
        sin_isdetach)); // enum{nonvib}

    insmark(new markbase(
        "leg",
        "Legato text marking.  "
        "To use, insert `legato' markings on every note that is to be played "
        "legato.  "
        "FOMUS then places \"legato\" and \"non legato\" texts over the "
        "correct notes automatically.  "
        "(This behavior can be disabled if desired--see the 'mark-group-defs' "
        "and 'mark-groups' settings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[leg]",
        sin_isdetach)); // enum{legato}
    insmark(new markbase(
        "moltoleg",
        "Molto legato text marking.  "
        "To use, insert `molto legato' markings on every note that is to be "
        "played molto legato.  "
        "FOMUS then places \"molto legato\" and \"non legato\" texts over the "
        "correct notes automatically.  "
        "(This behavior can be disabled if desired--see the 'mark-group-defs' "
        "and 'mark-groups' settings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[moltoleg]",
        sin_isdetach)); // enum{moltolegato}
    insmark(new markbase(
        "nonleg",
        "Nonlegato text marking.  You usually shouldn't specify this directly "
        "(see the `leg' and `moltoleg' markings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[nonleg]",
        sin_isdetach)); // enum{nonlegato}

    insmark(new markbase(
        "spic",
        "Spiccato text marking.  "
        "To use, insert `spic' markings on every note that is to be played "
        "spiccato.  "
        "FOMUS then places \"spicc\" and \"ord.\" texts over the correct notes "
        "automatically.  "
        "(This behavior can be disabled if desired--see the 'mark-group-defs' "
        "and 'mark-groups' settings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[spic]",
        sin_isdetach)); // enum{spic}
    insmark(new markbase(
        "tall",
        "\"at the frog\" text marking.  "
        "To use, insert `tall' markings on every note that is to be played al "
        "tallone.  "
        "FOMUS then places \"al tallone\" and \"ord.\" texts over the correct "
        "notes automatically.  "
        "(This behavior can be disabled if desired--see the 'mark-group-defs' "
        "and 'mark-groups' settings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[tall]",
        sin_isdetach)); // enum{tall}
    insmark(new markbase(
        "punta",
        "\"at the tip\" text marking.  "
        "To use, insert `punta' markings on every note that is to be played "
        "punta d'arco.  "
        "FOMUS then places \"punta d'arco\" and \"ord.\" texts over the "
        "correct notes automatically.  "
        "(This behavior can be disabled if desired--see the 'mark-group-defs' "
        "and 'mark-groups' settings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[punta]",
        sin_isdetach)); // enum{punta}
    insmark(new markbase(
        "pont",
        "\"near the bridge\" text marking.  "
        "To use, insert `pont' markings on every note that is to be played sul "
        "ponticello.  "
        "FOMUS then places \"sul pont.\" and \"ord.\" texts over the correct "
        "notes automatically.  "
        "(This behavior can be disabled if desired--see the 'mark-group-defs' "
        "and 'mark-groups' settings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[pont]",
        sin_isdetach)); // enum{pont}
    insmark(new markbase(
        "tasto",
        "\"on the fingerboard\" text marking.  "
        "To use, insert `tasto' markings on every note that is to be played "
        "sul tasto.  "
        "FOMUS then places \"sul tasto\" and \"ord.\" texts over the correct "
        "notes automatically.  "
        "(This behavior can be disabled if desired--see the 'mark-group-defs' "
        "and 'mark-groups' settings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[tasto]",
        sin_isdetach)); // enum{tasto}
    insmark(new markbase(
        "legno",
        "\"with the wood\" text marking.  "
        "To use, insert `legno' markings on every note that is to be played "
        "col legno.  "
        "FOMUS then places \"col legno\" and \"ord.\" texts over the correct "
        "notes automatically.  "
        "(This behavior can be disabled if desired--see the 'mark-group-defs' "
        "and 'mark-groups' settings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[legno]",
        sin_isdetach)); // enum{legno}
    insmark(new markbase(
        "flaut",
        "\"near the fingerboard\" text marking."
        "To use, insert `flaut' markings on every note that is to be played "
        "flautando.  "
        "FOMUS then places \"flautando\" and \"ord.\" texts over the correct "
        "notes automatically.  "
        "(This behavior can be disabled if desired--see the 'mark-group-defs' "
        "and 'mark-groups' settings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[flaut]",
        sin_isdetach)); // enum{flaut}
    insmark(new markbase(
        "etouf",
        "Dampen text marking."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[etouf]",
        sin_isdetach)); // enum{etouf}
    insmark(new markbase(
        "table",
        "\"near the soundboard\" text marking."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[table]",
        sin_isdetach)); // enum{table}
    // bisbigliando?
    insmark(new markbase(
        "cuivre",
        "Cuivré (or brassy) text marking."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[cuivre]",
        sin_isdetach)); // enum{cuivre}
    insmark(new markbase(
        "bellsup",
        "\"Bells up\" text marking."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[bellsup]",
        sin_isdetach)); // enum{bellsup}

    insmark(new markbase(
        "ord",
        "Naturale/ordinario/normale text marking.  You usually shouldn't "
        "specify this directly (e.g., see the `pont' and `tasto' markings)."
        "  The precise text that appears in the score is controlled by the "
        "`default-marktexts' and `marktexts' settings.",
        module_none, move_left, markpos_prefmiddleorabove, "[ord]",
        sin_isdetach)); // enum{ord}

    insmark(new markbase("break<",
                         "System break, occurring at the barline at or after "
                         "the attack time of the note.",
                         module_none, move_left, markpos_notehead, "[break<]",
                         sin_isdetach)); // enum{break_before}
    insmark(new markbase("break>",
                         "System break, occurring at the barline at or after "
                         "the release time of the note.",
                         module_none, move_right, markpos_notehead, "[break>]",
                         sin_isdetach)); // enum{break_after}

    // misc.
    insmark(new markbase("ferm", "Fermata marking.", module_none, move_right,
                         markpos_prefabove,
                         "[ferm]")); // enum{fermata}  short, long, verylong
    insmark(new markbase(
        "ferm-short", "Short fermata marking.", module_none, move_right,
        markpos_prefabove,
        "[ferm-short]")); // enum{fermata_short}  short, long, verylong
    insmark(new markbase(
        "ferm-long", "Long fermata marking.", module_none, move_right,
        markpos_prefabove,
        "[ferm-long]")); // enum{fermata_long}  short, long, verylong
    insmark(new markbase(
        "ferm-verylong", "Very long fermata marking.", module_none, move_right,
        markpos_prefabove,
        "[ferm-verylong]")); // enum{fermata_verylong}  short, long, verylong

    insmark(
        new markbase("arp",
                     "Arpeggio.  Applies to all notes in the same chord and "
                     "only if a chord (two or more simultaneous notes) exists.",
                     module_none, move_left, markpos_notehead,
                     "[arp]")); // enum{arpeggio}  "", up, down
    insmark(new markbase(
        "arp^",
        "Upwards arpeggio.  Applies to all notes in the same chord and only if "
        "a chord (two or more simultaneous notes) exists.",
        module_none, move_left, markpos_notehead,
        "[arp^]")); // enum{arpeggio_up}  "", up, down
    insmark(new markbase(
        "arp_",
        "Downwards arpeggio.  Applies to all notes in the same chord and only "
        "if a chord (two or more simultaneous notes) exists.",
        module_none, move_left, markpos_notehead,
        "[arp_]")); // enum{arpeggio_down}  "", up, down

    insmark(new markbase(
        "gliss<",
        "Glissando sign, connecting to the previous note in the same voice.",
        module_none, move_left, markpos_notehead,
        "[gliss<]")); // enum{gliss_before}  before, after
    insmark(new markbase(
        "gliss>",
        "Glissando sign, connecting to the next note in the same voice.",
        module_none, move_right, markpos_notehead,
        "[gliss>]")); // enum{gliss_after}  before, after

    insmark(new markbase(
        "port<",
        "Portamento sign, connecting to the previous note in the same voice.",
        module_none, move_left, markpos_notehead,
        "[port<]")); // enum{port_before}  before, after
    insmark(new markbase(
        "port>",
        "Portamento sign, connecting to the next note in the same voice.",
        module_none, move_right, markpos_notehead,
        "[port>]")); // enum{port_after}  before, after

    insmark(new markbase("breath<", "A breath mark placed before the note.",
                         module_none, move_left, markpos_notehead,
                         "[breath<]")); // enum{breath_before}  before, after
    insmark(new markbase("breath>", "A breath mark placed after the note.",
                         module_none, move_right, markpos_notehead,
                         "[breath>]")); // enum{breath_after}  before, after

    // tremolos
    insmark(new longtrmark(
        "longtr",
        "The main note of a trill.  This should be used together with "
        "`longtr2' "
        "on two simultaneous notes to specify the entire trill.  "
        "The `FIXME' setting specifies whether trills greater than a single "
        "step are automatically notated as unmeasured tremolos instead "
        "(unison trills are always converted to tremolos).  Otherwise a small "
        "note in parentheses is used.",
        module_none, move_leftright, markpos_prefabove,
        "[longtr]")); // enum{longtrill}  before, after
    insmark(
        new markbase("longtr2",
                     "The auxiliary note of a trill, usually notated as an "
                     "accidental above or below the trill sign.  "
                     "This should be used together with `longtr' on two "
                     "simultaneous notes to specify the entire trill.  "
                     "The auxiliary note is deleted during FOMUS's processing, "
                     "so any additional marks should be placed in the "
                     "base `longtr' trill note.",
                     module_none, move_leftright, markpos_prefabove,
                     "[longtr2]")); // enum{longtrill2}  before, after

    insmark(new tremmark(
        "trem",
        "A tremolo.  The optional numeric argument indicates the duration of a "
        "single tremolo beat.  "
        "No argument indicates an unmeasured tremolo.  "
        "When used together with `trem2' on two sets of simultaneous "
        "pitches/chords, specifies a keyboard-style tremelo "
        "with `trem' indicating the first chord and `trem2' indicating the "
        "second.  "
        "The `FIXME' setting specifies whether unmeasured semitone/wholetone "
        "tremolos are automatically notated as trills instead.",
        module_number, move_leftright, markpos_notehead,
        "[trem rational>0]")); // enum{trem}  before, after
    insmark(new markbase("trem2",
                         "The second chord in a keyboard-style tremelo.  "
                         "This must be used with `trem'.",
                         module_none, move_leftright, markpos_notehead,
                         "[trem2]")); // enum{trem2}  before, after
    // harmonics
    insmark(new markbase(
        "natharm-sounding",
        "Indicates that the note is the sounding pitch in a natural harmonic.",
        module_none, move_leftright, markpos_notehead, "[natharm-sounding]",
        spr_dontspread)); // enum{natharm_sounding}  before, after
    insmark(new markbase(
        "artharm-sounding",
        "Indicates that the note is the sounding pitch in an artificial "
        "harmonic.",
        module_none, move_leftright, markpos_notehead, "[artharm-sounding]",
        spr_dontspread)); // enum{artharm_sounding}  before, after

    insmark(new markbase(
        "natharm-touched",
        "Indicates that the note is the place to touch on the string in a "
        "natural harmonic.",
        module_none, move_leftright, markpos_notehead, "[natharm-touched]",
        spr_dontspread)); // enum{natharm_touched}  before, after
    insmark(new markbase(
        "artharm-touched",
        "Indicates that the note is the place to touch on the string in an "
        "artificial harmonic.",
        module_none, move_leftright, markpos_notehead, "[artharm-touched]",
        spr_dontspread)); // enum{artharm_touched}  before, after

    insmark(new markbase("artharm-base",
                         "Indicates that the note is the base pitch to play on "
                         "the string in an artificial harmonic.",
                         module_none, move_leftright, markpos_notehead,
                         "[artharm-base]",
                         spr_dontspread)); // enum{artharm_base}  before, after
    insmark(new markbase(
        "natharm-string",
        "Indicates that the note is the pitch of the open string in a natural "
        "harmonic.",
        module_none, move_leftright, markpos_notehead, "[natharm-string]",
        spr_dontspread)); // enum{natharm_string}  before, after
    // various text
    insmark(new sulmark(
        "sul",
        "Mark for indicating which string to play on.  " // The number argument
                                                         // specifies the string
                                                         // (a note or string
                                                         // number).  "
        "The `sul-style' setting specifies how this is to be interpretted and "
        "printed in the score.",
        module_none, move_left, markpos_prefabove,
        "[sul integer0..128]")); // enum{sul}  before, after
    // dynamics
    insmark(new markbase("sf", "Sforzando dynamic marking.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[sf]",
                         sin_isdetach)); // enum{sf}
    insmark(new markbase("sff", "Double sforzando dynamic marking.",
                         module_none, move_left, markpos_prefmiddleorbelow,
                         "[sff]", sin_isdetach)); // enum{sff}
    insmark(new markbase("sfff", "Triple sforzando dynamic marking.",
                         module_none, move_left, markpos_prefmiddleorbelow,
                         "[sfff]", sin_isdetach)); // enum{sfff}
    insmark(new markbase("sfz", "Sforzato dynamic marking.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[sfz]",
                         sin_isdetach)); // enum{sfz}
    insmark(new markbase("sffz", "Double sforzato dynamic marking.",
                         module_none, move_left, markpos_prefmiddleorbelow,
                         "[sffz]", sin_isdetach)); // enum{sffz}
    insmark(new markbase("sfffz", "Triple sforzato dynamic marking.",
                         module_none, move_left, markpos_prefmiddleorbelow,
                         "[sfffz]", sin_isdetach)); // enum{sfffz}
    insmark(new markbase("fz", "Forzando dynamic marking.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[fz]",
                         sin_isdetach)); // enum{fz}
    insmark(new markbase("ffz", "Double forzando dynamic marking.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[ffz]",
                         sin_isdetach)); // enum{ffz}
    insmark(new markbase("fffz", "Triple forzando dynamic marking.",
                         module_none, move_left, markpos_prefmiddleorbelow,
                         "[fffz]", sin_isdetach)); // enum{fffz}
    insmark(new markbase("rfz", "Rinforzando dynamic marking.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[rfz]",
                         sin_isdetach)); // enum{rfz}
    insmark(new markbase("rf", "Rinforzando dynamic marking.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[rf]",
                         sin_isdetach)); // enum{rf}
    insmark(new markbase("pppppp", "Pianissississississimo dynamic level.",
                         module_none, move_left, markpos_prefmiddleorbelow,
                         "[pppppp]", sin_isdetach)); // enum{pppppp}
    insmark(new markbase("ppppp", "Pianississississimo dynamic level.",
                         module_none, move_left, markpos_prefmiddleorbelow,
                         "[ppppp]", sin_isdetach)); // enum{ppppp}
    insmark(new markbase("pppp", "Pianissississimo dynamic level.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[pppp]",
                         sin_isdetach)); // enum{pppp}
    insmark(new markbase("ppp", "Pianississimo dynamic level.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[ppp]",
                         sin_isdetach)); // enum{ppp}
    insmark(new markbase("pp", "Pianissimo dynamic level.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[pp]",
                         sin_isdetach)); // enum{pp}
    insmark(new markbase("p", "Piano dynamic level.", module_none, move_left,
                         markpos_prefmiddleorbelow, "[p]",
                         sin_isdetach)); // enum{p}
    insmark(new markbase("mp", "Mezzo-piano dynamic level.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[mp]",
                         sin_isdetach)); // enum{mp}
    insmark(new markbase("ffff", "Fortissississimo dynamic level.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[ffff]",
                         sin_isdetach)); // enum{ffff}
    insmark(new markbase("fff", "Fortississimo dynamic level.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[fff]",
                         sin_isdetach)); // enum{fff}
    insmark(new markbase("ff", "Fortissimo dynamic level.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[ff]",
                         sin_isdetach)); // enum{ff}
    insmark(new markbase("f", "Forte dynamic level.", module_none, move_left,
                         markpos_prefmiddleorbelow, "[f]",
                         sin_isdetach)); // enum{f}
    insmark(new markbase("mf", "Mezzo-forte dynamic level.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[mf]",
                         sin_isdetach)); // enum{mf}

    insmark(new markbase("fp", "Forte-piano dynamic marking.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[fp]",
                         sin_isdetach)); // enum{fp}
    insmark(new markbase("fzp", "Forzando-piano dynamic marking.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[fzp]",
                         sin_isdetach)); // enum{fzp}
    insmark(new markbase("sfp", "Sforzando-piano dynamic marking.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[sfp]",
                         sin_isdetach)); // enum{sfp}
    insmark(new markbase("sfzp", "Sforzato-piano dynamic marking.", module_none,
                         move_left, markpos_prefmiddleorbelow, "[sfzp]",
                         sin_isdetach)); // enum{sfzp}

    insmark(new markbase(
        "dyn",
        "An unknown dynamic marking.  The numeric dynamic value for the note "
        "event is scaled to a proper dynamic text mark "
        "using the settings `dyn-range' and `dynsym-range'.  "
        "An optional numeric argument overrides the note event's dynamic "
        "value.",
        module_number, move_leftright, markpos_prefmiddleorbelow,
        "[dyn number]", sin_isdetach)); // enum{dyn}

    spanmark* slur_begin;
    insmark(slur_begin = new spanmark(
                "(..",
                "Begin slur or bowing articulation." // enum{slur_begin}
                "  The `(' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `-' (the beginning and ends "
                "of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `(r..', `(-..' and `(r-..'.",
                module_none, m_voicebegin, spr_cannotspanone, move_left,
                markpos_notehead, "[(..]", 1, SLURCANTOUCHDEF_ID, 0,
                SLURCANSPANRESTSDEF_ID));
    spanmark* slur_end;
    insmark(slur_end = new spanmark(
                "..)",
                "End slur or bowing articulation." // enum{slur_end}
                "  The `)' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `-' (the beginning and ends "
                "of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `..)r', `..)-' and `..)r-'.",
                module_none, m_voiceend, spr_cannotspanone, move_right,
                markpos_notehead, "[..)]", 1, SLURCANTOUCHDEF_ID, 0,
                SLURCANSPANRESTSDEF_ID));
    contmark* slur_cont;
    insmark(slur_cont = new contmark(
                ".(.",
                "Continue slur or bowing articulation." // enum{slur_cont}
                "  FOMUS begins/ends slur marks where these continue marks "
                "start/stop."
                "  The `(' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `-' (the beginning and ends "
                "of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `.(r.', `.(-.' and `.(r-.'.",
                true, module_none, move_left, markpos_notehead, "[.(.]"));

    spanmark* dottedslur_begin;
    insmark(
        dottedslur_begin = new spanmark(
            "dot(..",
            "Begin dotted slur or bowing articulation." // enum{dottedslur_begin}
            "  The `(' may be followed by one or more of the following flag "
            "characters: `r' (the mark can span rests), "
            "`n' (the mark cannot span rests), `-' (the beginning and ends of "
            "the mark can touch) "
            "and `|' (the beginning and ends of the mark cannot touch).  "
            "Examples are `dot(r..', `dot(-..' and `dot(r-..'.",
            module_none, m_voicebegin, spr_cannotspanone, move_left,
            markpos_notehead, "[dot(..]", 1, SLURCANTOUCHDEF_ID, 0,
            SLURCANSPANRESTSDEF_ID));
    spanmark* dottedslur_end;
    insmark(
        dottedslur_end = new spanmark(
            "..dot)",
            "End dotted slur or bowing articulation." // enum{dottedslur_end}
            "  The `)' may be followed by one or more of the following flag "
            "characters: `r' (the mark can span rests), "
            "`n' (the mark cannot span rests), `-' (the beginning and ends of "
            "the mark can touch) "
            "and `|' (the beginning and ends of the mark cannot touch).  "
            "Examples are `..dot)r', `..dot)-' and `..dot)r-'.",
            module_none, m_voiceend, spr_cannotspanone, move_right,
            markpos_notehead, "[..dot)]", 1, SLURCANTOUCHDEF_ID, 0,
            SLURCANSPANRESTSDEF_ID));
    contmark* dottedslur_cont;
    insmark(
        dottedslur_cont = new contmark(".dot(.",
                                       "Continue dotted slur or bowing "
                                       "articulation." // enum{dottedslur_cont}
                                       "  FOMUS begins/ends slur marks where "
                                       "these continue marks start/stop."
                                       "  The `(' may be followed by one or "
                                       "more of the following flag characters: "
                                       "`r' (the mark can span rests), "
                                       "`n' (the mark cannot span rests), `-' "
                                       "(the beginning and ends of the mark "
                                       "can touch) "
                                       "and `|' (the beginning and ends of the "
                                       "mark cannot touch).  Examples are "
                                       "`.dot(r.', `.dot(-.' and `.dot(r-.'.",
                                       true, module_none, move_left,
                                       markpos_notehead, "[.dot(.]"));

    spanmark* dashedslur_begin;
    insmark(
        dashedslur_begin = new spanmark(
            "dash(..",
            "Begin dashed slur or bowing articulation." // enum{dashedslur_begin}
            "  The `(' may be followed by one or more of the following flag "
            "characters: `r' (the mark can span rests), "
            "`n' (the mark cannot span rests), `-' (the beginning and ends of "
            "the mark can touch) "
            "and `|' (the beginning and ends of the mark cannot touch).  "
            "Examples are `dash(r..', `dash(-..' and `dash(r-..'.",
            module_none, m_voicebegin, spr_cannotspanone, move_left,
            markpos_notehead, "[dash(..]", 1, SLURCANTOUCHDEF_ID, 0,
            SLURCANSPANRESTSDEF_ID));
    spanmark* dashedslur_end;
    insmark(
        dashedslur_end = new spanmark(
            "..dash)",
            "End dashed slur or bowing articulation." // enum{dashedslur_end}
            "  The `)' may be followed by one or more of the following flag "
            "characters: `r' (the mark can span rests), "
            "`n' (the mark cannot span rests), `-' (the beginning and ends of "
            "the mark can touch) "
            "and `|' (the beginning and ends of the mark cannot touch).  "
            "Examples are `..dash)r', `..dash)-' and `..dash)r-'.",
            module_none, m_voiceend, spr_cannotspanone, move_right,
            markpos_notehead, "[..dash)]", 1, SLURCANTOUCHDEF_ID, 0,
            SLURCANSPANRESTSDEF_ID));
    contmark* dashedslur_cont;
    insmark(
        dashedslur_cont = new contmark(".dash(.",
                                       "Continue dashed slur or bowing "
                                       "articulation." // enum{dashedslur_cont}
                                       "  FOMUS begins/ends slur marks where "
                                       "these continue marks start/stop."
                                       "  The `(' may be followed by one or "
                                       "more of the following flag characters: "
                                       "`r' (the mark can span rests), "
                                       "`n' (the mark cannot span rests), `-' "
                                       "(the beginning and ends of the mark "
                                       "can touch) "
                                       "and `|' (the beginning and ends of the "
                                       "mark cannot touch).  Examples are "
                                       "`.dash(r.', `.dash(-.' and "
                                       "`.dash(r-.'.",
                                       true, module_none, move_left,
                                       markpos_notehead, "[.dash(.]"));

    spanmark* phrase_begin;
    insmark(
        phrase_begin = new spanmark("((..",
                                    "Begin higher-level slur or phrase "
                                    "articulation.  " // enum{phrase_begin}
                                    "Use to indicate phrases marks over slurs "
                                    "created with `(..' and `..)' marks."
                                    "  The `((' may be followed by one or more "
                                    "of the following flag characters: `r' "
                                    "(the mark can span rests), "
                                    "`n' (the mark cannot span rests), `-' "
                                    "(the beginning and ends of the mark can "
                                    "touch) "
                                    "and `|' (the beginning and ends of the "
                                    "mark cannot touch).  Examples are "
                                    "`((r..', `((-..' and `((r-..'.",
                                    module_none, m_voicebegin,
                                    spr_cannotspanone, move_left,
                                    markpos_notehead, "[((..]", 2,
                                    SLURCANTOUCHDEF_ID, 0,
                                    SLURCANSPANRESTSDEF_ID));
    spanmark* phrase_end;
    insmark(phrase_end = new spanmark(
                "..))",
                "End higher-level slur or phrase articulation.  "
                "Use to indicate phrases over slurs created with `(..' and "
                "`..)' marks." // enum{phrase_end}
                "  The `))' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `-' (the beginning and ends "
                "of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `..))r', `..))-', and `..))r-'.",
                module_none, m_voiceend, spr_cannotspanone, move_right,
                markpos_notehead, "[..))]", 2, SLURCANTOUCHDEF_ID, 0,
                SLURCANSPANRESTSDEF_ID));
    contmark* phrase_cont;
    insmark(phrase_cont =
                new contmark(".((.",
                             "Continue higher-level slur or phrase "
                             "articulation." // enum{phrase_cont}
                             "  FOMUS begins/ends slur marks where these "
                             "continue marks start/stop."
                             "  The `((' may be followed by one or more of the "
                             "following flag characters: `r' (the mark can "
                             "span rests), "
                             "`n' (the mark cannot span rests), `-' (the "
                             "beginning and ends of the mark can touch) "
                             "and `|' (the beginning and ends of the mark "
                             "cannot touch).  Examples are `.((r.', `.((-.' "
                             "and `.((r-.'.",
                             true, module_none, move_left, markpos_notehead,
                             "[.((.]"));

    spanmark* dottedphrase_begin;
    insmark(dottedphrase_begin =
                new spanmark("dot((..",
                             "Begin higher-level dotted slur or phrase "
                             "articulation.  " // enum{dottedphrase_begin}
                             "Use to indicate phrases marks over slurs created "
                             "with `(..' and `..)' marks."
                             "  The `((' may be followed by one or more of the "
                             "following flag characters: `r' (the mark can "
                             "span rests), "
                             "`n' (the mark cannot span rests), `-' (the "
                             "beginning and ends of the mark can touch) "
                             "and `|' (the beginning and ends of the mark "
                             "cannot touch).  Examples are `dot((r..', "
                             "`dot((-..', and `dot((r-..'.",
                             module_none, m_voicebegin, spr_cannotspanone,
                             move_left, markpos_notehead, "[dot((..]", 2,
                             SLURCANTOUCHDEF_ID, 0, SLURCANSPANRESTSDEF_ID));
    spanmark* dottedphrase_end;
    insmark(dottedphrase_end =
                new spanmark("..dot))",
                             "End higher-level dotted slur or phrase "
                             "articulation.  " // enum{dottedphrase_end}
                             "Use to indicate phrases over slurs created with "
                             "`(..' and `..)' marks."
                             "  The `))' may be followed by one or more of the "
                             "following flag characters: `r' (the mark can "
                             "span rests), "
                             "`n' (the mark cannot span rests), `-' (the "
                             "beginning and ends of the mark can touch) "
                             "and `|' (the beginning and ends of the mark "
                             "cannot touch).  Examples are `..dot))r', "
                             "`..dot))-' and `..dot))r-'.",
                             module_none, m_voiceend, spr_cannotspanone,
                             move_right, markpos_notehead, "[..dot))]", 2,
                             SLURCANTOUCHDEF_ID, 0, SLURCANSPANRESTSDEF_ID));
    contmark* dottedphrase_cont;
    insmark(dottedphrase_cont =
                new contmark(".dot((.",
                             "Continue higher-level dotted slur or phrase "
                             "articulation." // enum{dottedphrase_cont}
                             "  FOMUS begins/ends slur marks where these "
                             "continue marks start/stop."
                             "  The `((' may be followed by one or more of the "
                             "following flag characters: `r' (the mark can "
                             "span rests), "
                             "`n' (the mark cannot span rests), `-' (the "
                             "beginning and ends of the mark can touch) "
                             "and `|' (the beginning and ends of the mark "
                             "cannot touch).  Examples are `.dot((r.', "
                             "`.dot((-.', and `.dot((r-.'.",
                             true, module_none, move_left, markpos_notehead,
                             "[.dot((.]"));

    spanmark* dashedphrase_begin;
    insmark(dashedphrase_begin =
                new spanmark("dash((..",
                             "Begin higher-level dashed slur or phrase "
                             "articulation.  " // enum{dashedphrase_begin}
                             "Use to indicate phrases marks over slurs created "
                             "with `(..' and `..)' marks."
                             "  The `((' may be followed by one or more of the "
                             "following flag characters: `r' (the mark can "
                             "span rests), "
                             "`n' (the mark cannot span rests), `-' (the "
                             "beginning and ends of the mark can touch) "
                             "and `|' (the beginning and ends of the mark "
                             "cannot touch).  Examples are `dash((r..', "
                             "`dash((-..' and `dash((r-..'.",
                             module_none, m_voicebegin, spr_cannotspanone,
                             move_left, markpos_notehead, "[dash((..]", 2,
                             SLURCANTOUCHDEF_ID, 0, SLURCANSPANRESTSDEF_ID));
    spanmark* dashedphrase_end;
    insmark(dashedphrase_end = new spanmark(
                "..dash))",
                "End higher-level dashed slur or phrase articulation.  "
                "Use to indicate phrases over slurs created with `(..' and "
                "`..)' marks." // enum{dashedphrase_end}
                "  The `))' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `-' (the beginning and ends "
                "of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `..dash))r', `..dash))-' and `..dash))r-'.",
                module_none, m_voiceend, spr_cannotspanone, move_right,
                markpos_notehead, "[..dash))]", 2, SLURCANTOUCHDEF_ID, 0,
                SLURCANSPANRESTSDEF_ID));
    contmark* dashedphrase_cont;
    insmark(dashedphrase_cont =
                new contmark(".dash((.",
                             "Continue higher-level dashed slur or phrase "
                             "articulation." // enum{dashedphrase_cont}
                             "  FOMUS begins/ends slur marks where these "
                             "continue marks start/stop."
                             "  The `((' may be followed by one or more of the "
                             "following flag characters: `r' (the mark can "
                             "span rests), "
                             "`n' (the mark cannot span rests), `-' (the "
                             "beginning and ends of the mark can touch) "
                             "and `|' (the beginning and ends of the mark "
                             "cannot touch).  Examples are `.dash((r.', "
                             "`.dash((-.' and `.dash((r-.'.",
                             true, module_none, move_left, markpos_notehead,
                             "[.dash((.]"));

    spanmark* graceslur_begin;
    insmark(graceslur_begin = new spanmark(
                "grace(..",
                "Begin grace note slur articulation.  (FOMUS automatically "
                "adds these.)" // enum{graceslur_begin}
                "  The `(' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `-' (the beginning and ends "
                "of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `grace(r..', `grace(-..' and `grace(r-..'.",
                module_none, m_voicebegin, spr_cannotspanone, move_left,
                markpos_notehead, "[grace(..]", 5, SLURCANTOUCHDEF_ID, 0,
                SLURCANSPANRESTSDEF_ID));
    spanmark* graceslur_end;
    insmark(graceslur_end = new spanmark(
                "..grace)",
                "End grace note slur articulation.  (FOMUS automatically adds "
                "these.)" // enum{graceslur_end}
                "  The `)' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `-' (the beginning and ends "
                "of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `..grace)r', `..grace)-' and `..grace)r-'.",
                module_none, m_voiceend, spr_cannotspanone, move_right,
                markpos_notehead, "[..grace)]", 5, SLURCANTOUCHDEF_ID, 0,
                SLURCANSPANRESTSDEF_ID));
    contmark* graceslur_cont;
    insmark(graceslur_cont = new contmark(
                ".grace(.",
                "Continue grace note slur articulation." // enum{graceslur_cont}
                "  FOMUS begins/ends slur marks where these continue marks "
                "start/stop."
                "  The `(' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `-' (the beginning and ends "
                "of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `.grace(r.', `.grace(-.' and `.grace(r-.'.",
                true, module_none, move_left, markpos_notehead, "[.grace(.]"));

    spanmark* cresc_begin;
    insmark(cresc_begin = new spanmark(
                "<..",
                "Begin crescendo wedge." // enum{cresc_begin}
                "  The `<' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `1' (the mark can span one "
                "note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `<1..', `<-..' and `<1-..'.",
                module_none, m_voicebegin, sin_isdetach, move_left,
                markpos_prefmiddleorbelow, "[<..]", 3, WEDGECANTOUCHDEF_ID,
                WEDGECANSPANONEDEF_ID, WEDGECANSPANRESTSDEF_ID));
    spanmark* cresc_end;
    insmark(cresc_end = new spanmark(
                "..<",
                "End crescendo wedge." // enum{cresc_end}
                "  The `<' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `1' (the mark can span one "
                "note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `..<1', `..<-' and `..<1-'.",
                module_none, m_voiceend, sin_isdetach, move_right,
                markpos_prefmiddleorbelow, "[..<]", 3, WEDGECANTOUCHDEF_ID,
                WEDGECANSPANONEDEF_ID, WEDGECANSPANRESTSDEF_ID));
    contmark* cresc_cont;
    insmark(cresc_cont = new contmark(
                ".<.",
                "Continue crescendo wedge." // enum{cresc_cont}
                "  FOMUS begins/ends wedge marks where these continue marks "
                "start/stop."
                "  The `<' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `1' (the mark can span one "
                "note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `.<1.', `.<-.' and `.<1-.'.",
                true, module_none, move_left, markpos_prefmiddleorbelow,
                "[.<.]", sin_isdetach));

    spanmark* dim_begin;
    insmark(dim_begin = new spanmark(
                ">..",
                "Begin diminuendo wedge." // enum{dim_begin}
                "  The `>' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `1' (the mark can span one "
                "note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `>1..', `>-..' and `>1-..'.",
                module_none, m_voicebegin, sin_isdetach, move_left,
                markpos_prefmiddleorbelow, "[>..]", 3, WEDGECANTOUCHDEF_ID,
                WEDGECANSPANONEDEF_ID, WEDGECANSPANRESTSDEF_ID));
    spanmark* dim_end;
    insmark(dim_end = new spanmark(
                "..>",
                "End diminuendo wedge." // enum{dim_end}
                "  The `>' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `1' (the mark can span one "
                "note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `..>1', `..>-' and `..>1-'.",
                module_none, m_voiceend, sin_isdetach, move_right,
                markpos_prefmiddleorbelow, "[..>]", 3, WEDGECANTOUCHDEF_ID,
                WEDGECANSPANONEDEF_ID, WEDGECANSPANRESTSDEF_ID));
    contmark* dim_cont;
    insmark(dim_cont = new contmark(
                ".>.",
                "Continue diminuendo wedge." // enum{dim_cont}
                "  FOMUS begins/ends wedge marks where these continue marks "
                "start/stop."
                "  The `>' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `1' (the mark can span one "
                "note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `.>1.', `.>-.' and `.>1-.'.",
                true, module_none, move_left, markpos_prefmiddleorbelow,
                "[.>.]", sin_isdetach));

    spanmark* ped_begin;
    insmark(ped_begin = new spanmark(
                "ped..",
                "Begin piano pedal mark." // enum{ped_begin}
                "  `ped' may be followed by one or more of the following flag "
                "characters: `1' (the mark can span one note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `ped1..', `ped-..' and `ped1-..'."
                "  To insure that this mark appears below the bottom staff, it "
                "should be defined in a mark event and not attached to a note.",
                module_none, m_voicebegin,
                spr_canspanrests | spr_canendonrests | spr_cantouch |
                    sin_mustdetach,
                move_left, markpos_below, "[ped..]", 7, PEDCANTOUCHDEF_ID,
                PEDCANSPANONEDEF_ID, 0));
    spanmark* ped_end;
    insmark(ped_end = new spanmark(
                "..ped",
                "End piano pedal mark." // enum{ped_end}
                "  `ped' may be followed by one or more of the following flag "
                "characters: `1' (the mark can span one note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `..ped1', `..ped-' and `..ped1-'."
                "  To insure that this mark appears below the bottom staff, it "
                "should be defined in a mark event and not attached to a note.",
                module_none, m_voiceend,
                spr_canspanrests | spr_canendonrests | spr_cantouch |
                    sin_mustdetach,
                move_right, markpos_below, "[..ped]", 7, PEDCANTOUCHDEF_ID,
                PEDCANSPANONEDEF_ID, 0));
    contmark* ped_cont;
    insmark(ped_cont = new contmark(
                ".ped.",
                "Continue piano pedal mark." // enum{ped_cont}
                "  `ped' may be followed by one or more of the following flag "
                "characters: `1' (the mark can span one note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `.ped1.', `.ped-.' and `.ped1-.'."
                "  To insure that this mark appears below the bottom staff, it "
                "should be defined in a mark event and not attached to a note.",
                true, module_none, move_left, markpos_below, "[.ped.]",
                sin_mustdetach));

    insmark(new textmark("x",
                         "Text marking attached to a notehead.  "
                         "The text for this usually appears above the note in "
                         "a normal (non-italic) typeface.",
                         module_string, move_left, markpos_prefabove,
                         "[x string_text]")); // enum{text}

    insmark(new textmark("x_",
                         "Text marking in italics.  The text usually appears "
                         "below the staff or in the middle of a grand staff.",
                         module_string, move_left, markpos_prefmiddleorbelow,
                         "[x_ string_text]",
                         sin_isdetach)); // enum{italictextabove}
    spanmark* itdntext_begin;
    insmark(itdntext_begin = new textspanner(
                "x_..",
                "Begin text spanner in italics."
                "  The text for this usually appears below the staff or in the "
                "middle of a grand staff."
                "  The `x_' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `1' (the mark can span one "
                "note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `x_1..', `x_-..' and `x_1-..'.",
                module_string, m_voicebegin, spr_isreduc | sin_isdetach,
                move_left, markpos_prefmiddleorbelow, "[x_.. string_text]", 10,
                TEXTCANTOUCHDEF_ID, TEXTCANSPANONEDEF_ID,
                TEXTCANSPANRESTSDEF_ID)); // enum{italictextabove_begin}
    spanmark* itdntext_end;
    insmark(itdntext_end = new textspanner(
                "..x_",
                "End text spanner in italics."
                "  The text for this usually appears below the staff or in the "
                "middle of a grand staff."
                "  The `x_' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `1' (the mark can span one "
                "note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `..x_1', `..x_-' and `..x_1-'.",
                module_string, m_voiceend, spr_isreduc | sin_isdetach,
                move_right, markpos_prefmiddleorbelow, "[..x_ string_text]", 10,
                TEXTCANTOUCHDEF_ID, TEXTCANSPANONEDEF_ID,
                TEXTCANSPANRESTSDEF_ID)); // enum{italictextabove_end}
    contmark* itdntext_cont;
    insmark(
        itdntext_cont = new textcontmark(
            ".x_.",
            "Continue text spanner in italics." // enum{italictextabove_cont}
            "  FOMUS begins/ends text marks where these continue marks "
            "start/stop."
            "  The text for this usually appears below the staff or in the "
            "middle of a grand staff."
            "  The `x_' may be followed by one or more of the following flag "
            "characters: `r' (the mark can span rests), "
            "`n' (the mark cannot span rests), `1' (the mark can span one "
            "note), "
            "`m' (the mark cannot span one note), `-' (the beginning and ends "
            "of the mark can touch) "
            "and `|' (the beginning and ends of the mark cannot touch).  "
            "Examples are `.x_1.', `.x_-.' and `.x_1-.'.",
            true, module_none, move_left, markpos_prefmiddleorbelow,
            "[.x_. string_text]", sin_isdetach));

    insmark(new textmark("x^",
                         "Text marking in italics.  The text usually appears "
                         "above the staff or in the middle of a grand staff.",
                         module_string, move_left, markpos_prefmiddleorabove,
                         "[x^ string_text]",
                         sin_isdetach)); // enum{italictextbelow}
    spanmark* ituptext_begin;
    insmark(ituptext_begin = new textspanner(
                "x^..",
                "Begin text spanner in italics."
                "  The text for this usually appears above the staff or in the "
                "middle of a grand staff."
                "  The `x^' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `1' (the mark can span one "
                "note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `x^1..', `x^-..' and `x^1-..'.",
                module_string, m_voicebegin, spr_isreduc | sin_isdetach,
                move_left, markpos_prefmiddleorabove, "[x^.. string_text]", 11,
                TEXTCANTOUCHDEF_ID, TEXTCANSPANONEDEF_ID,
                TEXTCANSPANRESTSDEF_ID)); // enum{italictextbelow_begin}
    spanmark* ituptext_end;
    insmark(ituptext_end = new textspanner(
                "..x^",
                "End text spanner in italics."
                "  The text for this usually appears above the staff or in the "
                "middle of a grand staff."
                "  The `x^' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `1' (the mark can span one "
                "note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `..x^1', `..x^-' and `..x^1-'.",
                module_string, m_voiceend, spr_isreduc | sin_isdetach,
                move_right, markpos_prefmiddleorabove, "[..x^ string_text]", 11,
                TEXTCANTOUCHDEF_ID, TEXTCANSPANONEDEF_ID,
                TEXTCANSPANRESTSDEF_ID)); // enum{italictextbelow_end}
    contmark* ituptext_cont;
    insmark(
        ituptext_cont = new textcontmark(
            ".x^.",
            "Continue text spanner in italics." // enum{italictextbelow_cont}
            "  FOMUS begins/ends text marks where these continue marks "
            "start/stop."
            "  The text for this usually appears above the staff or in the "
            "middle of a grand staff."
            "  The `x^' may be followed by one or more of the following flag "
            "characters: `r' (the mark can span rests), "
            "`n' (the mark cannot span rests), `1' (the mark can span one "
            "note), "
            "`m' (the mark cannot span one note), `-' (the beginning and ends "
            "of the mark can touch) "
            "and `|' (the beginning and ends of the mark cannot touch).  "
            "Examples are `.x^1.', `.x^-.' and `.x^1-.'.",
            true, module_string, move_left, markpos_prefmiddleorabove,
            "[.x^. string_text]", sin_isdetach));

    insmark(new textmark(
        "x!",
        "Text marking in bold face.  The text usually appears above the staff.",
        module_string, move_left, markpos_prefabove, "[x! string_text]",
        sin_isdetach)); // enum{stafftext}
    spanmark* stafftext_begin;
    insmark(stafftext_begin = new textspanner(
                "x!..",
                "Begin text spanner in bold face.  The text for this usually "
                "appears above the staff."
                "  The `x!' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `1' (the mark can span one "
                "note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `x!1..', `x!-..' and `x!1-..'.",
                module_string, m_voicebegin, spr_isreduc | sin_isdetach,
                move_left, markpos_prefabove, "[x!.. string_text]", 12,
                TEXTCANTOUCHDEF_ID, TEXTCANSPANONEDEF_ID,
                TEXTCANSPANRESTSDEF_ID)); // enum{stafftext_begin}
    spanmark* stafftext_end;
    insmark(stafftext_end = new textspanner(
                "..x!",
                "End text spanner in bold face.  The text for this usually "
                "appears above the staff."
                "  The `x!' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `1' (the mark can span one "
                "note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `..x!1', `..x!-' and `..x!1-'.",
                module_string, m_voiceend, spr_isreduc | sin_isdetach,
                move_right, markpos_prefabove, "[..x! string_text]", 12,
                TEXTCANTOUCHDEF_ID, TEXTCANSPANONEDEF_ID,
                TEXTCANSPANRESTSDEF_ID)); // enum{stafftext_end}
    contmark* stafftext_cont;
    insmark(stafftext_cont = new textcontmark(
                ".x!.",
                "Continue text spanner in bold face.  The text for this "
                "usually appears above the staff." // enum{stafftext_cont}
                "  FOMUS begins/ends text marks where these continue marks "
                "start/stop."
                "  The `x!' may be followed by one or more of the following "
                "flag characters: `r' (the mark can span rests), "
                "`n' (the mark cannot span rests), `1' (the mark can span one "
                "note), "
                "`m' (the mark cannot span one note), `-' (the beginning and "
                "ends of the mark can touch) "
                "and `|' (the beginning and ends of the mark cannot touch).  "
                "Examples are `.x!1.', `.x!-.' and `.x!1-.'.",
                true, module_string, move_left, markpos_prefabove,
                "[.x!. string_text]", sin_isdetach));

    // vocal text
    insmark(new textmark("*", "Vocal/lyric text syllable.", module_string,
                         move_left, markpos_prefbelow, "[* string_text]",
                         sin_isdetach)); // enum{vocal_text}

    // other stuff
    insmark(new markbase("~", "Forces a tie to the next note.", module_none,
                         move_right, markpos_notehead, "[~]")); // enum{tie}
    insmark(new tupletmark(
        "tup..",
        "Begin a tuplet on this note.  If the optional number argument is "
        "given, begin a tuplet at that nested level.",
        module_number, move_left, markpos_notehead,
        "[tup.. integer>=1]")); // enum{tuplet_begin}
    insmark(
        new tupletmark("..tup",
                       "End a tuplet on this note.  If the optional number "
                       "argument is given, end a tuplet at that nested level.",
                       module_number, move_right, markpos_notehead,
                       "[..tup integer>=1]")); // enum{tuplet_end}

    insmark(new markbase("/", "Attach a slash to a group of grace notes.",
                         module_none, move_left, markpos_notehead,
                         "[/]")); // enum{graceslash}

    assert(markdefs.size() == mark_nmarks);
    insspanmarks("(", *slur_begin, "..)", *slur_end, *slur_cont, false);
    insspanmarks("dot(", *dottedslur_begin, "..dot)", *dottedslur_end,
                 *dottedslur_cont, false);
    insspanmarks("dash(", *dashedslur_begin, "..dash)", *dashedslur_end,
                 *dashedslur_cont, false);
    insspanmarks("((", *phrase_begin, "..))", *phrase_end, *phrase_cont, false);
    insspanmarks("dot((", *dottedphrase_begin, "..dot))", *dottedphrase_end,
                 *dottedphrase_cont, false);
    insspanmarks("dash((", *dashedphrase_begin, "..dash))", *dashedphrase_end,
                 *dashedphrase_cont, false);
    insspanmarks("grace(", *graceslur_begin, "..grace)", *graceslur_end,
                 *graceslur_cont, false);
    insspanmarks("<", *cresc_begin, "..<", *cresc_end, *cresc_cont);
    insspanmarks(">", *dim_begin, "..>", *dim_end, *dim_cont);
    insspanmarks("x_", *itdntext_begin, "..x_", *itdntext_end, *itdntext_cont);
    insspanmarks("x^", *ituptext_begin, "..x^", *ituptext_end, *ituptext_cont);
    insspanmarks("x!", *stafftext_begin, "..x!", *stafftext_end,
                 *stafftext_cont);
    insspanmarks("ped", *ped_begin, "..ped", *ped_end, *ped_cont, true, false);

    spec.insert(std::set<std::pair<enum module_markids, enum module_markids>>::
                    value_type(mark_longtrill, mark_longtrill2));
    spec.insert(std::set<std::pair<enum module_markids, enum module_markids>>::
                    value_type(mark_trem, mark_trem2));
    spec.insert(std::set<std::pair<enum module_markids, enum module_markids>>::
                    value_type(mark_natharm_sounding, mark_natharm_touched));
    spec.insert(std::set<std::pair<enum module_markids, enum module_markids>>::
                    value_type(mark_natharm_sounding, mark_natharm_string));
    spec.insert(std::set<std::pair<enum module_markids, enum module_markids>>::
                    value_type(mark_natharm_touched, mark_natharm_sounding));
    spec.insert(std::set<std::pair<enum module_markids, enum module_markids>>::
                    value_type(mark_natharm_touched, mark_natharm_string));
    spec.insert(std::set<std::pair<enum module_markids, enum module_markids>>::
                    value_type(mark_natharm_string, mark_natharm_sounding));
    spec.insert(std::set<std::pair<enum module_markids, enum module_markids>>::
                    value_type(mark_natharm_string, mark_natharm_touched));
    spec.insert(std::set<std::pair<enum module_markids, enum module_markids>>::
                    value_type(mark_artharm_sounding, mark_artharm_touched));
    spec.insert(std::set<std::pair<enum module_markids, enum module_markids>>::
                    value_type(mark_artharm_sounding, mark_artharm_base));
    spec.insert(std::set<std::pair<enum module_markids, enum module_markids>>::
                    value_type(mark_artharm_touched, mark_artharm_sounding));
    spec.insert(std::set<std::pair<enum module_markids, enum module_markids>>::
                    value_type(mark_artharm_touched, mark_artharm_base));
    spec.insert(std::set<std::pair<enum module_markids, enum module_markids>>::
                    value_type(mark_artharm_base, mark_artharm_sounding));
    spec.insert(std::set<std::pair<enum module_markids, enum module_markids>>::
                    value_type(mark_artharm_base, mark_artharm_touched));
  }

  void markbase::checkargs(const fomusdata* fom, const std::string& str,
                           const numb& val, const filepos& pos) const {
    assert(vartype == module_none);
    if (!str.empty() || val.isntnull()) {
      CERR << "expected `" << typedoc << '\'';
      pos.printerr();
      throw errbase();
    }
  }

  void longtrmark::checkargs(const fomusdata* fom, const std::string& str,
                             const numb& val, const filepos& pos) const {
    if (val.isntnull() && (val.type() != module_string ||
                           doparseacc(fom, str.c_str(), true, 0).isnull())) {
      CERR << "expected `" << typedoc << '\'';
      pos.printerr();
      throw errbase();
    }
  }

  void tremmark::checkargs(const fomusdata* fom, const std::string& str,
                           const numb& val, const filepos& pos) const {
    if (val.isntnull() && (!val.israt() || val <= (fint) 0)) {
      CERR << "expected `" << typedoc << '\'';
      pos.printerr();
      throw errbase();
    }
  }

  void sulmark::checkargs(const fomusdata* fom, const std::string& str,
                          const numb& val, const filepos& pos) const {
    if (!val.isint() || val < (fint) 0 || val > (fint) 128) {
      CERR << "expected `" << typedoc << '\'';
      pos.printerr();
      throw errbase();
    }
  }

  void tupletmark::checkargs(const fomusdata* fom, const std::string& str,
                             const numb& val, const filepos& pos) const {
    if (val.isntnull() && (!val.isint() || val < (fint) 1)) {
      CERR << "expected `" << typedoc << '\'';
      pos.printerr();
      throw errbase();
    }
  }

  void textmark::checkargs(const fomusdata* fom, const std::string& str,
                           const numb& val, const filepos& pos) const {
    if (str.empty() || val.isntnull()) {
      CERR << "expected `" << typedoc << '\'';
      pos.printerr();
      throw errbase();
    }
  }

  void textspanner::checkargs(const fomusdata* fom, const std::string& str,
                              const numb& val, const filepos& pos) const {
    if (/*str.empty() ||*/ val.isntnull()) {
      CERR << "expected `" << typedoc << '\'';
      pos.printerr();
      throw errbase();
    }
  }

  void tempomark::checkargs(const fomusdata* fom, const std::string& str,
                            const numb& val, const filepos& pos) const {
    if (str.empty() && val.isnull()) {
      CERR << "expected `" << typedoc << '\'';
      pos.printerr();
      throw errbase();
    }
  }

  void textcontmark::checkargs(const fomusdata* fom, const std::string& str,
                               const numb& val, const filepos& pos) const {
    if (/*str.empty() ||*/ val.isntnull()) {
      CERR << "expected `" << typedoc << '\'';
      pos.printerr();
      throw errbase();
    }
  }

} // namespace fomus

using namespace fomus;

int module_strtomark(const char* str) {
  ENTER_API;
  std::map<std::string, fomus::markbase*, fomus::isiless>::const_iterator i(
      fomus::marksbyname.find(str));
  if (i == fomus::marksbyname.end())
    return -1;
  return i->second->getid();
  EXIT_API_0;
}
const char* module_marktostr(int id) {
  ENTER_API;
  if (id < 0 || id >= mark_nmarks) {
    CERR << "invalid mark id ";
    fomus::modprinterr();
    return 0;
  }
  return fomus::markdefs[id].getname();
  EXIT_API_0;
}
