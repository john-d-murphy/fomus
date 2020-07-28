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

#include "data.h"
#include "instrs.h"
#include "moremath.h" // floor_base

namespace fomus {

  int fomusdata::getvarid(const std::string& str) const {
    varsptrmap_constit v(strinvars.find(str));
    if (v == strinvars.end()) {
      CERR << "unknown setting `" << str << '\'';
      pos.printerr();
      throw errbase();
    }
    return v->second;
  }

  void fomusdata::param_settingid(const int id) {
    if (id < 0 || id >= (int) vars.size()) {
      CERR << "invalid setting id";
      pos.printerr();
      throw errbase();
    }
    curvar = id;
  }

  void fomusdata::setlset() {
    try {
      checkiscurvar();
      DBG("instack size = " << instack.size() << std::endl);
      std::auto_ptr<varbase> v(invars[curvar]->getnew(inlist, pos));
      DBG("calling THROWIFINVALID" << std::endl);
      v->throwifinvalid(this);
      ++pos;
      varbase* v0;
      invars[curvar].reset(v0 = v.release());
      clearlist();
      v0->activate(symtabs(note_inparse, acc_inparse, mic_inparse, oct_inparse,
                           note_inprint, acc_inprint, mic_inprint, oct_inprint,
                           this, durdot_inparse, dursyms_inparse,
                           durtie_inparse, tupsyms_inparse));
    } catch (const errbase& e) {
      clearlist();
      throw;
    }
  }
  void fomusdata::setlset_note() {
    try {
      checkiscurvar();
      DBG("instack size = " << instack.size() << std::endl);
      std::auto_ptr<varbase> v(invars[curvar]->getnew(inlist, pos));
      v->throwifinvalid(this);
      data->addset(v.release());
      clearlist();
    } catch (const errbase& e) {
      clearlist();
      throw;
    }
  }
  void fomusdata::appendlset_el(const listel& val) {
    checkiscurvar();
    std::auto_ptr<varbase> v(invars[curvar]->getnewprepend(
        listelvect(1, val), pos)); // getnewprepend destoys the list!
    v->throwifinvalid(this);
    ++pos;
    varbase* v0;
    invars[curvar].reset(v0 = v.release());
    v0->activate(symtabs(note_inparse, acc_inparse, mic_inparse, oct_inparse,
                         note_inprint, acc_inprint, mic_inprint, oct_inprint,
                         this, durdot_inparse, dursyms_inparse, durtie_inparse,
                         tupsyms_inparse));
  }
  void fomusdata::appendlset() {
    try {
      checkiscurvar();
      std::auto_ptr<varbase> v(invars[curvar]->getnewprepend(
          inlist, pos)); // getnewprepend destoys the list!
      v->throwifinvalid(this);
      ++pos;
      varbase* v0;
      invars[curvar].reset(v0 = v.release());
      clearlist();
      v0->activate(symtabs(note_inparse, acc_inparse, mic_inparse, oct_inparse,
                           note_inprint, acc_inprint, mic_inprint, oct_inprint,
                           this, durdot_inparse, dursyms_inparse,
                           durtie_inparse, tupsyms_inparse));
    } catch (const errbase& e) {
      clearlist();
      throw;
    }
  }
  void fomusdata::appendlsetel_note(const listel& val) {
    checkiscurvar();
    std::auto_ptr<varbase> v(invars[curvar]->getnewprepend(
        listelvect(1, val), pos)); // getnewprepend destoys the list!
    v->throwifinvalid(this);
    data->addset(v.release());
  }
  void fomusdata::appendlset_note() {
    try {
      checkiscurvar();
      std::auto_ptr<varbase> v(invars[curvar]->getnewprepend(
          inlist, pos)); // getnewprepend destoys the list!
      v->throwifinvalid(this);
      data->addset(v.release());
      clearlist();
    } catch (const errbase& e) {
      clearlist();
      throw;
    }
  }

  void fomusdata::listsetinst() {
    try {
      checkiscurvar();
      std::auto_ptr<varbase> v(invars[INSTRS_ID]->getnew(
          inlistinsts, pos)); // don't need to do valid check
      ++pos;
      varbase* v0;
      invars[INSTRS_ID].reset(v0 = v.release());
      inlistinsts.clear();
      v0->activate(symtabs(note_inparse, acc_inparse, mic_inparse, oct_inparse,
                           note_inprint, acc_inprint, mic_inprint, oct_inprint,
                           this, durdot_inparse, dursyms_inparse,
                           durtie_inparse, tupsyms_inparse));
    } catch (const errbase& e) {
      inlistinsts.clear();
      throw;
    }
  }
  void fomusdata::listappendinst() {
    try {
      checkiscurvar();
      std::auto_ptr<varbase> v(invars[INSTRS_ID]->getnewprepend(
          inlistinsts, pos)); // don't need to do valid check
      ++pos;
      varbase* v0;
      invars[INSTRS_ID].reset(v0 = v.release());
      inlistinsts.clear();
      v0->activate(symtabs(note_inparse, acc_inparse, mic_inparse, oct_inparse,
                           note_inprint, acc_inprint, mic_inprint, oct_inprint,
                           this, durdot_inparse, dursyms_inparse,
                           durtie_inparse, tupsyms_inparse));
    } catch (const errbase& e) {
      inlistinsts.clear();
      throw;
    }
  }
  void fomusdata::listsetpercinst() {
    try {
      checkiscurvar();
      std::auto_ptr<varbase> v(invars[PERCINSTRS_ID]->getnew(
          inlistpercs, pos)); // don't need to do valid check
      ++pos;
      varbase* v0;
      invars[PERCINSTRS_ID].reset(v0 = v.release());
      inlistpercs.clear();
      v0->activate(symtabs(note_inparse, acc_inparse, mic_inparse, oct_inparse,
                           note_inprint, acc_inprint, mic_inprint, oct_inprint,
                           this, durdot_inparse, dursyms_inparse,
                           durtie_inparse, tupsyms_inparse));
    } catch (const errbase& e) {
      inlistpercs.clear();
      throw;
    }
  }
  void fomusdata::listappendpercinst() {
    try {
      checkiscurvar();
      std::auto_ptr<varbase> v(invars[PERCINSTRS_ID]->getnewprepend(
          inlistpercs, pos)); // don't need to do valid check
      ++pos;
      varbase* v0;
      invars[PERCINSTRS_ID].reset(v0 = v.release());
      inlistpercs.clear();
      v0->activate(symtabs(note_inparse, acc_inparse, mic_inparse, oct_inparse,
                           note_inprint, acc_inprint, mic_inprint, oct_inprint,
                           this, durdot_inparse, dursyms_inparse,
                           durtie_inparse, tupsyms_inparse));
    } catch (const errbase& e) {
      inlistpercs.clear();
      throw;
    }
  }
  void fomusdata::listaddinst() {
    try {
      instr->complete(*this);
      if (instr->getid().empty()) {
        thrownoid("instrument", pos);
      }
      globinstsvarvect_it x(inlistinsts.find(instr->getid()));
      if (x != inlistinsts.end()) {
        throwdupid("instrument", pos);
      }
      inlistinsts.insert(globinstsvarvect_val(instr->getid(), instr));
      instr.reset(new instr_str());
    } catch (const errbase& e) {
      instr.reset(new instr_str());
      throw;
    }
  }
  void fomusdata::listaddpercinst() {
    try {
      perc->complete(*this);
      if (perc->getid().empty()) {
        thrownoid("percussion instrument", pos);
      }
      globpercsvarvect_it x(inlistpercs.find(perc->getid()));
      if (x != inlistpercs.end()) {
        throwdupid("instrument", pos);
      }
      inlistpercs.insert(globpercsvarvect_val(perc->getid(), perc));
      perc.reset(new percinstr_str());
    } catch (const errbase& e) {
      perc.reset(new percinstr_str());
      throw;
    }
  }
  void fomusdata::set_part_instr() {
    instr->complete(*this);
    const std::string& id = instr->getid();
    if (!id.empty()) {
      definstsmap_it x(default_insts.find(id));
      if (x != default_insts.end()) {
        x->second->replace(*instr);
        instr = x->second;
      } else
        default_insts.insert(definstsmap_val(id, instr));
    }
    apart->setinstrstr(instr);
  }
  void fomusdata::ins_instr_percinstr() {
    perc->complete(*this);
    const std::string& id = perc->getid();
    if (!id.empty()) {
      defpercsmap_it x(default_percs.find(id));
      if (x != default_percs.end()) {
        x->second->replace(*perc);
        perc = x->second;
      } else
        default_percs.insert(defpercsmap_val(id, perc));
    }
    instr->inspercinstrstr(perc);
  }
  void fomusdata::set_partmap_part() {
    apart->complete(*this);
    const std::string& id = apart->getid();
    if (!id.empty()) {
      defpartsmap_it x(default_parts.find(id));
      if (x != default_parts.end()) {
        x->second->replace(*apart);
        apart = x->second;
      } else
        default_parts.insert(defpartsmap_val(id, apart));
    }
    partmap->setpartstr(apart);
  }
  void fomusdata::set_partmap_metapart() {
    mpart->complete(*this);
    const std::string& id = mpart->getid();
    if (!id.empty()) {
      defmpartsmap_it x(default_mparts.find(id));
      if (x != default_mparts.end()) {
        x->second->replace(*mpart);
        mpart = x->second;
      } else
        default_mparts.insert(defmpartsmap_val(id, mpart));
    }
    partmap->setmpartstr(mpart);
  }

