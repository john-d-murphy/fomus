(fms:with-score
    (:filename *filename*
     :sets '((:quartertones t)
             (:double-accs nil)) ; boolean values
     :parts '((:id "prt1" :inst "guitar")))
  (loop
     for o from 0 below 3 by 1/2
     for p from 0 by 1/2
     do
       (fms:note :part 'prt1 :time o :dur 1/2 :pitch (+ 65 p))))
