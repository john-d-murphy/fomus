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

#ifndef FOMUS_INTERN_H
#define FOMUS_INTERN_H

#ifndef BUILD_LIBFOMUS
#error "intern.h shouldn't be included"
#endif

#include "heads.h"

#include "mods.h"
#include "modtypes.h"

#include "ifacedumb.h"

namespace fomus {

  void dopostmeas(const module_partobj p);

  extern "C" {
  const char* internalerr_fun(void* moddata);
  }

  void postmeasdoit(FOMUS fom, void* moddata);
  void posttquantdoit(FOMUS fom, void* moddata);
  void posttquantinvdoit(FOMUS fom, void* moddata);
  void postpquantdoit(FOMUS fom, void* moddata);
  void postvoicesdoit(FOMUS fom, void* moddata);
  void poststavesdoit(FOMUS fom, void* moddata);
  void poststavesdoit2(FOMUS fom, void* moddata);
  void postprunedoit(FOMUS fom, void* moddata);
  void prevspandoit(FOMUS fom, void* moddata);
  void postspandoit(FOMUS fom, void* moddata);
  void postoctdoit(FOMUS fom, void* moddata);
  void postacc1doit(FOMUS fom, void* moddata);
  void posttiedoit(FOMUS fom, void* moddata);
  void postbeamsdoit(FOMUS fom, void* moddata);
  void fillholesdoit(FOMUS fom, void* moddata);
  // void fillholesadoit(FOMUS fom, void* moddata);
  void postmergedoit(FOMUS fom, void* moddata);
  // void postpartsdoit(FOMUS fom, void* moddata);
  void spreadmarksdoit(FOMUS fom, void* moddata);
  // void mpartsdoit(FOMUS fom, void* moddata);
  void pnotesdoit(FOMUS fom, void* moddata);
  void fillnotes1doit(FOMUS fom, void* moddata);
  void fillnotes2doit(FOMUS fom, void* moddata);
  void postmarkevsdoit(FOMUS fom, void* moddata);
  void redotiesdoit(FOMUS fom, void* moddata);
  void finalmarksdoit(FOMUS fom, void* moddata);
  void postspecialdoit(FOMUS fom, void* moddata);
  void fixlyrsdoit(FOMUS fom, void* moddata);
  void barlinesdoit(FOMUS fom, void* moddata);
  void splittremsdoit(FOMUS fom, void* moddata);
  void markhelpersdoit(FOMUS fom, void* moddata);
  void inbetweenmarksdoit(FOMUS fom, void* moddata);
  void sysbreakdoit(FOMUS fom, void* moddata);
  void contmarksdoit(FOMUS fom, void* moddata);
  // void eatnotesdoit(FOMUS fom, void* moddata);
  // void fillnotes1adoit(FOMUS fom, void* moddata);
  void tremtiesdoit(FOMUS fom, void* moddata);