  void fomusdata::startlist() {
    if (instack.empty()) {
      instack.push(&inlist);
    } else {
      listelvect* v;
      instack.top()->push_back(listelshptr(v = new listelvect));
      instack.push(v); // put an invect on the stack (current list)
    }
  }

  void fomusdata::endlist(const bool istop) {
    if (istop ? instack.size() == 1 : instack.size() > 1)
      instack.pop();
  }

  void fomusdata::insertf(const ffloat val) {
    if (instack.empty()) {
      throwinvalid(pos);
    } else
      instack.top()->push_back(numb(val));
  }
  void fomusdata::inserti(const fint val) {
    if (instack.empty()) {
      throwinvalid(pos);
    } else
      instack.top()->push_back(numb(val));
  }
  void fomusdata::insertr(const fint num, const fint den) {
    rat val(num, den);
    if (instack.empty()) {
      throwinvalid(pos);
    } else
      instack.top()->push_back(numb(val));
  }
  void fomusdata::insertm(const fint val, const fint num, const fint den) {
    rat vl2(val + rat(num, den));
    if (instack.empty()) {
      throwinvalid(pos);
    } else
      instack.top()->push_back(numb(vl2));
  }
  void fomusdata::inserts(const char* val) {
    if (instack.empty()) {
      throwinvalid(pos);
    } else
      instack.top()->push_back(val);
  }

  // for settings
  // forceacc is -1, 1 to force flat, sharp
  std::string notenumtostring(const fomusdata* fd, const numb& val,
                              const char* str, const bool withoct,
                              const int forceacc) {
    std::ostringstream out;
    if ((fd ? fd->get_ival(NOTEPRINT_ID) : getdefaultival(NOTEPRINT_ID)) &&
        (val.type() == module_int || val.type() == module_rat)) {
      static int accs[12] = {0, -1, 0, -1, 0, 0, 1, 0, -1, 0, -1, 0};
      rat n(numtorat(val));
      fint ob = floor_base12(n); // oct base pitch
      assert(ob <= n && ob + 12 > n);
      rat rem(n - ob); // rem = [0, 12)
      assert(rem >= 0 && rem < 12);
      fint pc = round_downbase1(rem); // pc (could be 12 at this point)
      assert(pc <= rem && pc + 1 > rem);
      assert(pc >= 0 && pc < 12);
      int acc0;
      switch (forceacc) {
      case -1:
        acc0 = -1;
        break;
      case 0:
        acc0 = accs[pc];
        break;
      case 1:
        acc0 = 1;
        break;
      default:
        throw badforceacc();
      }
      pc -= acc0; // pc now a white key
      if (pc >= 12) {
        ob += 12;
        pc -= 12;
      } else if (pc < 0) {
        ob -= 12;
        pc += 12;
      }
      if (blacknotes[pc]) {
        assert(forceacc != 0);
        throw badforceacc();
      }
      rat mic(rem - pc);
      assert(mic > -2 && mic < 2);
      fint acc = (mic >= 1) ? (--mic, 1) : ((mic <= -1) ? (++mic, -1) : 0);
      const printmap& npr = fd ? fd->getnotepr() : note_print;
      printmap_constit nx(npr.find(pc));
      if (nx == npr.end())
        goto NODEAL;
      printmap_constit ax, mx;
      if (acc != 0 || mic != 0) {
        const printmap& apr = fd ? fd->getaccpr() : acc_print;
        ax = (apr.find(acc));
        if (ax == apr.end())
          goto NODEAL;
        if (mic != 0) {
          const printmap& mpr = fd ? fd->getmicpr() : mic_print;
          mx = (mpr.find(mic));
          if (mx == mpr.end())
            goto NODEAL;
        }
      }
      printmap_constit ox;
      if (withoct) {
        const printmap& opr = fd ? fd->getoctpr() : oct_print;
        ox = opr.find(ob);
        if (ox == opr.end())
          goto NODEAL;
      }
      out << nx->second;
      if (acc != 0 || mic != 0) {
        out << ax->second;
        if (mic != 0)
          out << mx->second;
      }
      if (withoct)
        out << ox->second;
      return stringify(out.str(), str);
    } else {
    NODEAL:
      out.setf(std::ios_base::fixed, std::ios_base::floatfield);
      out << std::setprecision(3) << val;
    }
    return out.str();
  }

  int fomusdata::getclef(const std::string& str) {
    try {
      return strtoclef(str);
    } catch (const badstr& e) {
      CERR << "invalid clef `" << str << '\'';
      pos.printerr();
      throw;
    }
  }

