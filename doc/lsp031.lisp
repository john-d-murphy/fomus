(fms:with-score
    (:filename *filename*
     :sets '((:timesigs ((2 8) (4 8) (8 8))) ; nested list of possible time signatures
             (:keysig-defs  ; mapping from keysig symbols to lists
              (asmin (fs cs gs ds as es bs)
               afmaj (bf ef af df)
               afmin (bf ef af df gf cf ff)
               amaj (fs cs gs)
               amin ()
               bfmaj (bf ef)
               bfmin (bf ef af df gf)
               bmaj (fs cs gs ds as)
               bmin (fs cs)
               csmaj (fs cs gs ds as es bs)
               csmin (fs cs gs ds)
               cfmaj (bf ef af df gf cf ff)
               cmaj ()
               cmin (bf ef af)
               dsmin (fs cs gs ds as es)
               dfmaj (bf ef af df gf)
               dmaj (fs cs)
               dmin (bf)
               efmaj (bf ef af)
               efmin (bf ef af df gf cf)
               emaj (fs cs gs ds)
               emin (fs)
               fsmaj (fs cs gs ds as es)
               fsmin (fs cs gs)
               fmaj (bf)
               fmin (bf ef af df)
               gsmin (fs cs gs ds as)
               gfmaj (bf ef af df gf cf)
               gmaj (fs)))
             (:keysig "dmaj"))
     :parts '((:id "prt1" :inst "guitar")))
  (loop
     for o from 0 below 3 by 1/2
     for p from 0
     do
       (fms:note :part 'prt1 :time o :dur 1/2 :pitch (+ 65 p)
                 :voice '(1 2)))) ; list of voices