  struct intmodbase : public modbase {
    void* getdata(FOMUS f) const {
      return 0;
    }
    void freedata(void* dat) const {}
    enum module_type gettype() const {
      return module_modinternal;
    }
    const char* getiniterr() const {
      return 0;
    }
    int getsetting(int n, struct module_setting* set, int id) const {
      return 0;
    }
    int getifaceid() const {
      return ENGINE_INTERFACEID;
    }
    const char* whicheng(void* data) const {
      return "dumb";
    }
    bool getsameinst(void* a, void* b) const {
      return true;
    }
  };
  struct intmod_postmeas : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = postmeasdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart;
    }
  };
  struct intmod_posttquant : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = posttquantdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_all;
    }
  };
  struct intmod_posttquantinv : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = posttquantinvdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_all;
    }
  };
  struct intmod_postpquant : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = postpquantdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bymeas | module_norests | module_noperc | module_firsttied;
    }
  };
  struct intmod_postvoices : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = postvoicesdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart /*| module_noperc | module_firsttied |
                              module_norests*/
          ;
    }
  };
  struct intmod_poststaves : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = poststavesdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart /*| module_norests | module_firsttied*/;
    }
  };
  struct intmod_poststaves2 : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = poststavesdoit2;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bymeas /*| module_restsonly | module_noperc*/;
    }
  };
  struct intmod_postprune : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = postprunedoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bymeas /*| module_norests | module_noperc |
                              module_firsttied*/
          ;
    }
  };

  struct intmod_fixlyrs : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = fixlyrsdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart | module_byvoice;
    }
  };
  struct intmod_prevspan : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = prevspandoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart | module_byvoice;
    }
  };
  struct intmod_postvspan : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = postspandoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart | module_byvoice;
    }
  };
  struct intmod_postsspan : public intmodbase {
    void* getdata(FOMUS f) const {
      return (void*) 1;
    }
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata =
          (void*) 1; // same function, 1 = collect staff spanners instead
      ((dumb_iface*) iface)->run = postspandoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart | module_bystaff;
    }
  };
  struct intmod_postoct : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = postoctdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bymeas;
    }
  };

  struct intmod_postacc1 : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = postacc1doit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bymeas | module_norests | module_firsttied;
    }
  };

  struct intmod_posttie : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = posttiedoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bymeas;
    }
  };

  struct intmod_postbeams : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = postbeamsdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bymeas | module_byvoice;
    }
  };

  struct intmod_fillholes : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = fillholesdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bymeas | module_byvoice;
    }
  };

  struct intmod_postmerge : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = postmergedoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bymeas;
    }
  };

  // struct intmod_postparts:public intmodbase {
  //   void fillinterface(void* moddata, void* iface) const {
  //     ((dumb_iface*)iface)->moddata = 0;
  //     ((dumb_iface*)iface)->run = postpartsdoit;
  //     ((dumb_iface*)iface)->err = internalerr_fun;
  //   }
  //   int getitertype() const {return module_all;}
  // };

  struct intmod_spreadmarks : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = spreadmarksdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bymeas | module_byvoice;
    }
  };

  // struct intmod_mparts:public intmodbase {
  //   void fillinterface(void* moddata, void* iface) const {
  //     ((dumb_iface*)iface)->moddata = 0;
  //     ((dumb_iface*)iface)->run = mpartsdoit;
  //     ((dumb_iface*)iface)->err = internalerr_fun;
  //   }
  //   int getitertype() const {return module_all;}
  // };
  struct intmod_pnotes : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = pnotesdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bymeas;
    }
  };
  struct intmod_fillnotes1 : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = fillnotes1doit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart | module_byvoice;
    }
  };
  // struct intmod_fillnotes1a:public intmodbase {
  //   void fillinterface(void* moddata, void* iface) const {
  //     ((dumb_iface*)iface)->moddata = 0;
  //     ((dumb_iface*)iface)->run = fillnotes1adoit;
  //     ((dumb_iface*)iface)->err = internalerr_fun;
  //   }
  //   int getitertype() const {return module_bypart;}
  // };
  struct intmod_fillnotes2 : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = fillnotes2doit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bymeas | module_byvoice;
    }
  };
  struct intmod_postmarkevs : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = postmarkevsdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bymeas;
    }
  };
  struct intmod_redoties : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = redotiesdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart | module_byvoice | module_norests;
    }
  };

  struct intmod_finalmarksv : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata = 0;
      ((dumb_iface*) iface)->run = finalmarksdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart | module_byvoice;
    }
  };
  struct intmod_finalmarkss : public intmodbase {
    void* getdata(FOMUS f) const {
      return (void*) 1;
    }
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata =
          (void*) 1; // same function, 1 = collect staff spanners instead
      ((dumb_iface*) iface)->run = finalmarksdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart | module_bystaff;
    }
  };
  struct intmod_postspecial : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata =
          0; // same function, 1 = collect staff spanners instead
      ((dumb_iface*) iface)->run = postspecialdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart;
    }
  };
  struct intmod_barlines : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata =
          0; // same function, 1 = collect staff spanners instead
      ((dumb_iface*) iface)->run = barlinesdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart | module_byvoice;
    }
  };
  struct intmod_splittrems : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata =
          0; // same function, 1 = collect staff spanners instead
      ((dumb_iface*) iface)->run = splittremsdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart;
    }
  };
  struct intmod_markhelpers : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata =
          0; // same function, 1 = collect staff spanners instead
      ((dumb_iface*) iface)->run = markhelpersdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bymeas;
    }
  };
  struct intmod_inbetweenmarks : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata =
          0; // same function, 1 = collect staff spanners instead
      ((dumb_iface*) iface)->run = inbetweenmarksdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart | module_byvoice;
    }
  };
  struct intmod_sysbreak : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata =
          0; // same function, 1 = collect staff spanners instead
      ((dumb_iface*) iface)->run = sysbreakdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_all;
    }
  };
  struct intmod_contmarksv : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata =
          0; // same function, 1 = collect staff spanners instead
      ((dumb_iface*) iface)->run = contmarksdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart | module_byvoice;
    }
  };
  struct intmod_contmarkss : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata =
          (void*) 1; // same function, 1 = collect staff spanners instead
      ((dumb_iface*) iface)->run = contmarksdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart | module_bystaff;
    }
  };
  // struct intmod_eatnotes_bymeas:public intmodbase {
  //   void fillinterface(void* moddata, void* iface) const {
  //     ((dumb_iface*)iface)->moddata = 0;
  //     ((dumb_iface*)iface)->run = eatnotesdoit;
  //     ((dumb_iface*)iface)->err = internalerr_fun;
  //   }
  //   int getitertype() const {return module_bymeas;}
  // };
  struct intmod_tremties : public intmodbase {
    void fillinterface(void* moddata, void* iface) const {
      ((dumb_iface*) iface)->moddata =
          0; // same function, 1 = collect staff spanners instead
      ((dumb_iface*) iface)->run = tremtiesdoit;
      ((dumb_iface*) iface)->err = internalerr_fun;
    }
    int getitertype() const {
      return module_bypart | module_byvoice | module_norests;
    }
  };

  extern intmod_postmeas imod_postmeas_destr;
  extern intmod_posttquant imod_posttquant_destr;
  extern intmod_posttquantinv imod_posttquantinv_destr;
  extern intmod_postpquant imod_postpquant;
  extern intmod_postvoices imod_postvoices_destr;
  extern intmod_poststaves imod_poststaves_destr;
  extern intmod_poststaves2 imod_poststaves2_destr;
  extern intmod_postprune imod_postprune_destr;
  extern intmod_postsspan imod_postsspan;
  extern intmod_prevspan imod_prevspan;
  extern intmod_postvspan imod_postvspan;
  extern intmod_postoct imod_postoct;
  extern intmod_postacc1 imod_postacc1;
  extern intmod_posttie imod_posttie_destr;
  extern intmod_postbeams imod_postbeams;
  extern intmod_fillholes imod_fillholes_destr;
  // extern intmod_fillholesa imod_fillholesa_destr;
  extern intmod_postmerge imod_postmerge_destr;
  // extern intmod_postparts imod_postparts_destr;
  extern intmod_spreadmarks imod_spreadmarks;
  // extern intmod_mparts imod_mparts_destr;
  extern intmod_pnotes imod_pnotes_destr;
  extern intmod_fillnotes1 imod_fillnotes1_destr;
  // extern intmod_fillnotes1a imod_fillnotes1a_destr;
  extern intmod_fillnotes2 imod_fillnotes2_destr;
  extern intmod_postmarkevs imod_postmarkevs_destr;
  extern intmod_redoties imod_redoties_destr;
  extern intmod_finalmarksv imod_finalmarksv;
  extern intmod_finalmarkss imod_finalmarkss;
  extern intmod_postspecial imod_postspecial_destr;
  extern intmod_fixlyrs imod_fixlyrs;
  extern intmod_barlines imod_barlines;
  extern intmod_splittrems imod_splittrems_destr;
  extern intmod_markhelpers imod_markhelpers;
  extern intmod_inbetweenmarks imod_inbetweenmarks;
  extern intmod_sysbreak imod_sysbreak;
  extern intmod_contmarksv imod_contmarksv;
  extern intmod_contmarkss imod_contmarkss;
  // extern intmod_eatnotes_bymeas imod_eatnotes_bymeas;
  extern intmod_tremties imod_tremties;

} // namespace fomus

#endif