  void fomusdata::ins_mpart_partmap(const char* id) {
    if (!mpartstack.empty()) {
      mpart = mpartstack.back();
      mpartstack.pop_back();
    }
    if (id) {
      if (default_parts.find(id) != default_parts.end())
        set_partmap_part(id);
      else if (default_mparts.find(id) != default_mparts.end())
        set_partmap_metapart(id);
    }
    partmap->complete(*this);
    mpart->inspartmap(partmap);
  } //

  void fomusdata::ins_part() {
    try {
      apart->complete(*this);
      if (apart->getid().empty()) {
        thrownoid("part", pos);
      }
      defmpartsmap_it x0(default_mparts.find(apart->getid()));
      if (x0 != default_mparts.end()) {
        default_mparts.erase(x0);
        remfrompartlist(apart->getid());
      }
      defpartsmap_it x(default_parts.find(apart->getid()));
      if (x != default_parts.end()) {
        x->second->replace(*apart);
        apart = x->second;
        remfrompartlist(apart->getid());
      } else
        default_parts.insert(defpartsmap_val(apart->getid(), apart));
      scoreparts.push_back(apart);
      apart->setself(boost::prior(scoreparts.end()), nextpartind());
      curseldpart = apart;
      lastent = apart;
      apart.reset(new part_str());
    } catch (const errbase& e) {
      apart.reset(new part_str());
      throw;
    }
  }
  void fomusdata::ins_metapart() {
    try {
      mpart->complete(*this);
      if (mpart->getid().empty()) {
        thrownoid("metapart", pos);
      }
      defpartsmap_it x0(default_parts.find(mpart->getid()));
      if (x0 != default_parts.end()) {
        default_parts.erase(x0);
        remfrompartlist(mpart->getid());
      }
      defmpartsmap_it x(default_mparts.find(mpart->getid()));
      if (x != default_mparts.end()) {
        x->second->replace(*mpart);
        mpart = x->second;
        remfrompartlist(mpart->getid());
      } else
        default_mparts.insert(defmpartsmap_val(mpart->getid(), mpart));
      scoreparts.push_back(mpart);
      mpart->setself(boost::prior(scoreparts.end()), nextpartind());
      curseldpart = mpart;
      lastent = mpart;
      mpart.reset(new mpart_str());
    } catch (const errbase& e) {
      mpart.reset(new mpart_str());
      throw;
    }
  }
  void fomusdata::ins_percinst() {
    try {
      perc->complete(*this);
      if (perc->getid().empty()) {
        thrownoid("percussion instrument", pos);
      }
      defpercsmap_it x(default_percs.find(perc->getid()));
      if (x != default_percs.end()) {
        x->second->replace(*perc);
        perc = x->second;
      } else
        default_percs.insert(defpercsmap_val(perc->getid(), perc));
      lastent = perc;
      perc.reset(new percinstr_str());
    } catch (const errbase& e) {
      perc.reset(new percinstr_str());
      throw;
    }
  }
  void fomusdata::ins_inst() {
    try {
      instr->complete(*this);
      if (instr->getid().empty()) {
        thrownoid("instrument", pos);
      }
      definstsmap_it x(default_insts.find(instr->getid()));
      if (x != default_insts.end()) {
        x->second->replace(*instr);
        instr = x->second;
      } else
        default_insts.insert(definstsmap_val(instr->getid(), instr));
      lastent = instr;
      instr.reset(new instr_str());
    } catch (const errbase& e) {
      instr.reset(new instr_str());
      throw;
    }
  }
  void fomusdata::ins_measdef() { // checks for an id!
    try {
      measdef->complete(*this);
      if (measdef->getid().empty()) {
        thrownoid("measure definition", pos);
      }
      defmeasdefmap_it x(default_measdef.find(measdef->getid()));
      if (x != default_measdef.end()) {
        x->second->replace(*measdef);
        measdef = x->second;
      } else
        default_measdef.insert(defmeasdefmap_val(measdef->getid(), measdef));
      lastent = measdef;
      measdef.reset(new measdef_str());
    } catch (const errbase& e) {
      measdef.reset(new measdef_str());
      throw;
    }
  }
  void
  fomusdata::set_measdef() { // doesn't require an id!--for "embedded" measattrs
    try {
      measdef->complete(*this);
      if (!measdef->getid().empty()) {
        defmeasdefmap_it x(default_measdef.find(measdef->getid()));
        if (x != default_measdef.end()) {
          x->second->replace(*measdef);
          measdef = x->second;
        } else
          default_measdef.insert(defmeasdefmap_val(measdef->getid(), measdef));
      }
      curmeasdef = measdef;
      measdef.reset(new measdef_str());
    } catch (const errbase& e) {
      measdef.reset(new measdef_str());
      throw;
    }
  }
  void fomusdata::set_measdef(std::string str) {
    if (str.empty())
      str = "default";
    defmeasdefmap_it x(default_measdef.find(str));
    if (x == default_measdef.end()) {
      CERR << "measure definition `" << str << "' not found";
      pos.printerr();
      throw errbase();
    }
    curmeasdef = x->second;
  }

  const part_str& fomusdata::getdefpart(const std::string& id) const {
    defpartsmap_constit i(default_parts.find(id));
    if (i == default_parts.end()) {
      if (boost::algorithm::iequals(id, "default"))
        return *thedefpartdef;
      CERR << "part `" << id << "' doesn't exist";
      pos.printerr();
      throw errbase();
    }
    return *i->second;
  }
  void fomusdata::setdefpartormpartshptr(
      const std::string& id,
      boost::variant<boost::shared_ptr<part_str>, boost::shared_ptr<mpart_str>,
                     std::string>& p) const {
    defpartsmap_constit i(default_parts.find(id));
    if (i == default_parts.end()) {
      if (boost::algorithm::iequals(id, "default")) {
        p = thedefpartdef;
        return;
      }
      defmpartsmap_constit i0(default_mparts.find(id));
      if (i0 != default_mparts.end()) {
        p = i0->second;
        return;
      }
      if (boost::algorithm::iequals(id, "all")) {
        p = theallmpartdef;
        return;
      }
      CERR << "part `" << id << "' doesn't exist";
      pos.printerr();
      throw errbase();
    }
    p = i->second;
  }
  const mpart_str& fomusdata::getdefmpart(const std::string& id) const {
    defmpartsmap_constit i(default_mparts.find(id));
    if (i == default_mparts.end()) {
      if (boost::algorithm::iequals(id, "all"))
        return *theallmpartdef;
      CERR << "metapart `" << id << "' doesn't exist";
      pos.printerr();
      throw errbase();
    }
    return *i->second;
  }
  boost::shared_ptr<measdef_str>&
  fomusdata::getdefmeasdefptr(const std::string& id) {
    defmeasdefmap_it i(default_measdef.find(id));
    if (i == default_measdef.end()) {
      if (boost::algorithm::iequals(id, "default"))
        return thedefmeasdef;
      CERR << "measure definition `" << id << "' doesn't exist";
      pos.printerr();
      throw errbase();
    }
    return i->second;
  }

  void percinstr_str::completeaux(const percinstr_str& b) {
    if (!imsmod)
      copyimps(b.ims, ims, this);
    if (!ex.get() && b.ex.get())
      ex = b.ex->copy(this); // <----- copy it!
    completesets(b);
  }

