(fms:with-score
    (:filename *filename*
     :parts '((:id "prt1" :inst "guitar") ; using strings
              (:id prt2 :inst TUBA))) ; using symbols
  (loop
     for o from 0 below 3 by 1/2
     for p from 0
     do
       (fms:note :part 'prt1 :time o :dur 1/2 :pitch (+ p 65))
       (fms:note :part "prt2" :time o :dur 1/2 :pitch (- 48 p))))