  class percinstr_isstring : public boost::static_visitor<bool> {
public:
    bool operator()(const std::string& x) const {
      return true;
    }
    bool operator()(const boost::shared_ptr<percinstr_str>& x) const {
      return false;
    }
  };
  class convert_percinstr
      : public boost::static_visitor<boost::shared_ptr<percinstr_str>> {
    fomusdata& fd;
    const instr_str* par;

public:
    convert_percinstr(fomusdata& fd, const instr_str* par) : fd(fd), par(par) {}
    boost::shared_ptr<percinstr_str> operator()(const std::string& x) const {
      return fd.getdefpercinstr(x).copy(/*&fd,*/ par);
    }
    boost::shared_ptr<percinstr_str>
    operator()(const boost::shared_ptr<percinstr_str>& x) const {
      assert(false);
    }
  };
  class convert_gpercinstr
      : public boost::static_visitor<boost::shared_ptr<percinstr_str>> {
    const instr_str* par;
    const filepos& pos;

public:
    convert_gpercinstr(const instr_str* par, const filepos& pos)
        : par(par), pos(pos) {}
    boost::shared_ptr<percinstr_str> operator()(const std::string& x) const {
      return getaglobpercinstr(x, pos).copy(/*0,*/ par);
    }
    boost::shared_ptr<percinstr_str>
    operator()(const boost::shared_ptr<percinstr_str>& x) const {
      assert(false);
    }
  };
  void instr_str::completeaux(const instr_str& b) {
    if (!stavesmod)
      staves = b.staves;
    if (!imsmod)
      copyimps(b.ims, ims, this);
    if (!ex.get() && b.ex.get())
      ex = b.ex->copy(this);
    completesets(b);
  }
  void instr_str::complete(fomusdata& fd) {
    if (!basedon.empty()) {
      const instr_str& b(fd.getdefinstr(basedon));
      completeaux(b);
      if (!percsmod)
        percs = b.percs;
      else
        goto MORE;
    } else {
    MORE:
      for (std::vector<boost::variant<boost::shared_ptr<percinstr_str>,
                                      std::string>>::iterator i(percs.begin());
           i != percs.end(); ++i) {
        if (boost::apply_visitor(percinstr_isstring(), *i))
          *i = boost::apply_visitor(convert_percinstr(fd, this),
                                    *i); // if NULL, supply default instr
      }
    }
  }
  void instr_str::complete(const filepos& pos) {
    if (!basedon.empty()) {
      const instr_str& b(getaglobinstr(basedon, pos));
      completeaux(b);
      if (!percsmod)
        percs = b.percs;
      else
        goto MORE;
    } else {
    MORE:
      for (std::vector<boost::variant<boost::shared_ptr<percinstr_str>,
                                      std::string>>::iterator i(percs.begin());
           i != percs.end(); ++i) {
        if (boost::apply_visitor(percinstr_isstring(), *i))
          *i = boost::apply_visitor(convert_gpercinstr(this, pos),
                                    *i); // if NULL, supply default instr
      }
    }
  }

  class instr_iszero : public boost::static_visitor<bool> {
public:
    bool operator()(const std::string& x) const {
      return false;
    }
    bool operator()(const boost::shared_ptr<instr_str>& x) const {
      return x.get() == 0;
    }
  };
  class instr_mustconv : public boost::static_visitor<bool> {
public:
    bool operator()(const std::string& x) const {
      return true;
    }
    bool operator()(const boost::shared_ptr<instr_str>& x) const {
      return x.get() == 0;
    }
  };
  class convert_instr
      : public boost::static_visitor<boost::shared_ptr<instr_str>> {
    fomusdata& fd;
    const part_str* par;

public:
    convert_instr(fomusdata& fd, const part_str* par) : fd(fd), par(par) {}
    boost::shared_ptr<instr_str> operator()(const std::string& x) const {
      return fd.getdefinstr(x).copy(/*&fd,*/ par);
    }
    boost::shared_ptr<instr_str>
    operator()(const boost::shared_ptr<instr_str>& x) const {
      return fd.getdefinstr("default").copy(/*&fd,*/ par);
    }
  };
  void part_str::complete(fomusdata& fd) {
    if (!basedon.empty()) {
      const part_str& b(fd.getdefpart(basedon));
      completesets(b);
      if (apply_visitor(instr_iszero(), instr))
        instr = b.instr;
      else
        goto MORE; // copy the variant--don't (shouldn't) need the stored
                   // object, it'll still be there
    } else {
    MORE:
      if (apply_visitor(instr_mustconv(), instr))
        instr = boost::apply_visitor(convert_instr(fd, this),
                                     instr); // if NULL, supply default instr
    }
#ifndef NDEBUG
    boost::apply_visitor(instr_assert(), instr);
#endif
    assert(prt.get());
    prt->insdefmeas(fd.getdefmeasdefptr("default"));
  }

  void mpart_str::complete(fomusdata& fd) {
    if (!basedon.empty()) {
      const mpart_str* b = fd.getdefmpart_noexc(basedon);
      if (b) {
        if (parts.empty())
          parts = b->parts;
        completesets(*b);
      } else { // try a parts struct--can't copy mpart specific stuff then
        const part_str& b2(fd.getdefpart(basedon)); // will throw if error
        completesets(b2);
      }
    }
    prt->insdefmeas(fd.getdefmeasdefptr("default"));
  }

  fomusdata::fomusdata()
      :
#ifndef NDEBUG
        fomusdebug(12345),
#endif
        curvar(-1), pos(info_global), queuestate(false), data(&datanorm),
        redunent(false), soff(false), curblast(fomus_par_noteevent),
        imp(new import_str()), exp(new export_str()), clef(new clef_str()),
        staff(new staff_str()), perc(new percinstr_str()),
        instr(new instr_str()), apart(new part_str()),
        partmap(new partmap_str()), mpart(new mpart_str()),
        measdef(new measdef_str()), theallmpartdef(new mpart_str("all")),
        merge(module_none), note_inparse(note_parse), acc_inparse(acc_parse),
        mic_inparse(mic_parse), oct_inparse(oct_parse),
        note_inprint(note_print), acc_inprint(acc_print),
        mic_inprint(mic_print), oct_inprint(oct_print), prevnote((fint) 60),
        prevnotegup(true), stagenum(0),
        partind(-(std::numeric_limits<fint>::min() / 2)), grpcnt(0) {
    std::for_each(
        vars.begin(), vars.end(),
        boost::lambda::bind(&fomusdata::makein, this, boost::lambda::_1));
    thedefmeasdef.reset(new measdef_str(0)); // default measure
    part_str* tmp;
    thedefpartdef.reset(tmp = new part_str("default"));
    curseldpart = thedefpartdef;
    scoreparts.push_back(curseldpart); // default part
    tmp->setself(boost::prior(scoreparts.end()), nextpartind());
    const globinstsmap& xx =
        ((const instrs_var*) vars[INSTRS_ID].get())->getmap();
    globinstsmap_constit i(xx.find("default")); // default instrument
    if (i != xx.end()) {
      boost::shared_ptr<instr_str> t(i->second->copy(tmp));
      tmp->setinstrstr(t);
    } else {
      boost::shared_ptr<instr_str> t(new instr_str("default"));
      tmp->setinstrstr(t);
    }
    boost::shared_ptr<measdef_str>& x(getdefmeasdefptr("default"));
    tmp->insdefmeas(x);
    setlist.n = 0;
    setlist.sets = 0;
    // default_mparts.insert(defmpartsmap_val("all", theallmpartdef));
    scoreparts.push_back(theallmpartdef);
    theallmpartdef->setself(boost::prior(scoreparts.end()), nextpartind());
    theallmpartdef->insdefmeas(x);
  }
  fomusdata::fomusdata(fomusdata& x)
      :
#ifndef NDEBUG
        fomusdebug(12345),
#endif
        invars(x.invars),       // COPY SETTINGS!
        strinvars(x.strinvars), // get rid of this eventually, don't need it
        curvar(-1), pos(info_global), queuestate(false), data(&datanorm),
        redunent(false), soff(false), curblast(fomus_par_noteevent),
        makemeass(x.makemeass), // COPY MEASURE!
        imp(new import_str()), exp(new export_str()), clef(new clef_str()),
        staff(new staff_str()), perc(new percinstr_str()),
        instr(new instr_str()), apart(new part_str()),
        partmap(new partmap_str()), mpart(new mpart_str()),
        measdef(new measdef_str()),

        default_parts(x.default_parts), // COPY DEFAULT OBJMAPS
        default_mparts(x.default_mparts), default_insts(x.default_insts),
        default_percs(x.default_percs), default_measdef(x.default_measdef),
        thedefmeasdef(x.thedefmeasdef),

        scoreparts(x.scoreparts), merge(module_none),

        note_inparse(note_parse), acc_inparse(acc_parse),
        mic_inparse(mic_parse), oct_inparse(oct_parse),
        note_inprint(note_print), acc_inprint(acc_print),
        mic_inprint(mic_print), oct_inprint(oct_print), prevnote((fint) 60),
        prevnotegup(true), stagenum(0),
        partind(x.partind), // COPY PART INDEX COUNTER!
        grpcnt(0) {
    DBG("############## COPY COPY COPY" << std::endl);
#ifndef NDEBUGOUT
    for (defpartsmap_it jj(default_parts.begin()); jj != default_parts.end();
         ++jj)
      DBG("default_parts: " << jj->second.get() << std::endl);
    for (defmpartsmap_it jj(default_mparts.begin()); jj != default_mparts.end();
         ++jj)
      DBG("default_mparts: " << jj->second.get() << std::endl);
    for (scorepartlist_it jj(scoreparts.begin()); jj != scoreparts.end(); ++jj)
      DBG("scoreparts: " << jj->get() << std::endl);
#endif
    DBG("makemeass.size() = " << makemeass.size() << std::endl);
    DBG("x.makemeass.size() = " << x.makemeass.size() << std::endl);
    std::map<part_str*, boost::shared_ptr<part_str>> clones;
    for (scorepartlist_it i(scoreparts.begin()); i != scoreparts.end(); ++i) {
      if (!(*i)->ismetapart()) {
        boost::shared_ptr<part_str> tmp(((part_str&) (**i)).fomclone());
        //#warning "compare part against x.thedefpart and reset to tmp clone if
        //matches"
        clones.insert(
            std::map<part_str*, boost::shared_ptr<part_str>>::value_type(
                (part_str*) i->get(), tmp));
        tmp->setself(i);
        if (i->get() == x.thedefpartdef.get())
          thedefpartdef = tmp;
        for (defpartsmap_it j(default_parts.begin()); j != default_parts.end();
             ++j) {
          if (j->second == *i) {
            j->second = tmp;
#ifndef NDEBUG
            for (defpartsmap_it jj(default_parts.begin());
                 jj != default_parts.end(); ++jj) {
              assert(jj->second != *i);
            }
#endif
            break;
          }
        }
        *i = tmp;
      }
    }
#ifndef NDEBUGOUT
    for (defpartsmap_it jj(default_parts.begin()); jj != default_parts.end();
         ++jj)
      DBG("default_parts: " << jj->second.get() << std::endl);
    for (defmpartsmap_it jj(default_mparts.begin()); jj != default_mparts.end();
         ++jj)
      DBG("default_mparts: " << jj->second.get() << std::endl);
    for (scorepartlist_it jj(scoreparts.begin()); jj != scoreparts.end(); ++jj)
      DBG("scoreparts: " << jj->get() << std::endl);
#endif
    std::map<mpart_str*, boost::shared_ptr<mpart_str>> mclones;
    for (scorepartlist_it i(scoreparts.begin()); i != scoreparts.end(); ++i) {
      if ((*i)->ismetapart()) {
        std::map<mpart_str*, boost::shared_ptr<mpart_str>>::iterator z(
            mclones.find((mpart_str*) i->get()));
        if (z != mclones.end()) {
          *i = z->second; // got a clone already
          continue;
        }
        boost::shared_ptr<mpart_str> tmp(
            ((mpart_str&) (**i)).fomclone(0, clones, mclones));
        mclones.insert(
            std::map<mpart_str*, boost::shared_ptr<mpart_str>>::value_type(
                (mpart_str*) i->get(), tmp));
        tmp->setself(i);
        if (i->get() == x.theallmpartdef.get())
          theallmpartdef = tmp;
        for (defmpartsmap_it j(default_mparts.begin());
             j != default_mparts.end(); ++j) {
          if (j->second == *i) {
            j->second = tmp;
#ifndef NDEBUG
            for (defmpartsmap_it jj(default_mparts.begin());
                 jj != default_mparts.end(); ++jj) {
              assert(jj->second != *i);
            }
#endif
            break;
          }
        }
        DBG("i=" << (partormpart_str*) i->get()
                 << " tmp=" << (partormpart_str*) tmp.get() << std::endl);
        *i = tmp;
        DBG("i=" << (partormpart_str*) i->get()
                 << " tmp=" << (partormpart_str*) tmp.get() << std::endl);
      }
    }
#ifndef NDEBUGOUT
    for (defpartsmap_it jj(default_parts.begin()); jj != default_parts.end();
         ++jj) {
      DBG("default_parts: " << jj->second.get() << " instobj: "
                            << jj->second->getinstobj() << std::endl);
    }
    for (defmpartsmap_it jj(default_mparts.begin()); jj != default_mparts.end();
         ++jj)
      DBG("default_mparts: " << jj->second.get() << std::endl);
    for (scorepartlist_it jj(scoreparts.begin()); jj != scoreparts.end(); ++jj)
      DBG("scoreparts: " << jj->get() << std::endl);
#endif
    assert(!scoreparts.empty());
    if (!thedefpartdef.get())
      thedefpartdef.reset(new part_str());
    curseldpart = thedefpartdef;
    if (!theallmpartdef.get())
      theallmpartdef.reset(new mpart_str());
    setlist.n = 0;
    setlist.sets = 0;
    DBG("KEYSIGS is " << &(const var_keysigs&) get_varbase(KEYSIG_ID)
                      << std::endl);
  }

  void fomusdata::mergeinto(fomusdata& x) {
    if (&x == this)
      return;
    std::map<part_str*, boost::shared_ptr<part_str>> clones;
    const scorepartlist_it beg((x.scoreparts.size() > 1 &&
                                x.scoreparts.front()->getpart().tmpevsempty())
                                   ? boost::next(x.scoreparts.begin())
                                   : x.scoreparts.begin());
    for (scorepartlist_it i(beg); i != x.scoreparts.end(); ++i) {
      if (!(*i)->ismetapart()) {
        part_str& xprt((part_str&) (**i));                  // from
        defpartsmap_it k(default_parts.find(xprt.getid())); // to
        if (k == default_parts
                     .end()) { // not in default parts list, clone and insert
          if (i == x.scoreparts.begin()) { // default
            assert(!scoreparts.empty() && !scoreparts.front()->ismetapart());
            clones.insert(
                std::map<part_str*, boost::shared_ptr<part_str>>::value_type(
                    (part_str*) i->get(),
                    (boost::shared_ptr<part_str>&) scoreparts.front()));
            scoreparts.front()->mergefrom(xprt.getpart(), x.merge);
          } else {
            boost::shared_ptr<part_str> tmp(xprt.fomclone(x.merge)); // clone
            clones.insert(
                std::map<part_str*, boost::shared_ptr<part_str>>::value_type(
                    (part_str*) i->get(), tmp));
            assert(xprt.getid() == tmp->getid());
            if (!xprt.getid().empty())
              default_parts.insert(defpartsmap_val(xprt.getid(), tmp));
            scoreparts.push_back(tmp);
            tmp->setself(boost::prior(scoreparts.end()));
          }
        } else { // matches id in default parts list, just copy events/measures
                 // and add to scoreparts list if not there
          for (scorepartlist_it w(scoreparts.begin()); w != scoreparts.end();
               ++w) {
            if ((partormpart_str*) k->second.get() == w->get())
              goto SKIP1;
          }
          scoreparts.push_back(k->second);
        SKIP1:
          clones.insert(
              std::map<part_str*, boost::shared_ptr<part_str>>::value_type(
                  (part_str*) i->get(), k->second));
          k->second->mergefrom(xprt.getpart(), x.merge);
        }
      }
    }
    std::map<mpart_str*, boost::shared_ptr<mpart_str>> mclones;
    for (scorepartlist_it i(beg); i != x.scoreparts.end(); ++i) {
      if ((*i)->ismetapart()) {
        mpart_str& xprt((mpart_str&) (**i));                  // from
        defmpartsmap_it k(default_mparts.find(xprt.getid())); // to
        if (k == default_mparts
                     .end()) { // not in default mparts list, clone and insert
          boost::shared_ptr<mpart_str> tmp; // clone
          std::map<mpart_str*, boost::shared_ptr<mpart_str>>::iterator z(
              mclones.find((mpart_str*) i->get()));
          if (z != mclones.end())
            tmp = z->second;
          else
            tmp.reset(
                ((mpart_str&) (**i)).fomclone(0, clones, mclones, x.merge));
          mclones.insert(
              std::map<mpart_str*, boost::shared_ptr<mpart_str>>::value_type(
                  (mpart_str*) i->get(), tmp));
          assert(xprt.getid() == tmp->getid());
          if (!xprt.getid().empty())
            default_mparts.insert(defmpartsmap_val(xprt.getid(), tmp));
          scoreparts.push_back(tmp); // to
          tmp->setself(boost::prior(scoreparts.end()));
        } else {
          for (scorepartlist_it w(scoreparts.begin()); w != scoreparts.end();
               ++w) {
            if ((partormpart_str*) k->second.get() == w->get())
              goto SKIP2;
          }
          scoreparts.push_back(k->second);
        SKIP2:
          mclones.insert(
              std::map<mpart_str*, boost::shared_ptr<mpart_str>>::value_type(
                  (mpart_str*) i->get(), k->second));
          k->second->mergefrom(xprt.getpart(), x.merge);
        }
      }
    }
    x.merge = module_none;
  }

  void fomusdata::get_settinginfo(info_setting& info,
                                  const varbase& var) const {
    info.modname = var.getmodcname();
    info.name = var.getname();
    info.id = var.getid();
    info.basetype = var.gettype();
    info.typedoc = var.gettypedoc();
    info.descdoc = var.getdescdoc();
    info.loc = var.getloc();
    info.val = var.getmodval();
    info.valstr = var.getnewvalstr(this, "");
    info.uselevel = var.getuselevel();
    info.where = var.getmodif();
    info.reset = var.getreset();
  }

  void get_markinfo(info_mark& m, const markbase& b) {
    m.modname = b.getmodcname();
    m.name = b.getname();
    m.argsdoc = b.gettypedoc();
    m.type = b.gettype();
    m.descdoc = b.getdoc();
  }

  scoped_info_marklist marklist;
  scoped_info_setlist globsetlist;

  const info_objinfo_list& fomusdata::getpercinstsinfo() {
    inoutpercinsts.resize(default_percs.size());
    for_each2(
        default_percs.begin(), default_percs.end(), inoutpercinsts.objs,
        boost::lambda::bind(
            &fomusdata::fillinoutinfo2<percinstr_str>, this, boost::lambda::_2,
            boost::lambda::bind(&boost::shared_ptr<percinstr_str>::get,
                                boost::lambda::bind(&defpercsmap_val::second,
                                                    boost::lambda::_1))));
    return inoutpercinsts;
  }
  const info_objinfo_list& fomusdata::getinstsinfo() {
    inoutinsts.resize(default_insts.size());
    for_each2(
        default_insts.begin(), default_insts.end(), inoutinsts.objs,
        boost::lambda::bind(
            &fomusdata::fillinoutinfo2<instr_str>, this, boost::lambda::_2,
            boost::lambda::bind(&boost::shared_ptr<instr_str>::get,
                                boost::lambda::bind(&definstsmap_val::second,
                                                    boost::lambda::_1))));
    return inoutinsts;
  }
  struct getpartsinfosort : std::binary_function<const partormpart_str*,
                                                 const partormpart_str*, bool> {
    bool operator()(const partormpart_str* x, const partormpart_str* y) const {
      return x->insord < y->insord;
    }
  };
  const info_objinfo_list& fomusdata::getpartsinfo() {
    inoutparts.resize(default_parts.size());
    std::vector<part_str*> tmp;
    std::transform(
        default_parts.begin(), default_parts.end(), std::back_inserter(tmp),
        boost::lambda::bind(
            &boost::shared_ptr<part_str>::get,
            boost::lambda::bind(&defpartsmap_val::second, boost::lambda::_1)));
    std::sort(tmp.begin(), tmp.end(), getpartsinfosort());
    for_each2(tmp.begin(), tmp.end(), inoutparts.objs,
              boost::lambda::bind(&fomusdata::fillinoutinfo2<part_str>, this,
                                  boost::lambda::_2, boost::lambda::_1));
    return inoutparts;
  }
  const info_objinfo_list& fomusdata::getmpartsinfo() {
    inoutmparts.resize(default_mparts.size());
    std::vector<mpart_str*> strs;
    std::transform(
        default_mparts.begin(), default_mparts.end(), std::back_inserter(strs),
        boost::lambda::bind(
            &boost::shared_ptr<mpart_str>::get,
            boost::lambda::bind(&defmpartsmap_val::second, boost::lambda::_1)));
    std::sort(strs.begin(), strs.end(), getpartsinfosort());
    std::stable_sort(strs.begin(), strs.end(),
                     boost::lambda::bind(&mpart_str::contains,
                                         boost::lambda::_2, boost::lambda::_1));
    for_each2(strs.begin(), strs.end(), inoutmparts.objs,
              boost::lambda::bind(&fomusdata::fillinoutinfo2m, this,
                                  boost::lambda::_2, boost::lambda::_1));
    return inoutmparts;
  }
  const info_objinfo_list& fomusdata::getmeasdefinfo() {
    inoutmeasdef.resize(default_measdef.size());
    for_each2(
        default_measdef.begin(), default_measdef.end(), inoutmeasdef.objs,
        boost::lambda::bind(
            &fomusdata::fillinoutinfo<measdef_str>, this, boost::lambda::_2,
            boost::lambda::bind(&boost::shared_ptr<measdef_str>::get,
                                boost::lambda::bind(&defmeasdefmap_val::second,
                                                    boost::lambda::_1))));
    return inoutmeasdef;
  }

  // void fomusdata::setpart(const std::string& str) {
  //   defmpartsmap_constit i2(default_mparts.find(str));
  //   if (i2 == default_mparts.end()) {
  //     if (boost::algorithm::iequals(str, "all")) curseldpart =
  //     theallmpartdef; else {
  // 	defpartsmap_constit i3(default_parts.find(str));
  // 	if (i3 == default_parts.end()) {
  // 	  if (boost::algorithm::iequals(str, "default")) curseldpart =
  // thedefpartdef; 	  else { 	    CERR << "part `" << str << "' doesn't exist";
  // 	    pos.printerr();
  // 	    throw errbase();
  // 	  }
  // 	} else curseldpart = i3->second;
  //     }
  //   } else curseldpart = i2->second;
  // }

  void fomusdata::setpart(const std::string& str) {
    defpartsmap_constit i3(default_parts.find(str));
    if (i3 == default_parts.end()) {
      if (boost::algorithm::iequals(str, "default"))
        curseldpart = thedefpartdef;
      else {
        defmpartsmap_constit i2(default_mparts.find(str));
        if (i2 == default_mparts.end()) {
          if (boost::algorithm::iequals(str, "all"))
            curseldpart = theallmpartdef;
          else {
            CERR << "part `" << str << "' doesn't exist";
            pos.printerr();
            throw errbase();
          }
        } else
          curseldpart = i2->second;
      }
    } else
      curseldpart = i3->second;
  }

  void fomusdata::endregion(const fint val) { // actual end of region
    for (datastack_rit i(stack.rbegin()); i != stack.rend(); ++i) {
      if (i->hasid(val)) {
        stack.erase(prior(i.base()));
        return;
      }
    }
    CERR << "region id mismatch";
    pos.printerr();
    throw errbase();
  }

  inline void checkset(const setmap::value_type& v,
                       const enum module_setting_loc l, const char* wh,
                       const filepos& pos) {
    if (!v.second->isallowed(l)) {
      CERR << "cannot set setting `" << v.second->getname() << "' inside a "
           << wh;
      pos.printerr();
      throw errbase();
    }
  }

  void dataholdernorm::stickin(fomusdata& fd) {
    if (!fd.soff && groff.isnull()) {
      groff = groffbak;
    }
    try {
      numb du;
      switch (fd.curblast) {
      case fomus_par_noteevent:
        checknumbs(fd.getpos());
        break;
      case fomus_par_markevent:
      case fomus_par_restevent:
        checktimenumbs(fd.getpos());
        checkdurnumb(fd.getpos());
        break;
      case fomus_par_measevent:
      case fomus_par_meas:
        checktimenumbs(fd.getpos());
        du = checkmdurnumb(fd.getpos());
        break;
#ifndef NDEBUG
      default:
        assert(false);
#endif
      }
      if (!fd.getstack().empty())
        sets.insert(fd.getstack().back().sets.begin(),
                    fd.getstack()
                        .back()
                        .sets.end()); // these sets shouldn't be overwritten!
      switch (fd.curblast) {
      case fomus_par_noteevent:
      case fomus_par_restevent:
      case fomus_par_markevent:
        std::for_each(sets.begin(), sets.end(),
                      boost::lambda::bind(
                          checkset, boost::lambda::_1, module_locnote, "note",
                          boost::lambda::constant_ref(fd.getpos())));
        break;
      case fomus_par_measevent:
      case fomus_par_meas:
        std::for_each(sets.begin(), sets.end(),
                      boost::lambda::bind(
                          checkset, boost::lambda::_1, module_locmeasdef,
                          "measure", boost::lambda::constant_ref(fd.getpos())));
        break;
#ifndef NDEBUG
      default:
        assert(false);
#endif
      }
      std::set<int> vs(voices);
      std::for_each(fd.getstack().begin(), fd.getstack().end(),
                    boost::lambda::bind(&dataholderreg::modvoices,
                                        boost::lambda::_1,
                                        boost::lambda::var(vs)));
      std::for_each(
          vs.begin(), vs.end(),
          boost::lambda::bind(checkvoice, boost::lambda::_1,
                              boost::lambda::constant_ref(fd.getpos())));
      std::for_each(
          marks.begin(), marks.end(),
          boost::lambda::bind(&markobj::checkargs, boost::lambda::_1, &fd,
                              boost::lambda::constant_ref(fd.getpos())));
      boost::ptr_set<markobj> mm;
      std::for_each(fd.getstack().begin(), fd.getstack().end(),
                    boost::lambda::bind(&dataholderreg::modmarks,
                                        boost::lambda::_1,
                                        boost::lambda::var(mm)));
      mm.transfer(marks); // transfering is ok here
      const char* pp =
          (perc.empty()) ? 0 : fd.percinstnames.insert(perc).first->c_str();
      if (endtime.isntnull()) { // if user has set the endtime
        if (groff.isnull()) {
          dur = endtime - off;
        } else {
          dur = grendtime - groff;
        }
        if (dur < (fint) 0)
          dur = (fint) 0; // it could happen
        endtime.null();
        point = point_none;
      }
      if ((dur.isntnull() && dur > (fint) 0) ||
          fd.curblast == fomus_par_markevent)
        point = point_none;
      else {
        if (point == point_none)
          point = point_auto;
        if (groff.isnull()) {
          switch (point) {
          case point_auto: // same as point_right
          case point_right:
            groff = std::numeric_limits<fint>::max() / 2;
            break;
          case point_left:
            groff = std::numeric_limits<fint>::min() / 2;
            break;
          default:;
          }
        } else {
          switch (point) {
          case point_auto:
            point = point_grauto;
            break;
          case point_right:
            point = point_grright;
            break;
          case point_left:
            point = point_grleft;
            break;
          default:;
          }
        }
      }
      switch (fd.curblast) {
      case fomus_par_noteevent:
        fd.curseldpart->insertnew(new noteev(off, groff, dur, pitch, dyn, vs,
                                             fd.getpos(), mm, pp, point, sets));
        break;
      case fomus_par_restevent:
        fd.curseldpart->insertnew(
            new restev(off, groff, dur, vs, fd.getpos(), mm, point, sets));
        break;
      case fomus_par_markevent:
        fd.curseldpart->insertnewmarkev(
            new markev(off, groff, dur, vs, fd.getpos(), mm, point, sets));
        break;
      case fomus_par_measevent:
        if (!fd.getcurmeasdef().get())
          fd.getcurmeasdef() = fd.getdefmeasdefptr("default");
        assert(fd.getcurmeasdef().get());
        fd.getcurmeasdef()->sets.insert(sets.begin(),
                                        sets.end()); // continue to next!
        fd.blastmeasaux(du);
        break;
      case fomus_par_meas:
        if (!fd.getcurmeasdef().get())
          fd.getcurmeasdef() = fd.getdefmeasdefptr("default");
        fd.blastmeasaux(du);
        break;
#ifndef NDEBUG
      default:
        assert(false);
#endif
      }
    } catch (const errbase& e) {
      marks.clear();
      sets.clear();
      throw;
    }
    marks.clear();
    sets.clear();
    off0 = off;
    groff0 = groff;
    dur0 = dur;
    point = point_none;
    fd.curblast = fomus_par_noteevent;
    if (groff.isntnull() && (groff >= std::numeric_limits<fint>::max() / 2 ||
                             groff <= std::numeric_limits<fint>::min() / 2))
      groff.null();
    fd.redunent = true;
    cleargr();
    fd.soff = false;
    DBG("soff is off" << std::endl);
  }

  void fomusdata::filltmppart() {
    tmpmeass.clear();
    std::for_each(
        scoreparts.begin(), scoreparts.end(),
        boost::lambda::bind(
            &partormpart_str::filltmppart,
            boost::lambda::bind(&boost::shared_ptr<partormpart_str>::get,
                                boost::lambda::_1),
            boost::lambda::var(tmpmeass)));
  }

  void scoped_info_objinfo_list::destroyvals() {
    std::for_each(
        arr.begin(), arr.end(),
        (boost::lambda::bind(
             delete_charptr0,
             boost::lambda::bind(&info_objinfo::valstr, boost::lambda::_1)),
         boost::lambda::bind(
             freevalue,
             boost::lambda::bind(&info_objinfo::val, boost::lambda::_1))));
  }

  void fomusdata::remfrompartlist(const std::string& id0) {
    assert(!scoreparts.empty());
    assert(scoreparts.front()->getid() == "default");
    for (scorepartlist_it i(boost::next(scoreparts.begin()));
         i != scoreparts.end();) {
      if (boost::algorithm::iequals((*i)->getid(), id0))
        i = scoreparts.erase(i);
      else
        ++i;
    }
  }

  void fomusdata::collectallvoices() {
    std::set<int> allvoices;
    std::for_each(
        scoreparts.begin(), scoreparts.end(),
        boost::lambda::bind(
            &partormpart_str::collectallvoices,
            boost::lambda::bind(&boost::shared_ptr<partormpart_str>::get,
                                boost::lambda::_1),
            boost::lambda::var(allvoices)));
    voicescache.assign(allvoices.begin(), allvoices.end());
  }

  void fomusdata::collectallstaves() {
    std::set<int> allstaves;
    std::for_each(
        scoreparts.begin(), scoreparts.end(),
        boost::lambda::bind(
            &partormpart_str::collectallstaves,
            boost::lambda::bind(&boost::shared_ptr<partormpart_str>::get,
                                boost::lambda::_1),
            boost::lambda::var(allstaves)));
    stavescache.assign(allstaves.begin(), allstaves.end());
  }

  void fomusdata::sortorder() {
    fomus_int srtord = -1;
    std::for_each(
        scoreparts.begin(), scoreparts.end(),
        boost::lambda::bind(
            &partormpart_str::sortord,
            boost::lambda::bind(&boost::shared_ptr<partormpart_str>::get,
                                boost::lambda::_1),
            boost::lambda::var(srtord)));
    for (measmapview_it i(tmpmeass.begin()); i != tmpmeass.end(); ++i)
      i->second->sortord(srtord);
  }

  void fomusdata::postmparts() {
    for (scorepartlist_it i(scoreparts.begin()); i != scoreparts.end();) {
      if ((*i)->ismetapart())
        i = scoreparts.erase(i);
      else
        (*i++)->postinput3();
    }
  }

  void fomusdata::fillnotes1() {
    std::for_each(
        scoreparts.begin(), scoreparts.end(),
        boost::lambda::bind(
            &partormpart_str::reinserttmps,
            boost::lambda::bind(&boost::shared_ptr<partormpart_str>::get,
                                boost::lambda::_1)));
  }

  void fomusdata::fillholes1() {
    std::for_each(
        scoreparts.begin(), scoreparts.end(),
        boost::lambda::bind(
            &partormpart_str::fillholes1,
            boost::lambda::bind(&boost::shared_ptr<partormpart_str>::get,
                                boost::lambda::_1)));
  }

  void fomusdata::fillholes2() {
    std::for_each(
        scoreparts.begin(), scoreparts.end(),
        boost::lambda::bind(
            &partormpart_str::fillholes2,
            boost::lambda::bind(&boost::shared_ptr<partormpart_str>::get,
                                boost::lambda::_1)));
  }

  void fomusdata::fillholes3() {
    std::for_each(
        scoreparts.begin(), scoreparts.end(),
        boost::lambda::bind(
            &partormpart_str::fillholes3,
            boost::lambda::bind(&boost::shared_ptr<partormpart_str>::get,
                                boost::lambda::_1)));
  }

  void fomusdata::insfills() {
    // its.clear();
    for (scorepartlist_it i(scoreparts.begin()); i != scoreparts.end(); ++i)
      (*i)->insertfiller();
  }
  void fomusdata::delfills() {
    // std::vector<eventmap_it>::iterator x(its.begin());
    for (scorepartlist_it i(scoreparts.begin()); i != scoreparts.end();
         ++i /*, ++x*/)
      (*i)->deletefiller(/**x*/);
  }

  void fomusdata::preprocess() {
    // its.clear();
    const numb& te(get_num(INITTEMPO_ID));
    const std::string& ts(get_sval(INITTEMPOTXT_ID));
    for (scorepartlist_it i(scoreparts.begin()); i != scoreparts.end(); ++i)
      (*i)->preprocess(te, ts);
  }

  void fomusdata::setnumbset_aux(std::auto_ptr<varbase>& v) {
    v->throwifinvalid(this);
    ++pos;
    varbase* v0;
    invars[curvar].reset(v0 = v.release());
    v0->activate(symtabs(note_inparse, acc_inparse, mic_inparse, oct_inparse,
                         note_inprint, acc_inprint, mic_inprint, oct_inprint,
                         this, durdot_inparse, dursyms_inparse, durtie_inparse,
                         tupsyms_inparse));
  }

} // namespace fomus
